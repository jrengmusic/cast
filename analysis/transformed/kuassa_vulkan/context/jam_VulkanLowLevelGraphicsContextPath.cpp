//
// Path file: fillPath(), clipToPath(), triangulatePath(), and their helpers.
// Depends on setFullViewport() defined in jam_VulkanLowLevelGraphicsContext.cpp,
// included before this file in jam_vulkan.cpp.

namespace jam
{
/*____________________________________________________________________________*/
// flattenPathIntoSubPaths — flattens a Path through deviceTransform into a list of
// point rings, one per sub-path (PathFlatteningIterator::subPathIndex == 0 starts a
// new ring). Each ring is a closed polygon boundary in device space.
static jam::Array<jam::Array<juce::Point<float>>> flattenPathIntoSubPaths (const juce::Path& path,
                                                                              const juce::AffineTransform& deviceTransform)
{
    jam::Array<jam::Array<juce::Point<float>>> polygon;
    jam::Array<juce::Point<float>> currentSubPath;

    juce::PathFlatteningIterator iter (path, deviceTransform);

    while (iter.next())
    {
        if (iter.subPathIndex == 0 and not currentSubPath.isEmpty())
        {
            polygon.add (std::move (currentSubPath));
            currentSubPath.clear();
        }

        if (currentSubPath.isEmpty())
            currentSubPath.add ({ iter.x1, iter.y1 });

        currentSubPath.add ({ iter.x2, iter.y2 });
    }

    if (not currentSubPath.isEmpty())
        polygon.add (std::move (currentSubPath));

    return polygon;
}

/*____________________________________________________________________________*/
// appendEarcutTriangulatedRing — earcut-triangulates a single ring (sub-path
// boundary, requires at least 3 points) and appends its vertices + indices to the
// combined output buffers, offsetting indices by the current outVertices size so
// they remain valid against the combined array.
static void appendEarcutTriangulatedRing (const jam::Array<juce::Point<float>>& ring,
                                            jam::Array<juce::Point<float>>& outVertices,
                                            jam::Array<uint32_t>& outIndices)
{
    if (ring.size() >= 3)
    {
        auto ringIndices { jam::Earcut::triangulate (ring) };

        if (not ringIndices.isEmpty())
        {
            const auto baseVertex { static_cast<uint32_t> (outVertices.size()) };

            for (auto idx : ringIndices)
                outIndices.add (idx + baseVertex);

            for (const auto& vertex : ring)
                outVertices.add (vertex);
        }
    }
}

/*____________________________________________________________________________*/
// triangulatePath — earcut-triangulates each already-flattened sub-path ring
// independently (ring 0 = outer boundary, ring 1+ = holes per sub-path, but JUCE
// multi-subpath shapes like 3-slice SVGs should each fill independently — see
// fillPath/clipToPath callers). Appends combined vertex + index buffers ready for
// upload, with indices already offset to reference the combined vertex array.
//
// Takes the already-flattened polygon (rather than flattening internally) since
// fillPath() must inspect the ring count BEFORE choosing earcut vs. the
// complex-path branch (isComplexFillPath) — flattening once and sharing the result
// avoids a redundant second PathFlatteningIterator pass.
static void triangulatePath (const jam::Array<jam::Array<juce::Point<float>>>& polygon,
                              jam::Array<juce::Point<float>>& outVertices,
                              jam::Array<uint32_t>& outIndices)
{
    for (const auto& ring : polygon)
        appendEarcutTriangulatedRing (ring, outVertices, outIndices);
}

/*____________________________________________________________________________*/
// isComplexFillPath — triangulatePath()'s per-ring-independent
// earcut can only ever produce a nonzero-like continuous fill for ONE ring; it has no
// way to represent a hole (a second ring subtracted from the first). A single simple
// subpath has no second ring to disambiguate against, so its fill is identical under
// either winding rule — the winding rule only changes the result once multiple
// subpaths can overlap. Complexity is therefore gated on subpath count alone; the
// winding rule stays irrelevant here and is consulted later, at
// recordWindingAccumulateAndCoverDrawCommands()'s cover-draw stencil compare-mask
// selection, to pick the correct test (nonzero vs. even-odd) once the path IS routed
// through fillComplexPath()'s isolated stencil-then-cover technique.
static bool isComplexFillPath (const jam::Array<jam::Array<juce::Point<float>>>& polygon)
{
    return polygon.size() > 1;
}

/*____________________________________________________________________________*/
// appendFanTriangulatedRing — fan-expands a single device-space ring (closed polygon
// boundary, requires at least 3 points) from its first point into a triangle list,
// appending vertices + indices exactly like appendEarcutTriangulatedRing(). Unlike
// earcut, the fan's triangle SHAPE is irrelevant here — the isolated scratch
// stencil pass derives correctness from each triangle's signed winding-number
// contribution (INCR_WRAP/DECR_WRAP per front/back facing), not from the fan
// covering the ring's true interior. Works identically for convex, holed, or
// self-intersecting rings. TRIANGLE_LIST (not vk::PrimitiveTopology::eTriangleFan,
// which MoltenVK/Metal do not support natively) — CPU-side fan expansion instead.
static void appendFanTriangulatedRing (const jam::Array<juce::Point<float>>& ring,
                                        jam::Array<juce::Point<float>>& outVertices,
                                        jam::Array<uint32_t>& outIndices)
{
    if (ring.size() >= 3)
    {
        const auto baseVertex { static_cast<uint32_t> (outVertices.size()) };

        for (int i = 1; i + 1 < ring.size(); ++i)
        {
            outIndices.add (baseVertex);
            outIndices.add (baseVertex + static_cast<uint32_t> (i));
            outIndices.add (baseVertex + static_cast<uint32_t> (i + 1));
        }

        for (const auto& vertex : ring)
            outVertices.add (vertex);
    }
}

/*____________________________________________________________________________*/
// triangulateFanPath — appendFanTriangulatedRing() applied to every ring of an
// already-flattened polygon, mirroring triangulatePath()'s exact shape.
static void triangulateFanPath (const jam::Array<jam::Array<juce::Point<float>>>& polygon,
                                  jam::Array<juce::Point<float>>& outVertices,
                                  jam::Array<uint32_t>& outIndices)
{
    for (const auto& ring : polygon)
        appendFanTriangulatedRing (ring, outVertices, outIndices);
}

/*____________________________________________________________________________*/
// appendTriangulatedPathToFrameBuffer — appends triangulatePath()'s output to the
// path VulkanFrameBuffer. Caller has already grown the VulkanFrameBuffer's capacity to fit
// `vertices`/`indices`. Shared by fillPath and clipToPath — both upload triangulated
// geometry identically, differing only in which pipeline subsequently draws it.
VulkanLowLevelGraphicsContext::PackedPathRange VulkanLowLevelGraphicsContext::appendTriangulatedPathToFrameBuffer (
    const jam::Array<juce::Point<float>>& vertices, const jam::Array<uint32_t>& indices)
{
    // juce::Point<float> is layout-compatible with the raw vec2 float pair the path
    // VulkanFrameBuffer's vertex buffer expects (two trivially-copyable floats, no vtable,
    // no compiler-inserted padding) — this guard verifies that assumption before the
    // memcpy below reinterprets the vector's storage as a flat float buffer.
    static_assert (sizeof (juce::Point<float>) == 2 * sizeof (float),
                    "juce::Point<float> must be layout-compatible with a raw vec2 float pair");

    const int vertexCount { vertices.size() };
    const int indexCount  { indices.size() };

    auto& frameBuffer { context.getPathFrameBuffer() };

    auto* vertexDst { static_cast<float*> (frameBuffer.getVertexMapped()) + frameBuffer.getUsedVertices() * 2 };
    std::memcpy (vertexDst, vertices.data(), static_cast<size_t> (vertexCount) * sizeof (juce::Point<float>));

    auto* indexDst { static_cast<uint32_t*> (frameBuffer.getIndexMapped()) + frameBuffer.getUsedIndices() };
    std::memcpy (indexDst, indices.data(), static_cast<size_t> (indexCount) * sizeof (uint32_t));

    const PackedPathRange range { frameBuffer.getUsedIndices(), frameBuffer.getUsedVertices() };

    frameBuffer.setUsedVertices (frameBuffer.getUsedVertices() + vertexCount);
    frameBuffer.setUsedIndices  (frameBuffer.getUsedIndices()  + indexCount);

    return range;
}

/*____________________________________________________________________________*/
// bindPathIndexedDrawState — binds the projection descriptor set (slot 3), the
// bindless texture array (slot 1 — bound to satisfy the shared pipeline layout's
// fixed four-set count, unread by fill_rect.frag itself), and the path
// VulkanFrameBuffer's vertex + index buffers. Shared by fillPath, clipToPath,
// and the windingStencilAccumulate draw — every triList-family draw issued against
// triangulatePath()'s/triangulateFanPath()'s output.
void VulkanLowLevelGraphicsContext::bindPathIndexedDrawState (vk::CommandBuffer cmd, vk::PipelineLayout layout) const
{
    vk::DescriptorSet projSet { context.getProjectionDescriptorSet() };
    vk::DescriptorSet imgSet { context.getBindlessTextureDescriptorSet() };
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 1, imgSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 3, projSet, nullptr);

    auto& frameBuffer { context.getPathFrameBuffer() };
    const vk::Buffer buffers[1] { frameBuffer.getVertexBuffer() };
    const vk::DeviceSize offsets[1] { 0 };
    cmd.bindVertexBuffers (0, buffers, offsets);
    cmd.bindIndexBuffer (frameBuffer.getIndexBuffer(), 0, vk::IndexType::eUint32);
}

/*____________________________________________________________________________*/
// recordFillPathDrawCommands — pushes the current fill colour, selects the
// tri-list pipeline (alpha/opaque × stencil/no-stencil), and issues the indexed
// draw for fillPath()'s triangulated geometry.
void VulkanLowLevelGraphicsContext::recordFillPathDrawCommands (const PackedPathRange& range, int indexCount)
{
    updateProjection();

    vk::CommandBuffer cmd { context.getCommandBuffer() };
    vk::PipelineLayout layout { context.getPipelines().getLayout() };

    bindPathIndexedDrawState (cmd, layout);

    const auto fillColour { currentState.fill.colour };

    VulkanImagePushConstants pc {};
    pc.color = VulkanColour::fromHex (fillColour);
    pc.color.a *= currentState.opacity;
    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof (pc), &pc);

    // Select pipeline based on alpha + active stencil clip — tri-list topology matches earcut indexed output.
    const bool useAlpha { fillColour.getFloatAlpha() < 1.0f or currentState.opacity < 1.0f };
    const auto pipelineId { currentState.stencilClipDepth > 0
        ? (useAlpha ? VulkanPipelines::ID::alphaBlendTriListStencil : VulkanPipelines::ID::opaqueTriListStencil)
        : (useAlpha ? VulkanPipelines::ID::alphaBlendTriList         : VulkanPipelines::ID::opaqueTriList) };

    setStencilTestStateIfActive (cmd);
    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, context.getPipelines()[pipelineId]);
    setViewportAndScissor (cmd);

    // vertexBase offsets earcut's 0-based indices to the correct position in the shared VB.
    cmd.drawIndexed (static_cast<uint32_t> (indexCount), 1,
                     static_cast<uint32_t> (range.firstIndex),
                     static_cast<int32_t> (range.vertexBase), 0);
}

/*____________________________________________________________________________*/
// fillPath — triangulatePath() flattens beziers through the composed transform and
// earcut-triangulates each sub-path into indexed triangles (outer boundary + optional
// holes). Vertices are vec2 position — same input layout as the quad VB pipeline.
//
// The explicit `transform` parameter is composed with currentTransform before being
// passed to PathFlatteningIterator, so vertices land in device space — isomorphic to
// CoreGraphics applying the CTM in createPath() and D2D pathToPathGeometry(). The MVP
// storage buffer holds only orthoProjection.
void VulkanLowLevelGraphicsContext::fillPath (const juce::Path& path,
                                         const juce::AffineTransform& transform)
{
    if (currentState.fill.isColour())
    {
        const auto fullTransform { currentState.currentTransform.getTransformWith (transform) };
        const auto polygon { flattenPathIntoSubPaths (path, fullTransform) };

        if (isComplexFillPath (polygon))
        {
            const auto deviceBounds { path.getBounds().transformedBy (fullTransform) };
            fillComplexPath (path, polygon, deviceBounds);
        }
        else
        {
            jam::Array<juce::Point<float>> allVertices;
            jam::Array<uint32_t> allIndices;
            triangulatePath (polygon, allVertices, allIndices);

            if (not allIndices.isEmpty())
            {
                auto& frameBuffer { context.getPathFrameBuffer() };

                frameBuffer.reserve (frameBuffer.getUsedVertices() + allVertices.size(),
                                     frameBuffer.getUsedIndices()  + allIndices.size());

                const auto range { appendTriangulatedPathToFrameBuffer (allVertices, allIndices) };
                recordFillPathDrawCommands (range, allIndices.size());
            }
        }
    }
}

/*____________________________________________________________________________*/
// recordClipToPathDrawCommands — binds the stencilWriteTriList pipeline and issues
// the indexed draw that writes currentState.stencilClipDepth into the stencil buffer
// (no colour output) for clipToPath()'s triangulated geometry.
void VulkanLowLevelGraphicsContext::recordClipToPathDrawCommands (const PackedPathRange& range, int indexCount)
{
    // Vertices are already in device space — orthoProjection maps directly to NDC.
    std::memcpy (context.getProjectionMappedPtr(), &orthoProjection, sizeof (orthoProjection));

    vk::CommandBuffer cmd { context.getCommandBuffer() };
    vk::PipelineLayout layout { context.getPipelines().getLayout() };

    bindPathIndexedDrawState (cmd, layout);

    // stencilWriteTriList — triangle list topology matches earcut indexed output.
    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, context.getPipelines()[VulkanPipelines::ID::stencilWriteTriList]);

    // The write draw's stencil EQUAL test gates on the PARENT depth
    // (stencilClipDepth - 1, computed in signed int before the cast, so depth
    // 0->1's first-ever write compares against 0, matching the LOAD_OP_CLEAR
    // stencil buffer). Content draws still bake/test the NEW depth via
    // record.stencilRef / setStencilTestStateIfActive — unchanged.
    cmd.setStencilReference  (vk::StencilFaceFlagBits::eFrontAndBack, static_cast<uint32_t> (currentState.stencilClipDepth - 1));
    cmd.setStencilWriteMask  (vk::StencilFaceFlagBits::eFrontAndBack, 0xFF);
    cmd.setStencilCompareMask (vk::StencilFaceFlagBits::eFrontAndBack, 0xFF);

    // Stencil-only write draw — no colour output, so the pushed colour stays zero-init.
    VulkanImagePushConstants pc {};
    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof (pc), &pc);

    setViewportAndScissor (cmd);

    cmd.drawIndexed (static_cast<uint32_t> (indexCount), 1,
                     static_cast<uint32_t> (range.firstIndex),
                     static_cast<int32_t> (range.vertexBase), 0);
}

/*____________________________________________________________________________*/
// clipToPath — triangulatePath() tessellates the path into stencil geometry using
// earcut, then draws it into the stencil buffer (no color output). Subsequent draw
// calls gate on the written stencil reference value, masking output to the path shape.
//
// Vertices are pre-transformed to device space by triangulatePath() via deviceTransform
// before upload. The MVP uploaded to the storage buffer is orthoProjection only (identity model)
// since vertices are already in device space. Multiple clipToPath calls per frame each
// occupy a distinct region of the path VulkanFrameBuffer (append, not overwrite — same reason
// as fillPath), and the clip region is additionally intersected with the path's device-space
// bounding box.
void VulkanLowLevelGraphicsContext::clipToPath (const juce::Path& path,
                                           const juce::AffineTransform& transform)
{
    const auto deviceTransform { currentState.currentTransform.getTransformWith (transform) };
    const auto polygon { flattenPathIntoSubPaths (path, deviceTransform) };

    jam::Array<juce::Point<float>> allVertices;
    jam::Array<uint32_t> allIndices;
    triangulatePath (polygon, allVertices, allIndices);

    if (not allIndices.isEmpty())
    {
        auto& frameBuffer { context.getPathFrameBuffer() };

        frameBuffer.reserve (frameBuffer.getUsedVertices() + allVertices.size(),
                             frameBuffer.getUsedIndices()  + allIndices.size());

        const auto range { appendTriangulatedPathToFrameBuffer (allVertices, allIndices) };

        ++currentState.stencilClipDepth;
        recordClipToPathDrawCommands (range, allIndices.size());

        const auto pathBounds { path.getBounds()
                                    .transformedBy (deviceTransform)
                                    .getSmallestIntegerContainer() };
        currentState.deviceSpaceClipList.clipTo (pathBounds);
    }
}

/*____________________________________________________________________________*/
// fillComplexPath — the complex-path branch: multiple subpaths
// (holes) or an explicit even-odd winding rule. Fan-expands every ring (NOT earcut —
// see appendFanTriangulatedRing()) into the path VulkanFrameBuffer, then hands off to
// recordWindingAccumulateAndCoverDrawCommands() (isolated scratch stencil pass) and
// recordWindingCompositeDrawCommands() (composite back onto the active target).
void VulkanLowLevelGraphicsContext::fillComplexPath (const juce::Path& path,
                                                const jam::Array<jam::Array<juce::Point<float>>>& polygon,
                                                juce::Rectangle<float> deviceBounds)
{
    const auto deviceBoundsInt { deviceBounds.getSmallestIntegerContainer() };
    const vk::Extent2D scratchExtent {
        static_cast<uint32_t> (juce::jmax (1, deviceBoundsInt.getWidth())),
        static_cast<uint32_t> (juce::jmax (1, deviceBoundsInt.getHeight()))
    };

    context.getOrCreateWindingScratch (scratchExtent);

    jam::Array<juce::Point<float>> allVertices;
    jam::Array<uint32_t> allIndices;
    triangulateFanPath (polygon, allVertices, allIndices);

    if (not allIndices.isEmpty())
    {
        auto& frameBuffer { context.getPathFrameBuffer() };

        frameBuffer.reserve (frameBuffer.getUsedVertices() + allVertices.size(),
                             frameBuffer.getUsedIndices()  + allIndices.size());

        const auto range { appendTriangulatedPathToFrameBuffer (allVertices, allIndices) };

        recordWindingAccumulateAndCoverDrawCommands (path, range, allIndices.size(),
                                                      deviceBounds, scratchExtent);
        recordWindingCompositeDrawCommands (deviceBounds, scratchExtent);
    }
}

/*____________________________________________________________________________*/
// recordWindingAccumulateAndCoverDrawCommands — begins the isolated scratch render
// pass (reuses VulkanGraphics' main CLEAR-op render pass against VulkanWindingScratch's own
// framebuffer), issues the windingStencilAccumulate draw followed by the
// windingCoverOpaque draw within the SAME subpass, then ends the scratch pass.
void VulkanLowLevelGraphicsContext::recordWindingAccumulateAndCoverDrawCommands (
    const juce::Path& path, const PackedPathRange& range, int indexCount,
    juce::Rectangle<float> deviceBounds, vk::Extent2D scratchExtent)
{
    auto& scratch { context.getWindingScratch() };
    const vk::Extent2D capacityExtent { scratch.getCapacityExtent() };

    // Vertices are already in device space — orthoProjection maps directly to NDC
    // (matches recordFillPathDrawCommands/recordClipToPathDrawCommands's own leading call).
    updateProjection();

    // Ends whatever render pass is currently active (the main swapchain pass, in the
    // common case) — mirrors beginTransparencyLayer's exact call before starting its
    // own offscreen pass.
    context.endRenderPass();

    vk::CommandBuffer cmd { context.getCommandBuffer() };

    // VulkanGraphics::beginRenderPass(vk::Framebuffer, vk::Extent2D) SSOT (jam_VulkanGraphics.cpp)
    // — same CLEAR-op render pass object as the main scene, targeting the scratch
    // framebuffer/extent instead of sceneFramebuffer/swapchainExtent. Also marks
    // VulkanGraphics::renderPassActive true, so the vk::CommandBuffer::endRenderPass call below is replaced
    // with context.endRenderPass() to keep the flag symmetric — mirrors
    // beginOffscreenTransparencyRenderPass()/endTransparencyLayer()'s established
    // convention for the offscreen transparency target.
    context.beginRenderPass (scratch.getFramebuffer(), capacityExtent);

    // Viewport shift — every vertex/record stays in the SAME full-window device-space
    // coordinates as any other draw in this LLGC (triangulateFanPath()'s ring points,
    // deviceBounds itself); ONLY the viewport's origin moves so that window device-space
    // position deviceBounds.getPosition() lands at the scratch framebuffer's pixel (0,0).
    // Avoids swapping the shared projection storage buffer mid-frame (orthoProjection never changes
    // within a frame elsewhere in this engine — see updateProjection()'s doc comment).
    vk::Viewport scratchViewport {};
    scratchViewport.x        = -deviceBounds.getX();
    scratchViewport.y        = -deviceBounds.getY();
    scratchViewport.width    = static_cast<float> (context.getSwapchainExtent().width);
    scratchViewport.height   = static_cast<float> (context.getSwapchainExtent().height);
    scratchViewport.minDepth = 0.0f;
    scratchViewport.maxDepth = 1.0f;

    const vk::Rect2D scratchScissor { { 0, 0 }, scratchExtent };

    vk::PipelineLayout layout { context.getPipelines().getLayout() };

    // --- windingStencilAccumulate: per-subpath fan, INCR_WRAP/DECR_WRAP, colorWriteMask=0 ---
    bindPathIndexedDrawState (cmd, layout);

    // VulkanColour output is discarded (colorWriteMask=0, see stagesTriList's doc
    // comment in jam_VulkanPipelines.cpp) — pushed colour stays zero-init.
    VulkanImagePushConstants pc {};
    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof (pc), &pc);

    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, context.getPipelines()[VulkanPipelines::ID::windingStencilAccumulate]);
    cmd.setViewport (0, scratchViewport);
    cmd.setScissor (0, scratchScissor);
    cmd.setStencilCompareMask (vk::StencilFaceFlagBits::eFrontAndBack, 0xFF);
    cmd.setStencilWriteMask  (vk::StencilFaceFlagBits::eFrontAndBack, 0xFF);
    cmd.setStencilReference  (vk::StencilFaceFlagBits::eFrontAndBack, 0);

    cmd.drawIndexed (static_cast<uint32_t> (indexCount), 1,
                     static_cast<uint32_t> (range.firstIndex),
                     static_cast<int32_t> (range.vertexBase), 0);

    // --- windingCoverOpaque: bbox quad, stencil-tested against the value just
    // written above — same subpass, no barrier needed (fixed-function depth/stencil
    // read-after-write ordering between sequential draws targeting the same attachment
    // within one subpass, exactly the guarantee clipToPath's own stencil write already
    // relies on for its later content draws). Always Blend::opaque (blend disabled,
    // direct write) — NOT a forced-alpha-1 write, the fill color's real alpha is still
    // written into the scratch image's color channel correctly. The scratch target is
    // LOAD_OP_CLEAR'd to transparent black immediately before this single draw (never
    // more than one cover draw accumulates into it per fillComplexPath() call), so
    // alpha-blending this draw into an always-empty target would apply the fill alpha
    // here AND a second time at recordWindingCompositeDrawCommands()'s later composite
    // blend — squaring it. Opaque write + single later composite blend applies the
    // alpha exactly once. ---
    auto& recordBuffer { context.getPrimitiveRecordBuffer() };

    context.reservePrimitiveRecords (recordBuffer.getUsedRecords() + 1);

    const int coverRecordIndex { appendPrimitiveRecord (deviceBounds, { 0.0f, 0.0f, 1.0f, 1.0f },
                                                          currentState.fill.colour, currentState.opacity, 0u) };

    vk::DescriptorSet projSet { context.getProjectionDescriptorSet() };
    vk::DescriptorSet imgSet { context.getBindlessTextureDescriptorSet() };
    vk::DescriptorSet recordSet { context.getRecordDescriptorSet() };
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 1, imgSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 2, recordSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 3, projSet, nullptr);

    auto coverColour { VulkanColour::fromHex (currentState.fill.colour) };
    coverColour.a *= currentState.opacity;
    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof (coverColour), &coverColour);

    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, context.getPipelines()[VulkanPipelines::ID::windingCoverOpaque]);
    cmd.setViewport (0, scratchViewport);
    cmd.setScissor (0, scratchScissor);

    // Nonzero: any nonzero accumulated value is "inside" (compareMask 0xFF tests the
    // full byte). Even-odd: INCR_WRAP/DECR_WRAP accumulate modulo 256 (wraparound IS
    // modular arithmetic), and 256 is even, so the accumulated value's LSB always
    // equals the true winding number's parity regardless of sign or wraparound —
    // masking to bit 0 (compareMask 0x01) directly yields the even-odd test from the
    // SAME accumulated value; no second accumulate pass or bit-INVERT trick needed.
    const uint32_t compareMask { path.isUsingNonZeroWinding() ? 0xFFu : 0x01u };
    cmd.setStencilCompareMask (vk::StencilFaceFlagBits::eFrontAndBack, compareMask);
    cmd.setStencilWriteMask  (vk::StencilFaceFlagBits::eFrontAndBack, 0x00);
    cmd.setStencilReference  (vk::StencilFaceFlagBits::eFrontAndBack, 0);

    cmd.draw (4, 1, 0, static_cast<uint32_t> (coverRecordIndex));

    // context.endRenderPass() — not a raw vk::CommandBuffer::endRenderPass call — so renderPassActive
    // stays correct for the offscreen scratch pass just closed, matching this
    // function's begin side above.
    context.endRenderPass();
}

/*____________________________________________________________________________*/
// recordWindingCompositeDrawCommands — transitions the finished scratch color image
// for sampling, resumes the correct render target (main scene, or the enclosing
// transparency layer if nested — see resumeRenderPassForNestingLevel()) with LOAD op,
// and composites it back as a textured quad via the existing imageInstanced/
// imageInstancedStencil pipeline — gated by the OUTER active clip exactly like any
// other content draw.
void VulkanLowLevelGraphicsContext::recordWindingCompositeDrawCommands (juce::Rectangle<float> deviceBounds,
                                                                    vk::Extent2D scratchExtent)
{
    auto& scratch { context.getWindingScratch() };

    jassert (scratch.getBindlessIndex() >= 0);

    // Transition — mirrors transitionTransparencyLayerForSampling exactly,
    // substituting the scratch RESOLVE image (scratch.getColorImage()
    // is the MSAA attachment and can never be sampled directly). The shared
    // renderPass's attachment 2 (resolve) already declares finalLayout =
    // SHADER_READ_ONLY_OPTIMAL (no longer PRESENT_SRC_KHR — that layout belonged
    // to the earlier swapchain-direct main pass); this barrier is now a
    // same-layout memory dependency, not an actual transition. Collapsed
    // via recordImageMemoryBarrier() (jam_VulkanUploadHelpers.h).
    recordImageMemoryBarrier (context.getCommandBuffer(), scratch.getResolveImage(),
                              vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader);

    // Resumes with LOAD op via the shared resumeRenderPassForNestingLevel() SSOT
    // (jam_VulkanLowLevelGraphicsContextTransparency.cpp) — the main scene in the
    // common case, or the enclosing transparency layer's own offscreen framebuffer
    // if this fillComplexPath() call happened while nested inside
    // beginTransparencyLayer(); exactly mirrors endTransparencyLayer()'s own resume.
    resumeRenderPassForNestingLevel (transparencyNestingLevel);
    updateProjection();

    auto& recordBuffer { context.getPrimitiveRecordBuffer() };

    context.reservePrimitiveRecords (recordBuffer.getUsedRecords() + 1);

    // uvRect scopes sampling to the scratch target's currently-used sub-rect —
    // monotonic growth (VulkanWindingScratch::reserve) means the underlying
    // image may be LARGER than this call's scratchExtent.
    const vk::Extent2D capacityExtent { scratch.getCapacityExtent() };
    const juce::Rectangle<float> uvRect {
        0.0f, 0.0f,
        static_cast<float> (scratchExtent.width)  / static_cast<float> (capacityExtent.width),
        static_cast<float> (scratchExtent.height) / static_cast<float> (capacityExtent.height)
    };

    // Opacity was already applied inside the cover pass's push-constant colour
    // (recordWindingAccumulateAndCoverDrawCommands) — this composite must not
    // re-apply it, mirroring compositeTransparencyLayer applying layerOpacity
    // exactly ONCE, at its own single composite point.
    const int recordIndex { appendPrimitiveRecord (deviceBounds, uvRect, juce::Colours::white, 1.0f,
                                                     static_cast<uint32_t> (scratch.getBindlessIndex())) };

    vk::CommandBuffer cmd { context.getCommandBuffer() };
    vk::PipelineLayout layout { context.getPipelines().getLayout() };
    vk::DescriptorSet projSet { context.getProjectionDescriptorSet() };
    vk::DescriptorSet recordSet { context.getRecordDescriptorSet() };
    vk::DescriptorSet imgSet { context.getBindlessTextureDescriptorSet() };

    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 1, imgSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 2, recordSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 3, projSet, nullptr);

    const auto pc { makeImagePushConstants (1.0f) };
    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof (pc), &pc);

    // Gated by the OUTER active clip exactly like any other content draw — the
    // scratch target's own stencil bits never participate here (an active
    // clipToPath around a complex fillPath still intersects correctly, since this
    // draw tests the MAIN stencil, same as recordImageDrawCommands).
    const auto imageId { currentState.stencilClipDepth > 0
        ? VulkanPipelines::ID::imageInstancedStencil
        : VulkanPipelines::ID::imageInstanced };

    setStencilTestStateIfActive (cmd);
    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, context.getPipelines()[imageId]);
    setViewportAndScissor (cmd);

    cmd.draw (4, 1, 0, static_cast<uint32_t> (recordIndex));
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
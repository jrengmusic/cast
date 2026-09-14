//
// Core file: shared push-constant/viewport/stencil helpers, projection/scissor
// utilities, and the rect-fill draw path. VulkanImage draw, path-fill/clip,
// transparency-layer, and glyph draw paths live in their own .cpp files — see
// jam_vulkan.cpp for the full list of sibling translation units making up this
// one class.

namespace jam
{
/*____________________________________________________________________________*/
// makeImagePushConstants — composes `opacity` explicitly into the image push
// constant layout's colour alpha channel (colour is always opaque white — the
// image's own pixels carry colour). VulkanImagePushConstants itself is declared in
// jam_VulkanGraphics.h (not file-local here) so VulkanGraphics::recordSceneCompositeDrawCommands()
// can also call this — see that header's doc comment for the cross-TU-ordering
// reason. This function therefore has external linkage (no `static`), matching
// its header declaration.
VulkanImagePushConstants makeImagePushConstants (float opacity) noexcept
{
    VulkanImagePushConstants pc {};
    pc.color = VulkanColour::fromHex (juce::Colours::white);
    pc.color.a *= opacity;
    return pc;
}

/*____________________________________________________________________________*/
std::unique_ptr<juce::ImageType> VulkanLowLevelGraphicsContext::getPreferredImageTypeForTemporaryImages() const
{
    return std::make_unique<jam::VulkanImageType>();
}

/*____________________________________________________________________________*/
// setFullViewport — sets a viewport spanning the full render target extent.
// Shared by every draw method in this class's split .cpp files.
static void setFullViewport (vk::CommandBuffer cmd, vk::Extent2D extent)
{
    vk::Viewport viewport {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float> (extent.width);
    viewport.height   = static_cast<float> (extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    cmd.setViewport (0, viewport);
}

/*____________________________________________________________________________*/
// updateProjection — upload orthoProjection to the projection storage buffer.
//
// orthoProjection is fixed at construction time (device pixels → NDC).
// Vertices are pre-transformed to device space on CPU by each draw method,
// so the projection storage buffer holds only the fixed ortho matrix — identical for all draws in a frame.
// Overwriting is harmless because orthoProjection never changes within a frame.
void VulkanLowLevelGraphicsContext::updateProjection()
{
    std::memcpy (context.getProjectionMappedPtr(), &orthoProjection, sizeof (orthoProjection));
}

/*____________________________________________________________________________*/
// computeScissor — converts deviceSpaceClipList bounds to a vk::Rect2D clamped to
// the swapchain extent.
//
// deviceSpaceClipList is maintained in physical pixels by the clip* overrides in
// the header, so no further transform is required here.
vk::Rect2D VulkanLowLevelGraphicsContext::computeScissor() const
{
    const auto clipBounds { currentState.deviceSpaceClipList.getBounds() };
    const vk::Extent2D extent { context.getSwapchainExtent() };

    vk::Rect2D scissor {};
    scissor.offset.x      = juce::jmax (0, clipBounds.getX());
    scissor.offset.y      = juce::jmax (0, clipBounds.getY());
    scissor.extent.width  = static_cast<uint32_t> (juce::jmin (clipBounds.getWidth(),
                                static_cast<int> (extent.width)  - scissor.offset.x));
    scissor.extent.height = static_cast<uint32_t> (juce::jmin (clipBounds.getHeight(),
                                static_cast<int> (extent.height) - scissor.offset.y));

    return scissor;
}

/*____________________________________________________________________________*/
// setViewportAndScissor — sets the full-target viewport and the current clip
// region as the scissor rect. Shared by issueRectFill, drawImage, fillPath,
// and clipToPath — every draw method that issues geometry against the active clip.
void VulkanLowLevelGraphicsContext::setViewportAndScissor (vk::CommandBuffer cmd) const
{
    const vk::Extent2D extent { context.getSwapchainExtent() };
    setFullViewport (cmd, extent);

    const vk::Rect2D scissor { computeScissor() };
    cmd.setScissor (0, scissor);
}

/*____________________________________________________________________________*/
// setStencilTestStateIfActive — sets the stencil compare/write masks and
// reference for stencil-gated test-only draws, when a stencil clip is active.
void VulkanLowLevelGraphicsContext::setStencilTestStateIfActive (vk::CommandBuffer cmd) const
{
    if (currentState.stencilClipDepth > 0)
    {
        cmd.setStencilCompareMask (vk::StencilFaceFlagBits::eFrontAndBack, 0xFF);
        cmd.setStencilWriteMask   (vk::StencilFaceFlagBits::eFrontAndBack, 0x00);
        cmd.setStencilReference   (vk::StencilFaceFlagBits::eFrontAndBack, static_cast<uint32_t> (currentState.stencilClipDepth));
    }
}

/*____________________________________________________________________________*/
// appendPrimitiveRecord — appends one VulkanPrimitiveRecord to the per-frame SSBO
// (VulkanPrimitiveRecordBuffer). Shared by rect fill, image draw, and clipToImageAlpha —
// every unified quad primitive writes its geometry/colour/uv this same way, differing
// only in which uvRect/colour they supply and which pipeline subsequently draws them.
// Caller has already ensured capacity via VulkanGraphics::reservePrimitiveRecords().
int VulkanLowLevelGraphicsContext::appendPrimitiveRecord (juce::Rectangle<float> deviceBounds,
                                                     juce::Rectangle<float> uvRect,
                                                     juce::Colour colour,
                                                     float opacity,
                                                     uint32_t textureIndex)
{
    auto& recordBuffer { context.getPrimitiveRecordBuffer() };
    auto* records { static_cast<VulkanPrimitiveRecord*> (recordBuffer.getMapped()) };

    const int recordIndex { recordBuffer.getUsedRecords() };
    jassert (recordIndex < recordBuffer.getCapacityRecords());
    VulkanPrimitiveRecord& record { records[recordIndex] };
    record.position.x = deviceBounds.getX();
    record.position.y = deviceBounds.getY();
    record.size = jam::Size<float> { deviceBounds.getWidth(), deviceBounds.getHeight() };
    record.uvRect = uvRect;
    record.color = VulkanColour::fromHex (colour);
    record.color.a *= opacity;
    // clip/flags remain dormant until the winding-rule work wires them up — mirrors
    // the earlier unified-vertex-pulling design's push-constant clip channel, which
    // was likewise declared but always zero. textureIndex is caller-supplied —
    // untextured rect fills never read it (fill_rect.frag doesn't bind set 1).
    // stencilRef is baked from currentState.stencilClipDepth AT THIS RECORD's emit
    // time — it can never desync from State's save/restore-covered depth.
    record.clip = juce::Rectangle<int> {};
    record.textureIndex = textureIndex;
    record.stencilRef = static_cast<uint32_t> (currentState.stencilClipDepth);
    record.flags = 0;
    record.maskTextureIndex = currentState.maskTextureIndex;

    recordBuffer.setUsedRecords (recordIndex + 1);
    return recordIndex;
}

/*____________________________________________________________________________*/
// appendRectFillRecord — transforms `bounds` from logical to device space on CPU,
// then appends one VulkanPrimitiveRecord via appendPrimitiveRecord(). Rect fills are
// untextured, so uvRect is the {0,0,1,1} placeholder and textureIndex is 0
// (unread by fill_rect.frag — those pipelines never bind set 1).
int VulkanLowLevelGraphicsContext::appendRectFillRecord (juce::Rectangle<float> bounds,
                                                    juce::Colour fillColour,
                                                    float opacity)
{
    const auto deviceRect { bounds.transformedBy (currentState.currentTransform.getTransform()) };
    return appendPrimitiveRecord (deviceRect, { 0.0f, 0.0f, 1.0f, 1.0f }, fillColour, opacity, 0u);
}

/*____________________________________________________________________________*/
// recordRectFillDrawCommands — binds descriptor/record state, pushes colour
// (opacity pre-multiplied into its alpha channel inline here — matches the
// glyph emit site's compose pattern at jam_VulkanLowLevelGraphicsContextGlyph.cpp:107-108 —
// fill_rect.frag reads color.a directly, no separate opacity
// field), selects the rect pipeline (alpha/opaque × stencil/no-stencil), and
// issues the instanced draw (4 vertices, 1 instance, gl_InstanceIndex = recordIndex).
// Set 1 (bindless texture array) is bound unconditionally to satisfy the
// shared pipeline layout's fixed three-set count — fill_rect.frag itself never
// samples it (untextured rect fills are colour-only).
void VulkanLowLevelGraphicsContext::recordRectFillDrawCommands (int recordIndex, juce::Colour fillColour, float opacity)
{
    updateProjection();

    vk::CommandBuffer cmd { context.getCommandBuffer() };
    vk::PipelineLayout layout { context.getPipelines().getLayout() };
    vk::DescriptorSet projSet { context.getProjectionDescriptorSet() };
    vk::DescriptorSet recordSet { context.getRecordDescriptorSet() };
    vk::DescriptorSet imgSet { context.getBindlessTextureDescriptorSet() };

    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 1, imgSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 2, recordSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 3, projSet, nullptr);

    auto colour { VulkanColour::fromHex (fillColour) };
    colour.a *= opacity;
    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof (colour), &colour);

    const bool useAlpha { fillColour.getFloatAlpha() < 1.0f or opacity < 1.0f };
    const auto pipelineId { currentState.stencilClipDepth > 0
        ? (useAlpha ? VulkanPipelines::ID::alphaBlendRectInstancedStencil : VulkanPipelines::ID::opaqueRectInstancedStencil)
        : (useAlpha ? VulkanPipelines::ID::alphaBlendRectInstanced         : VulkanPipelines::ID::opaqueRectInstanced) };

    setStencilTestStateIfActive (cmd);
    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, context.getPipelines()[pipelineId]);
    setViewportAndScissor (cmd);

    cmd.draw (4, 1, 0, static_cast<uint32_t> (recordIndex));
}

/*____________________________________________________________________________*/
// issueRectFill — native Vulkan rect draw.
//
// `bounds` is in logical (user-space) coordinates; appendRectFillRecord transforms
// it to device space on CPU before appending the VulkanPrimitiveRecord. The MVP (ortho
// only) then maps device-space pixels to NDC — isomorphic to how CoreGraphics
// applies the CTM before the GPU sees any coordinates.
void VulkanLowLevelGraphicsContext::issueRectFill (juce::Rectangle<float> bounds,
                                              juce::Colour fillColour,
                                              float opacity)
{
    if (bounds.getWidth() > 0.0f and bounds.getHeight() > 0.0f)
    {
        auto& recordBuffer { context.getPrimitiveRecordBuffer() };

        context.reservePrimitiveRecords (recordBuffer.getUsedRecords() + 1);

        const int recordIndex { appendRectFillRecord (bounds, fillColour, opacity) };
        recordRectFillDrawCommands (recordIndex, fillColour, opacity);
    }
}

/*____________________________________________________________________________*/
// recordGradientFillDrawCommands — rebuilds lut/lutGradient/lutTransform only when
// the opacity-baked gradient or the composed transform changed (ColourGradient::
// operator==, AffineTransform::operator==), promotes lut through the same
// texture-cache path drawImage() uses, pushes VulkanGradientPushConstants,
// selects the gradientFill pipeline (Blend::alpha, stencil/no-stencil variant
// gated on currentState.stencilClipDepth — same selection shape as
// recordRectFillDrawCommands), and issues the instanced draw (same 4-vertex/
// 1-instance shape as recordRectFillDrawCommands).
//
// point1/point2 are recomputed from currentState's transform on every call — only
// the LUT's content/size is cached, keyed on both gradient content and the
// transform used to build the lookup table.
void VulkanLowLevelGraphicsContext::recordGradientFillDrawCommands (int recordIndex, const juce::ColourGradient& gradient, float opacity)
{
    updateProjection();

    const auto composedTransform { currentState.currentTransform.getTransformWith (currentState.fill.transform) };

    juce::ColourGradient candidate { gradient };
    candidate.multiplyOpacity (currentState.fill.colour.getFloatAlpha());

    if (not (candidate == lutGradient) or not (composedTransform == lutTransform))
    {
        juce::HeapBlock<juce::PixelARGB> table;
        const int tableSize { candidate.createLookupTable (composedTransform, table) };

        lut = juce::Image (juce::Image::ARGB, tableSize, 1, false, juce::SoftwareImageType());
        const juce::Image::BitmapData bmp { lut, juce::Image::BitmapData::writeOnly };
        std::memcpy (bmp.getLinePointer (0), table.getData(), static_cast<size_t> (tableSize) * sizeof (juce::PixelARGB));

        lutGradient = candidate;
        lutTransform = composedTransform;
    }

    context.cacheImageTexture (lut);
    const int rawIndex { context.getBindlessIndex (lut) };
    jassert (rawIndex >= 0);// residency invariant: lut just cached above

    const auto point1 { gradient.point1.transformedBy (composedTransform) };
    const auto point2 { gradient.point2.transformedBy (composedTransform) };

    jassert (point1 != point2);// degenerate gradient divides by zero in gradient_fill.frag

    vk::CommandBuffer cmd { context.getCommandBuffer() };
    vk::PipelineLayout layout { context.getPipelines().getLayout() };
    vk::DescriptorSet projSet { context.getProjectionDescriptorSet() };
    vk::DescriptorSet recordSet { context.getRecordDescriptorSet() };
    vk::DescriptorSet imgSet { context.getBindlessTextureDescriptorSet() };

    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 1, imgSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 2, recordSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 3, projSet, nullptr);

    VulkanGradientPushConstants pushConstants {};
    pushConstants.point1[0] = point1.x;
    pushConstants.point1[1] = point1.y;
    pushConstants.point2[0] = point2.x;
    pushConstants.point2[1] = point2.y;
    pushConstants.isRadial = gradient.isRadial ? 1 : 0;
    pushConstants.lutTextureIndex = static_cast<uint32_t> (rawIndex);
    pushConstants.opacity = opacity;

    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof (pushConstants), &pushConstants);

    const auto pipelineId { currentState.stencilClipDepth > 0
        ? VulkanPipelines::ID::gradientFillStencil
        : VulkanPipelines::ID::gradientFill };

    setStencilTestStateIfActive (cmd);
    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, context.getPipelines()[pipelineId]);
    setViewportAndScissor (cmd);

    cmd.draw (4, 1, 0, static_cast<uint32_t> (recordIndex));
}

/*____________________________________________________________________________*/
// issueGradientFill — native Vulkan gradient draw, mirrors issueRectFill().
//
// `bounds` is in logical (user-space) coordinates; appendRectFillRecord transforms
// it to device space on CPU before appending the VulkanPrimitiveRecord. The record's
// own colour is unconsumed by gradient_fill.frag (it samples the LUT via push
// constants instead) — gradient's own first stop is passed through for SSBO
// contract consistency with every other appendRectFillRecord() caller.
void VulkanLowLevelGraphicsContext::issueGradientFill (juce::Rectangle<float> bounds,
                                                  const juce::ColourGradient& gradient,
                                                  float opacity)
{
    if (bounds.getWidth() > 0.0f and bounds.getHeight() > 0.0f)
    {
        auto& recordBuffer { context.getPrimitiveRecordBuffer() };

        context.reservePrimitiveRecords (recordBuffer.getUsedRecords() + 1);

        const int recordIndex { appendRectFillRecord (bounds, gradient.getColour (0), opacity) };
        recordGradientFillDrawCommands (recordIndex, gradient, opacity);
    }
}

/*____________________________________________________________________________*/
// fillRect family — solid-colour path reads currentState.fill + currentState.opacity;
// gradient path reads currentState.fill.gradient (LUT-sampled via issueGradientFill).
// Tiled-image fills remain unimplemented.
//
// Bounds are passed in logical (user-space) coordinates. issueRectFill/issueGradientFill
// apply currentTransform to vertices on CPU before writing to the VB.

void VulkanLowLevelGraphicsContext::fillRect (const juce::Rectangle<int>& r,
                                         bool /*replaceExistingContents*/)
{
    if (currentState.fill.isColour())
        issueRectFill (r.toFloat(), currentState.fill.colour, currentState.opacity);
    else if (currentState.fill.isGradient())
        issueGradientFill (r.toFloat(), *currentState.fill.gradient, currentState.opacity);
}

void VulkanLowLevelGraphicsContext::fillRect (const juce::Rectangle<float>& r)
{
    if (currentState.fill.isColour())
        issueRectFill (r, currentState.fill.colour, currentState.opacity);
    else if (currentState.fill.isGradient())
        issueGradientFill (r, *currentState.fill.gradient, currentState.opacity);
}

void VulkanLowLevelGraphicsContext::fillRectList (const juce::RectangleList<float>& list)
{
    if (currentState.fill.isColour())
    {
        for (const auto& rect : list)
            issueRectFill (rect, currentState.fill.colour, currentState.opacity);
    }
    else if (currentState.fill.isGradient())
    {
        for (const auto& rect : list)
            issueGradientFill (rect, *currentState.fill.gradient, currentState.opacity);
    }
}

/*____________________________________________________________________________*/
// drawLine — delegates to drawLineWithThickness with 1px thickness.
void VulkanLowLevelGraphicsContext::drawLine (const juce::Line<float>& l)
{
    drawLineWithThickness (l, 1.0f);
}

/*____________________________________________________________________________*/
// drawLineWithThickness — expand the line into a quad (4 vertices, 6 indices)
// and draw via the triList pipeline. Zero heap allocation, zero earcut.
void VulkanLowLevelGraphicsContext::drawLineWithThickness (const juce::Line<float>& line, float lineThickness)
{
    static_assert (sizeof (juce::Point<float>) == 2 * sizeof (float),
                    "juce::Point<float> must be layout-compatible with a raw vec2 float pair");

    if (currentState.fill.isColour())
    {
        const auto start { line.getStart() };
        const auto end { line.getEnd() };
        const auto delta { end - start };
        const auto lengthSquared { delta.x * delta.x + delta.y * delta.y };

        if (not juce::approximatelyEqual (lengthSquared, 0.0f))
        {
            const auto halfThick { lineThickness * 0.5f / std::sqrt (lengthSquared) };
            const juce::Point<float> perp { -delta.y * halfThick, delta.x * halfThick };

            const auto transform { currentState.currentTransform.getTransform() };

            const juce::Point<float> vertices[4]
            {
                (start + perp).transformedBy (transform),
                (start - perp).transformedBy (transform),
                (end + perp).transformedBy (transform),
                (end - perp).transformedBy (transform)
            };

            const uint32_t indices[6] { 0, 1, 2, 2, 1, 3 };

            auto& frameBuffer { context.getPathFrameBuffer() };

            frameBuffer.reserve (frameBuffer.getUsedVertices() + 4,
                                 frameBuffer.getUsedIndices()  + 6);

            auto* vertexDst { static_cast<float*> (frameBuffer.getVertexMapped()) + frameBuffer.getUsedVertices() * 2 };
            std::memcpy (vertexDst, vertices, sizeof (vertices));

            auto* indexDst { static_cast<uint32_t*> (frameBuffer.getIndexMapped()) + frameBuffer.getUsedIndices() };
            std::memcpy (indexDst, indices, sizeof (indices));

            const PackedPathRange range { frameBuffer.getUsedIndices(), frameBuffer.getUsedVertices() };

            frameBuffer.setUsedVertices (frameBuffer.getUsedVertices() + 4);
            frameBuffer.setUsedIndices  (frameBuffer.getUsedIndices()  + 6);

            recordFillPathDrawCommands (range, 6);
        }
    }
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
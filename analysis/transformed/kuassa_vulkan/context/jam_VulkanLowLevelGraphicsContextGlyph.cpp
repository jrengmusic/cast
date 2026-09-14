//
// Glyph file: drawGlyphs(), uploadDirtyAtlasSlots(), and
// their helpers. Depends on setFullViewport()/computeScissor()/updateProjection()
// defined in jam_VulkanLowLevelGraphicsContext.cpp, included before this file in
// jam_vulkan.cpp.

namespace jam
{
/*____________________________________________________________________________*/
// uploadDirtyAtlasSlots — uploads any dirty atlas slots (mono and/or emoji) to
// their GPU images. Each dirty type is its own synchronous one-shot fenced
// submission via the engine-owned VulkanImageRenderer's beginCommands()/
// submitAndWait() seam (jam_VulkanTextureCache.h's identical
// calibrateSampleCount precedent) — clearDirty happens only after the fence
// wait returns, so the global dirty flag never races a second GPU timeline,
// and every upload owns a disjoint staging buffer, eliminating the prior
// shared-arena hazard entirely.
void VulkanLowLevelGraphicsContext::uploadDirtyAtlasSlots (jam::GlyphAtlas& atlas, int atlasDim)
{
    auto* engine { jam::VulkanEngine::getInstance() };
    jassert (engine != nullptr);

    auto* imageRenderer { &engine->getImageRenderer() };

    for (auto type : { jam::GlyphAtlas::Type::mono, jam::GlyphAtlas::Type::emoji })
    {
        if (atlas.isDirty (type))
        {
            juce::Image::BitmapData data (atlas.getAtlas (type), juce::Image::BitmapData::readOnly);
            const vk::DeviceSize stagingSize { static_cast<vk::DeviceSize> (data.lineStride)
                                             * static_cast<vk::DeviceSize> (atlasDim) };
            VulkanBuffer staging { jam::createStagingBuffer (imageRenderer->getDevice(), stagingSize) };
            jassert (staging.isValid());// staging allocation failure / VRAM exhaustion

            const vk::CommandBuffer cmd { imageRenderer->beginCommands() };
            atlas.getTexture (type).upload (cmd, staging.getBuffer(), staging.getMapped(), 0,
                                            data, atlasDim, atlasDim);
            const vk::Result uploadResult { imageRenderer->submitAndWait() };

            if (uploadResult == vk::Result::eSuccess)
            {
                atlas.clearDirty (type);
            }
            else
            {
                jam::debug::Log::write ("VulkanLowLevelGraphicsContext::uploadDirtyAtlasSlots: atlas upload failed:",
                                           vk::to_string (uploadResult));

                if (uploadResult == vk::Result::eErrorDeviceLost)
                {
                    juce::MessageManager::callAsync ([]
                    {
                        if (auto* reinitialiseEngine { jam::VulkanEngine::getInstance() })
                            reinitialiseEngine->reinitialiseDevice();
                    });
                }
            }
        }
    }
}

/*____________________________________________________________________________*/
// packGlyphQuadType — resolves every glyph in @p glyphs through
// GlyphAtlas::getOrRasterize() (cached, or rasterized on miss into the CPU-side
// atlas — dirty flags set on miss, re-uploaded by the caller via
// uploadDirtyAtlasSlots()), and writes a VulkanPrimitiveRecord directly into the
// record buffer for each glyph whose resolved region belongs to @p type
// (whitespace glyphs and glyphs belonging to the other atlas type are skipped),
// starting at the current record cursor and advancing it. Returns the instance
// range this type occupies.
//
// hasCellRun gates pendingCellRun's use on its count matching this call's own
// glyph count — a stale or mismatched setCellRun() call (or none at all, the
// plain-text case) falls back to the no-cell-run-context Key shape (zeroed
// codepoint/span/cellWidth/cellHeight/baseline) for every glyph, matching
// jam::LowLevelGraphicsGlyphRenderer::drawGlyphs()'s identical guard.
//
// textureIndex is the atlas type's stable bindless array slot, written once
// at context creation by VulkanGraphics::registerGlyphAtlasSlots(). Read from the atlas's own
// jam::VulkanBindlessTexture, keyed by context.getNativeHandle() — the slot
// is a registered slot assignment in that texture's own per-window registry,
// the atlas itself (and its VulkanBindlessTexture instances) is VulkanEngine-wide shared.
VulkanLowLevelGraphicsContext::InstanceRange VulkanLowLevelGraphicsContext::packGlyphQuadType (
    jam::GlyphAtlas::Type type, juce::Span<const uint16_t> glyphs, juce::Span<const juce::Point<float>> positions,
    juce::Typeface* typeface, const juce::AffineTransform& composedTransform, float rotation,
    jam::GlyphAtlas& atlas, VulkanPrimitiveRecord* records, int& recordCursor, int capacityRecords)
{
    const int firstInstance { recordCursor };

    const auto fontHeight { currentState.font.getHeight() };
    // currentState.opacity composed explicitly here (matches the rect-fill pattern
    // at jam_VulkanLowLevelGraphicsContext.cpp:143) — glyph fill previously read the
    // raw juce::Colour, dropping the context opacity multiplier entirely.
    auto colour { VulkanColour::fromHex (currentState.fill.colour) };
    colour.a *= currentState.opacity;
    // GlyphAtlas::Key::make() composes fontHeight * scale → DPI-correct atlas key
    // (physical pixel size on Retina/high-DPI), the one SSOT site also used by
    // jam::LowLevelGraphicsGlyphRenderer::drawGlyphs(). codepoint/cellWidth/
    // cellHeight/baseline/span thread setCellRun()'s per-run cell-run state.
    // rotation (drawGlyphs()'s composedTransform-derived angle) participates in
    // glyph identity — the atlas rasterizes the bitmap already rotated, so the
    // quad placement math below is unchanged.
    const float composedScale { jam::GlyphAtlas::Key::getScale (composedTransform) };
    const bool hasCellRun { pendingCellRun.count == static_cast<int> (glyphs.size()) };
    const int rawIndex { atlas.getTexture (type).getBindlessIndex (context.getNativeHandle()) };
    jassert (rawIndex >= 0);// atlas slot registered at context creation

    for (size_t i { 0 }; i < glyphs.size(); ++i)
    {
        jassert (i < positions.size());

        const char32_t codepoint  { hasCellRun ? pendingCellRun.codepoints[i] : char32_t { 0 } };
        const uint8_t  span       { hasCellRun ? pendingCellRun.spans[i]      : uint8_t { 0 } };
        const int      cellWidth  { hasCellRun ? pendingCellRun.cellWidth     : 0 };
        const int      cellHeight { hasCellRun ? pendingCellRun.cellHeight    : 0 };
        const int      baseline   { hasCellRun ? pendingCellRun.baseline      : 0 };

        auto key { jam::GlyphAtlas::Key::make (typeface, static_cast<uint32_t> (glyphs[i]), fontHeight, composedScale,
                                               codepoint, cellWidth, cellHeight, baseline, span, rotation) };
        auto* region { atlas.getOrRasterize (key) };

        if (region != nullptr and region->type == type)
        {
            const auto [regionWidth, regionHeight] { region->size };

            if (regionWidth > 0 and regionHeight > 0)
            {
                const auto devicePos { positions[i].transformedBy (composedTransform) };
                const auto& uv { region->textureCoordinates };

                jassert (recordCursor < capacityRecords);
                VulkanPrimitiveRecord& record { records[recordCursor] };
                record.position = juce::Point<float> (
                    static_cast<float> (juce::roundToInt (devicePos.x) + region->bearing.x),
                    static_cast<float> (juce::roundToInt (devicePos.y) - region->bearing.y));
                record.size = jam::Size<float> (static_cast<float> (regionWidth), static_cast<float> (regionHeight));
                record.uvRect = juce::Rectangle<float>::leftTopRightBottom (uv.getX(), uv.getY(), uv.getRight(), uv.getBottom());
                record.color = colour;
                record.clip = juce::Rectangle<int> {};
                record.textureIndex = static_cast<uint32_t> (rawIndex);
                record.stencilRef = static_cast<uint32_t> (currentState.stencilClipDepth);
                record.flags = 0;
                record.maskTextureIndex = currentState.maskTextureIndex;
                ++recordCursor;
            }
        }
    }

    return { firstInstance, recordCursor - firstInstance };
}

/*____________________________________________________________________________*/
// packGlyphQuadsIntoPrimitiveRecords — packs each atlas type's glyphs into the
// per-frame VulkanPrimitiveRecordBuffer via packGlyphQuadType() (one pass per
// type over the glyph span), recording the instance range each type occupies
// for issueGlyphDrawCalls() to draw separately (one instanced
// vk::CommandBuffer::draw() per non-empty atlas type — the SSBO model lets every
// glyph quad in a drawGlyphs() call for one atlas type batch into a single draw
// via instancing). Caller (drawGlyphs) has already ensured capacity.
jam::HashMap<jam::GlyphAtlas::Type, VulkanLowLevelGraphicsContext::InstanceRange>
VulkanLowLevelGraphicsContext::packGlyphQuadsIntoPrimitiveRecords (
    juce::Span<const uint16_t> glyphs, juce::Span<const juce::Point<float>> positions,
    juce::Typeface* typeface, const juce::AffineTransform& composedTransform, float rotation,
    jam::GlyphAtlas& atlas)
{
    auto& recordBuffer { context.getPrimitiveRecordBuffer() };
    auto* records { static_cast<VulkanPrimitiveRecord*> (recordBuffer.getMapped()) };
    const int capacityRecords { recordBuffer.getCapacityRecords() };

    int recordCursor { recordBuffer.getUsedRecords() };

    jam::HashMap<jam::GlyphAtlas::Type, InstanceRange> instanceRangeByType;

    for (auto type : { jam::GlyphAtlas::Type::mono, jam::GlyphAtlas::Type::emoji })
        instanceRangeByType.emplace (type,
            packGlyphQuadType (type, glyphs, positions, typeface, composedTransform, rotation, atlas,
                              records, recordCursor, capacityRecords));

    recordBuffer.setUsedRecords (recordCursor);

    return instanceRangeByType;
}

/*____________________________________________________________________________*/
// bindGlyphFrameState — binds the projection + record descriptor sets, sets
// viewport/scissor, and delegates to setStencilTestStateIfActive() (SSOT shared
// with recordRectFillDrawCommands) for stencil test-only state — glyph quads
// read the stencil but never write it, and the reference tracks
// currentState.stencilClipDepth only when a stencil clip is active.
void VulkanLowLevelGraphicsContext::bindGlyphFrameState (vk::CommandBuffer cmd, vk::PipelineLayout layout)
{
    updateProjection();

    vk::DescriptorSet recordSet { context.getRecordDescriptorSet() };
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 2, recordSet, nullptr);

    vk::DescriptorSet projSet { context.getProjectionDescriptorSet() };
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 3, projSet, nullptr);

    setFullViewport (cmd, context.getSwapchainExtent());
    const vk::Rect2D scissor { computeScissor() };
    cmd.setScissor (0, scissor);

    setStencilTestStateIfActive (cmd);
}

/*____________________________________________________________________________*/
// issueGlyphTypeDrawCall — selects the glyphQuad pipeline (mono/emoji ×
// stencil/no-stencil), binds the persistent bindless texture descriptor set (Step
// A3 — same handle for both types, disambiguated by each quad's
// VulkanPrimitiveRecord::textureIndex), and issues its instanced draw (4 vertices per
// instance, one instance per glyph quad).
void VulkanLowLevelGraphicsContext::issueGlyphTypeDrawCall (vk::CommandBuffer cmd, vk::PipelineLayout layout,
                                                       jam::GlyphAtlas::Type type, const InstanceRange& range)
{
    const bool isEmoji { type == jam::GlyphAtlas::Type::emoji };
    const auto pipelineId { currentState.stencilClipDepth > 0
        ? (isEmoji ? VulkanPipelines::ID::glyphEmojiInstancedStencil : VulkanPipelines::ID::glyphMonoInstancedStencil)
        : (isEmoji ? VulkanPipelines::ID::glyphEmojiInstanced : VulkanPipelines::ID::glyphMonoInstanced) };

    const vk::DescriptorSet atlasSet { context.getBindlessTextureDescriptorSet() };

    cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, context.getPipelines()[pipelineId]);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eGraphics, layout, 1, atlasSet, nullptr);
    cmd.draw (4, static_cast<uint32_t> (range.instanceCount), 0, static_cast<uint32_t> (range.firstInstance));
}

/*____________________________________________________________________________*/
// issueGlyphDrawCalls — binds shared glyph draw state once, then issues one
// instanced vk::CommandBuffer::draw() call per non-empty atlas type.
void VulkanLowLevelGraphicsContext::issueGlyphDrawCalls (
    const jam::HashMap<jam::GlyphAtlas::Type, InstanceRange>& instanceRangeByType)
{
    vk::CommandBuffer cmd { context.getCommandBuffer() };
    vk::PipelineLayout layout { context.getPipelines().getLayout() };

    bindGlyphFrameState (cmd, layout);

    for (auto type : { jam::GlyphAtlas::Type::mono, jam::GlyphAtlas::Type::emoji })
    {
        const auto& range { instanceRangeByType.at (type) };

        if (range.instanceCount > 0)
            issueGlyphTypeDrawCall (cmd, layout, type, range);
    }
}

/*____________________________________________________________________________*/
// drawGlyphs — atlas-backed glyph rendering at ANY angle. Rotation is part of
// glyph identity (jam_GlyphAtlas.h's Key doc comment): rather than falling
// back to a native outline/bitmap-layer path for a rotated composedTransform,
// the rotation angle is extracted once below and threaded into every atlas
// key, so GlyphAtlas rasterizes the glyph bitmap ALREADY ROTATED — the same
// atlas backend + gamma/hinting pipeline as unrotated text, identical
// rendering at any angle. The prior outline/fillComplexPath fallback
// (drawRotatedGlyphs()) is gone.
//
// atan2 (composedTransform.mat10, composedTransform.mat00) recovers the
// rotation angle baked into composedTransform — exactly 0.0f when
// mat10 == 0 and mat00 > 0 (atan2 (0, positive) is bit-exact +0.0f per the
// C++ standard's atan2 specification), so an untransformed glyph's Key is
// bit-identical to its pre-rotation-support shape — no cache fragmentation
// for the overwhelmingly common unrotated case.
//
// Each glyph index is resolved through GlyphAtlas::getOrRasterize() by
// packGlyphQuadType(), which either returns a cached region or rasterizes the
// glyph and packs it into the CPU-side atlas image. On rasterization miss, the
// atlas dirty flags are set and the GPU image is re-uploaded after the gather
// loop. Records are written directly for mono (R8) and emoji (BGRA8) atlas
// types, one pass per type over the glyph span, each pass producing one
// contiguous range in the per-frame VulkanPrimitiveRecordBuffer. One instanced
// vk::CommandBuffer::draw call is issued per non-empty atlas type, each
// binding the appropriate glyphQuad pipeline and descriptor set. Quad
// placement math is unchanged regardless of rotation — the bitmap is
// pre-rotated, bearings are already in rotated-space (see each backend's own
// rasterize definition).
void VulkanLowLevelGraphicsContext::drawGlyphs (juce::Span<const uint16_t> glyphs,
                                           juce::Span<const juce::Point<float>> positions,
                                           const juce::AffineTransform& t)
{
    const auto composedTransform { currentState.currentTransform.getTransformWith (t) };
    const float rotation { std::atan2 (composedTransform.mat10, composedTransform.mat00) };

    auto typeface { currentState.font.getTypefacePtr() };
    jassert (typeface != nullptr);

    auto* atlas { jam::GlyphAtlas::getInstance() };
    jassert (atlas != nullptr);

    const int atlasDim { atlas->getDimension() };
    jassert (atlasDim > 0);
    uploadDirtyAtlasSlots (*atlas, atlasDim);

    // pendingCellRun (setCellRun()'s state, targeting THIS drawGlyphs() call)
    // is read entirely inside packGlyphQuadType() — the trailing clear below
    // ensures a following plain drawGlyphs() call never reuses stale run state.
    if (not glyphs.empty())
    {
        auto& recordBuffer { context.getPrimitiveRecordBuffer() };
        context.reservePrimitiveRecords (recordBuffer.getUsedRecords() + static_cast<int> (glyphs.size()));

        const auto instanceRangeByType { packGlyphQuadsIntoPrimitiveRecords (
            glyphs, positions, typeface.get(), composedTransform, rotation, *atlas) };

        uploadDirtyAtlasSlots (*atlas, atlasDim);

        issueGlyphDrawCalls (instanceRangeByType);
    }

    pendingCellRun = {}; // consumed — cleared so a following plain drawGlyphs() call never reuses stale run state
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
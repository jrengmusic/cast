/** @file jam_VulkanLowLevelGraphicsContext.h
 *  @brief Vulkan-backed VulkanLowLevelGraphicsContext for jam.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief VulkanLowLevelGraphicsContext backed by Vulkan, using JUCE's D2D-isomorphic
 *         coordinate model.
 *
 *  Maintains local graphics state (transform, clip region, fill, opacity, font)
 *  without delegating to a software renderer. fillRect / fillRectList / drawLine /
 *  drawImage / fillPath / clipToPath / clipToImageAlpha / drawGlyphs /
 *  beginTransparencyLayer / endTransparencyLayer all issue native Vulkan
 *  draw/stencil commands.
 *
 *  Coordinate model mirrors JUCE's Direct2D LLGC:
 *  - currentTransform tracks the cumulative user-space to device-space mapping
 *  - deviceSpaceClipList stores clip regions in device (physical pixel) space
 *  - orthoProjection maps device-space pixels to Vulkan NDC
 */
class VulkanLowLevelGraphicsContext : public juce::LowLevelGraphicsContext
{
public:
    /** @brief Constructs a Vulkan-backed graphics context.
     *  @param vulkanGraphics  The VulkanGraphics for this window (must outlive this object).
     *  @param width           Render target logical width in pixels.
     *  @param height          Render target logical height in pixels.
     *  @param scaleFactor     Display scale factor (physical / logical).
     */
    VulkanLowLevelGraphicsContext (VulkanGraphics& vulkanGraphics, int width, int height, float scaleFactor)
        : context (vulkanGraphics)
        , scale (scaleFactor)
        , orthoProjection (glm::ortho (
              0.0f,
              static_cast<float> (juce::jmax (1, static_cast<int> (width * scaleFactor))),
              0.0f,
              static_cast<float> (juce::jmax (1, static_cast<int> (height * scaleFactor))),
              -1.0f,
              1.0f))
    {
        const auto physicalWidth { juce::jmax (1, static_cast<int> (width * scaleFactor)) };
        const auto physicalHeight { juce::jmax (1, static_cast<int> (height * scaleFactor)) };
        currentState.deviceSpaceClipList.addWithoutMerging (
            juce::Rectangle<int> (0, 0, physicalWidth, physicalHeight));
        targetExtent.width  = static_cast<uint32_t> (physicalWidth);
        targetExtent.height = static_cast<uint32_t> (physicalHeight);

        if (scaleFactor != 1.0f)
            currentState.currentTransform.addTransform (juce::AffineTransform::scale (scaleFactor));

    }

    /** @brief Ends the frame via VulkanGraphics::endFrame(). */
    ~VulkanLowLevelGraphicsContext() override
    {
        context.endFrame();
    }

    // ---- VulkanLowLevelGraphicsContext overrides ----

    /** @brief Returns false — this is a raster device, not a vector device. */
    bool isVectorDevice() const override { return false; }

    /** @brief Shifts the coordinate origin by @p o in device space.
     *  @param o  The pixel delta to apply to the current origin.
     */
    void setOrigin (juce::Point<int> o) override { currentState.currentTransform.setOrigin (o); }

    /** @brief Accumulates @p t into the current user-to-device transform.
     *  @param t  The additional transform to compose.
     */
    void addTransform (const juce::AffineTransform& t) override
    {
        currentState.currentTransform.addTransform (t);
    }

    /** @brief Returns the physical pixel scale factor from the current transform. */
    float getPhysicalPixelScaleFactor() const override
    {
        return currentState.currentTransform.getPhysicalPixelScaleFactor();
    }

    /** @brief Intersects the clip region with @p r transformed to device space.
     *  @param r  The rectangle to clip to in user-space coordinates.
     *  @returns  False when the resulting clip region is empty.
     */
    bool clipToRectangle (const juce::Rectangle<int>& r) override
    {
        const auto& transform { currentState.currentTransform };

        if (transform.isOnlyTranslated)
        {
            currentState.deviceSpaceClipList.clipTo (transform.translated (r));
        }
        else
        {
            const auto transformed {
                transform.boundsAfterTransform (r.toFloat()).getSmallestIntegerContainer()
            };
            currentState.deviceSpaceClipList.clipTo (transformed);
        }

        return not isClipEmpty();
    }

    /** @brief Intersects the clip region with each rectangle in @p rects transformed to device space.
     *  @param rects  The rectangle list to clip to in user-space coordinates.
     *  @returns      False when the resulting clip region is empty.
     */
    bool clipToRectangleList (const juce::RectangleList<int>& rects) override
    {
        const auto& transform { currentState.currentTransform };

        if (transform.isOnlyTranslated)
        {
            juce::RectangleList<int> transformed;

            for (const auto& r : rects)
                transformed.addWithoutMerging (transform.translated (r));

            currentState.deviceSpaceClipList.clipTo (transformed);
        }
        else
        {
            juce::RectangleList<int> transformed;

            for (const auto& r : rects)
                transformed.addWithoutMerging (
                    transform.boundsAfterTransform (r.toFloat()).getSmallestIntegerContainer());

            currentState.deviceSpaceClipList.clipTo (transformed);
        }

        return not isClipEmpty();
    }

    /** @brief Subtracts @p r transformed to device space from the clip region.
     *  @param r  The rectangle to exclude in user-space coordinates.
     */
    void excludeClipRectangle (const juce::Rectangle<int>& r) override
    {
        const auto& transform { currentState.currentTransform };

        if (transform.isOnlyTranslated)
        {
            currentState.deviceSpaceClipList.subtract (transform.translated (r));
        }
        else
        {
            currentState.deviceSpaceClipList.subtract (
                transform.boundsAfterTransform (r.toFloat()).getSmallestIntegerContainer());
        }
    }

    /** @brief Clips to the stencil-rasterised path (see jam_VulkanLowLevelGraphicsContextPath.cpp).
     *  @param p  The path defining the new clip boundary in user-space coordinates.
     *  @param t  Additional transform applied on top of currentTransform.
     */
    void clipToPath (const juce::Path& p, const juce::AffineTransform& t) override;

    /** @brief Clips to an image alpha mask via the clipMaskInstanced pipeline
     *  (see jam_VulkanLowLevelGraphicsContextImage.cpp) — mirrors clipToPath's stencil
     *  write, substituting an image quad for triangulated path geometry.
     *  @param i  Source image whose alpha channel defines the clip mask.
     *  @param t  Transform mapping image space to device space.
     */
    void clipToImageAlpha (const juce::Image& i, const juce::AffineTransform& t) override;

    /** @brief Returns true when @p r transformed to device space intersects the current clip region.
     *  @param r  The rectangle to test in user-space coordinates.
     */
    bool clipRegionIntersects (const juce::Rectangle<int>& r) override
    {
        const auto& transform { currentState.currentTransform };

        if (transform.isOnlyTranslated)
            return currentState.deviceSpaceClipList.intersectsRectangle (transform.translated (r));

        return currentState.deviceSpaceClipList.intersectsRectangle (
            transform.boundsAfterTransform (r.toFloat()).getSmallestIntegerContainer());
    }

    /** @brief Returns the current clip bounds transformed back to user space. */
    juce::Rectangle<int> getClipBounds() const override
    {
        return currentState.currentTransform
            .deviceSpaceToUserSpace (currentState.deviceSpaceClipList.getBounds())
            .getSmallestIntegerContainer();
    }

    /** @brief Returns true when the current clip region contains no pixels. */
    bool isClipEmpty() const override { return currentState.deviceSpaceClipList.isEmpty(); }

    /** @brief Pushes the current graphics state onto the state stack. */
    void saveState() override { stateStack.add (currentState); }

    /** @brief Pops the most recently saved graphics state from the state stack. */
    void restoreState() override
    {
        if (not stateStack.isEmpty())
        {
            currentState = stateStack.last();
            stateStack.remove (stateStack.size() - 1);
        }
    }

    /** @brief Begins an offscreen transparency layer with the given opacity (see jam_VulkanLowLevelGraphicsContextTransparency.cpp).
     *  @param opacity  Opacity in [0, 1] applied when compositing the layer onto the parent target.
     */
    void beginTransparencyLayer (float opacity) override;

    /** @brief Composites the current offscreen transparency layer back onto the parent target (see jam_VulkanLowLevelGraphicsContextTransparency.cpp). */
    void endTransparencyLayer() override;

    /** @brief Sets the fill type for subsequent fill operations.
     *  @param f  The fill type (solid colour, gradient, or tiled image).
     */
    void setFill (const juce::FillType& f) override { currentState.fill = f; }

    /** @brief Sets the global opacity multiplier for subsequent draw operations.
     *  @param o  Opacity in [0, 1].
     */
    void setOpacity (float o) override { currentState.opacity = o; }

    /** @brief Sets the resampling quality for image draw operations.
     *  @param q  The desired resampling quality level.
     */
    void setInterpolationQuality (juce::Graphics::ResamplingQuality q) override
    {
        currentState.quality = q;
    }

    // fillRect family — native Vulkan path (see jam_VulkanLowLevelGraphicsContext.cpp).

    /** @brief Fills @p r with the current fill, issuing a native Vulkan draw.
     *  @param r                        The rectangle in user-space coordinates.
     *  @param replaceExistingContents  Ignored — Vulkan pipeline handles blending.
     */
    void fillRect (const juce::Rectangle<int>& r, bool replaceExistingContents) override;

    /** @brief Fills @p r with the current fill, issuing a native Vulkan draw.
     *  @param r  The rectangle in user-space coordinates (sub-pixel precision).
     */
    void fillRect (const juce::Rectangle<float>& r) override;

    /** @brief Fills each rectangle in @p r with the current fill via native Vulkan draws.
     *  @param r  The list of rectangles in user-space coordinates.
     */
    void fillRectList (const juce::RectangleList<float>& r) override;

    /** @brief Fills @p p with the current fill using earcut triangulation (see jam_VulkanLowLevelGraphicsContextPath.cpp).
     *  @param p  The path to fill in user-space coordinates.
     *  @param t  Additional transform applied on top of currentTransform.
     */
    void fillPath (const juce::Path& p, const juce::AffineTransform& t) override;

    /** @brief Draws @p i as a textured quad using the imageInstanced pipeline (see jam_VulkanLowLevelGraphicsContextImage.cpp).
     *  @param i  The JUCE image to draw.
     *  @param t  Transform mapping image space to device space, composed with currentTransform.
     */
    void drawImage (const juce::Image& i, const juce::AffineTransform& t) override;

    /** @brief Rasterises @p l as a 1 px-thick line — delegates to drawLineWithThickness().
     *  @param l  The line in user-space coordinates.
     */
    void drawLine (const juce::Line<float>& l) override;

    /** @brief Rasterises @p line as a quad of the given thickness via the triList pipeline.
     *  @param line           The line in user-space coordinates.
     *  @param lineThickness  Stroke width in user-space units.
     */
    void drawLineWithThickness (const juce::Line<float>& line, float lineThickness) override;

    /** @brief Sets the current font for glyph draw operations.
     *  @param f  The font to use.
     */
    void setFont (const juce::Font& f) override { currentState.font = f; }

    /** @brief Returns the current font. */
    const juce::Font& getFont() override { return currentState.font; }

    /** @brief Sets run-scoped codepoint/span + cell-box metrics consumed by the
     *  immediately following drawGlyphs() call — mirrors setFont()'s own
     *  per-run idiom (GlyphArrangement::draw()'s existing context.setFont(runFont)
     *  call site immediately before context.drawGlyphs()). Threads
     *  GlyphConstraint's own documented "required extension" (emoji
     *  cell-fit + PUA/NF GlyphConstraint application) into GlyphAtlas::Key —
     *  see packGlyphQuadType() (jam_VulkanLowLevelGraphicsContextGlyph.cpp).
     *  GlyphArrangement::draw() is the sole caller (dynamic_cast dispatch from
     *  its own juce::LowLevelGraphicsContext& handle — see that method's doc
     *  comment for why). Consumed and cleared by the following drawGlyphs()
     *  call; @p codepoints / @p spans must stay valid until it returns —
     *  ownership stays with the caller (GlyphArrangement::Run's own HeapBlocks).
     *  @param codepoints  Per-glyph original Unicode codepoints, parallel to the
     *                      glyph indices the following drawGlyphs() call will receive.
     *  @param spans       Per-glyph display width in cells (1 = narrow, 2 = wide).
     *  @param count       Element count of both arrays — must equal the following
     *                      drawGlyphs() call's glyph count, or this state is
     *                      ignored (packGlyphQuadType() falls back to the
     *                      no-cell-run-context Key shape for every glyph).
     *  @param cellWidth   Terminal cell width, physical pixels.
     *  @param cellHeight  Terminal cell height, physical pixels.
     *  @param baseline    Cell-top-to-baseline offset, physical pixels.
     */
    void setCellRun (const char32_t* codepoints, const uint8_t* spans, int count,
                     int cellWidth, int cellHeight, int baseline) noexcept
    {
        pendingCellRun = { codepoints, spans, count, cellWidth, cellHeight, baseline };
    }

    /** @brief Renders glyphs via the atlas pipeline — rasterizes on miss, packs quads, issues Vulkan draw
     *         (see jam_VulkanLowLevelGraphicsContextGlyph.cpp).
     *  @param glyphs      OpenType glyph indices.
     *  @param positions   Per-glyph baseline positions in user space.
     *  @param t           Additional transform composed with currentTransform.
     */
    void drawGlyphs (juce::Span<const uint16_t> glyphs,
                     juce::Span<const juce::Point<float>> positions,
                     const juce::AffineTransform& t) override;

#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
    /** @brief Executes shader's buffer passes (mid-paint suspend/resume of the
     *  active scene pass, via VulkanGraphics::recordShaderBufferPasses()) then emits
     *  its Image pass into its own offscreen gather target, followed by the
     *  background combine draw that composites the gather target's result
     *  into the still-active scene render pass (see
     *  jam_VulkanLowLevelGraphicsContextRender.cpp). No-op — skips the draw
     *  entirely for this call — if shader's GPU execution resources fail to
     *  build (transient build failure; logged via jam::debug::Log).
     *  @param shader           The compiled shader to render.
     *  @param opacity          Caller's own opacity value (VulkanShader carries no
     *                          opacity field, see jam_VulkanShader.h's VulkanShader
     *                          doc comment).
     *  @param resolutionScale  Caller's own intermediate-pass extent fraction,
     *                          [0, 1] (likewise not a VulkanShader field).
     *  @param mouse            Caller's own iMouse value, forwarded verbatim to
     *                          VulkanGraphics::recordShaderBufferPasses() (see its
     *                          own doc comment for the sign-encoding contract).
     *  @param camera           Caller's own jam::VulkanOrbitCamera — read
     *                          (getViewMatrix()/getProjectionMatrix()/
     *                          getNormalMatrix()) ONLY when shader's built
     *                          execution.hasMesh() is true
     *                          (recordShaderImagePassDrawCommands()'s own
     *                          appended mesh material-range/feature-edge draw);
     *                          unused, but always supplied, for every meshless
     *                          shader — mirrors @p mouse's own "always passed,
     *                          read only if the shader cares" contract
     *                          (jam::VulkanShaderComponent owns one
     *                          unconditionally, jam_VulkanShaderComponent.h).
     */
    void renderShader (const VulkanShader& shader, float opacity, float resolutionScale, const std::array<float, 4>& mouse,
                       const VulkanOrbitCamera& camera);
#endif

    /** @brief Returns VulkanImageType so temporary images (component buffers,
     *  effect sources) are engine-backed VulkanImagePixelData. JUCE's
     *  StandardCachedComponentImage and EffectState query this and
     *  transparently keep buffers GPU-side. Out-of-line in
     *  jam_VulkanLowLevelGraphicsContext.cpp — VulkanImageType is defined
     *  after this header in the jam_vulkan.h include chain. */
    std::unique_ptr<juce::ImageType> getPreferredImageTypeForTemporaryImages() const override;

    /** @brief Returns the monotonically increasing frame counter. */
    uint64_t getFrameId() const override { return frameId; }

private:
    // ---- Nested types (internal draw-call data, see corresponding .cpp for usage) ----

    /** @brief Location of a triangulated path's geometry once appended to the path
     *  VulkanFrameBuffer — the first index and the base vertex, for vk::CommandBuffer::drawIndexed().
     */
    struct PackedPathRange
    {
        int firstIndex { 0 };
        int vertexBase { 0 };
    };

    /** @brief Contiguous range of VulkanPrimitiveRecord entries appended for one drawGlyphs()
     *  atlas type, for vk::CommandBuffer::draw()'s instanced (firstInstance, instanceCount) parameters.
     *  Renamed from IndexRange — glyph quads are no longer drawn via an index
     *  buffer; each atlas type's quads become one instanced draw over their record range.
     */
    struct InstanceRange
    {
        int firstInstance { 0 };
        int instanceCount { 0 };
    };

    // ---- Shared + rect-fill helpers (see jam_VulkanLowLevelGraphicsContext.cpp) ----

    /** @brief Uploads orthoProjection to the projection storage buffer. Vertices are pre-transformed to device space on CPU by each draw method. */
    void updateProjection();

    /** @brief Converts deviceSpaceClipList bounds to a vk::Rect2D clamped to the swapchain extent.
     *  @returns  The scissor rectangle for the current clip region.
     */
    vk::Rect2D computeScissor() const;

    /** @brief Sets the full-target viewport and the current clip region as the scissor rect.
     *  Shared by every draw method that issues geometry against the active clip
     *  (issueRectFill, drawImage, fillPath, clipToPath).
     *  @param cmd  The command buffer to record into.
     */
    void setViewportAndScissor (vk::CommandBuffer cmd) const;

    /** @brief Sets the stencil compare/write masks and reference for stencil-gated
     *  test-only draws, when a stencil clip is active. No-op otherwise.
     *  @param cmd  The command buffer to record into.
     */
    void setStencilTestStateIfActive (vk::CommandBuffer cmd) const;

    /** @brief Appends one VulkanPrimitiveRecord to the per-frame SSBO (VulkanPrimitiveRecordBuffer)
     *  for a rect/image/clip-mask quad — position/size from @p deviceBounds, uv from
     *  @p uvRect ({0,0,1,1} for untextured primitives), colour with @p opacity baked
     *  into the alpha channel. stencilRef is populated from currentState.stencilClipDepth
     *  at the moment this record is appended — clip remains dormant until the
     *  winding-rule work wires encoding into the flags/clip channels.
     *  @param deviceBounds  Top-left position + size in device space.
     *  @param uvRect        UV rectangle, extent semantics (x, y = UV origin, w, h = UV extent).
     *  @param colour        Fill/tint colour (opaque white for image draws).
     *  @param opacity       Opacity multiplier, baked into colour's alpha channel.
     *  @param textureIndex  Bindless texture array slot (0 for untextured rect
     *                       fills, which never bind set 1).
     *  @returns              The record index written (for gl_InstanceIndex addressing).
     */
    int appendPrimitiveRecord (juce::Rectangle<float> deviceBounds,
                              juce::Rectangle<float> uvRect,
                              juce::Colour colour,
                              float opacity,
                              uint32_t textureIndex);

    /** @brief Transforms @p bounds to device space and appends one VulkanPrimitiveRecord
     *  via appendPrimitiveRecord().
     *  @param bounds      The rectangle to fill in logical (user-space) coordinates.
     *  @param fillColour  The colour to fill with.
     *  @param opacity     Opacity multiplier in [0, 1].
     *  @returns            The record index written.
     */
    int appendRectFillRecord (juce::Rectangle<float> bounds, juce::Colour fillColour, float opacity);

    /** @brief Binds descriptor/record state, pushes colour + opacity, selects the
     *  rect pipeline (alpha/opaque × stencil/no-stencil), and issues the instanced draw.
     *  @param recordIndex  The record index written by appendRectFillRecord().
     *  @param fillColour   The colour to fill with.
     *  @param opacity      Opacity multiplier in [0, 1].
     */
    void recordRectFillDrawCommands (int recordIndex, juce::Colour fillColour, float opacity);

    /** @brief Records a single rect fill — transforms bounds to device space, packs 4 pixel corners, binds pipeline, issues draw.
     *  @param bounds      The rectangle to fill in logical (user-space) coordinates.
     *  @param fillColour  The colour to fill with.
     *  @param opacity     Opacity multiplier in [0, 1].
     */
    void issueRectFill (juce::Rectangle<float> bounds, juce::Colour fillColour, float opacity);

    // Rebuilds lut/lutGradient when gradient (opacity-baked) changed, promotes lut
    // through the texture cache, pushes VulkanGradientPushConstants, selects the
    // gradientFill pipeline, and issues the instanced draw.
    void recordGradientFillDrawCommands (int recordIndex, const juce::ColourGradient& gradient, float opacity);

    // Native Vulkan gradient draw — mirrors issueRectFill(), substituting a
    // LUT-sampled gradient for a solid colour.
    void issueGradientFill (juce::Rectangle<float> bounds, const juce::ColourGradient& gradient, float opacity);

    // ---- VulkanImage draw helpers (see jam_VulkanLowLevelGraphicsContextImage.cpp) ----


    /** @brief Transforms the image-space bounds (0, 0, width, height) by currentTransform
     *  composed with @p t, returning the result in device space.
     *  @param image  The JUCE image being drawn (supplies width/height).
     *  @param t      Transform mapping image space to device space.
     *  @returns       The image's bounds in device space.
     */
    juce::Rectangle<float> computeImageDeviceBounds (const juce::Image& image, const juce::AffineTransform& t) const;

    /** @brief Transforms the image-space quad by currentTransform composed with @p t
     *  to device space, and appends one VulkanPrimitiveRecord via appendPrimitiveRecord().
     *  @param image  The JUCE image being drawn (supplies width/height).
     *  @param t      Transform mapping image space to device space.
     *  @returns       The record index written.
     */
    int appendImageQuadRecord (const juce::Image& image, const juce::AffineTransform& t);

    /** @brief Binds the image descriptor + record state, pushes colour + opacity,
     *  selects the imageInstanced pipeline (stencil/no-stencil), and issues the
     *  instanced draw.
     *  @param recordIndex  The record index written by appendImageQuadRecord().
     *  @param image        The JUCE image being drawn (supplies the cached image view).
     *  @param opacity      Opacity multiplier in [0, 1].
     */
    void recordImageDrawCommands (int recordIndex, const juce::Image& image, float opacity);

    /** @brief Binds the image descriptor + record state and issues the stencil-write
     *  draw for clipToImageAlpha() — the alpha-mask fragment shader discards where
     *  source alpha is zero, so only the covered region writes currentState.stencilClipDepth.
     *  @param recordIndex  The record index written by appendImageQuadRecord().
     *  @param image        The JUCE image whose alpha channel defines the clip mask.
     */
    void recordClipToImageAlphaDrawCommands (int recordIndex, const juce::Image& image);

    // ---- Path fill/clip helpers (see jam_VulkanLowLevelGraphicsContextPath.cpp) ----

    /** @brief Appends triangulatePath()'s output to the path VulkanFrameBuffer. Caller must
     *  have already grown the VulkanFrameBuffer's capacity to fit @p vertices / @p indices.
     *  Shared by fillPath and clipToPath — both upload triangulated geometry the same
     *  way, differing only in which pipeline subsequently draws it.
     *  @param vertices  Triangulated vertices in device space.
     *  @param indices   Triangulated indices, already offset to reference @p vertices.
     *  @returns          The location of the appended range.
     */
    PackedPathRange appendTriangulatedPathToFrameBuffer (const jam::Array<juce::Point<float>>& vertices,
                                                          const jam::Array<uint32_t>& indices);

    /** @brief Binds the projection descriptor set (slot 0) and the path VulkanFrameBuffer's
     *  vertex + index buffers. Shared by fillPath and clipToPath — both issue indexed
     *  draws from triangulatePath()'s output.
     *  @param cmd     The command buffer to record into.
     *  @param layout  The pipeline layout owning descriptor slot 0.
     */
    void bindPathIndexedDrawState (vk::CommandBuffer cmd, vk::PipelineLayout layout) const;

    /** @brief Pushes the current fill colour, selects the tri-list pipeline
     *  (alpha/opaque × stencil/no-stencil), and issues the indexed draw for fillPath().
     *  @param range       The packed geometry range to draw.
     *  @param indexCount  Total index count of the packed geometry.
     */
    void recordFillPathDrawCommands (const PackedPathRange& range, int indexCount);

    /** @brief Binds the stencilWriteTriList pipeline and issues the indexed draw that
     *  writes currentState.stencilClipDepth into the stencil buffer for clipToPath().
     *  @param range       The packed geometry range to draw.
     *  @param indexCount  Total index count of the packed geometry.
     */
    void recordClipToPathDrawCommands (const PackedPathRange& range, int indexCount);

    // ---- Winding-accumulate-then-cover helpers (see jam_VulkanLowLevelGraphicsContextPath.cpp) ----

    /** @brief fillPath()'s complex-path branch — multiple subpaths
     *  (holes) or an explicit even-odd winding rule, neither of which triangulatePath()'s
     *  per-ring-independent earcut can represent correctly. Renders the isolated
     *  winding-accumulate-then-cover sequence into a scratch stencil+color target sized
     *  to the path's device-space bounding box (see VulkanWindingScratch), then
     *  composites the result back onto the active render target as a small textured
     *  quad — mirrors beginTransparencyLayer/endTransparencyLayer's offscreen-then-
     *  composite shape, substituting an owned bbox-sized scratch stencil for the shared
     *  full-extent one.
     *  @param path          The path to fill in user-space coordinates.
     *  @param polygon       path's subpaths already flattened to device space by fillPath().
     *  @param deviceBounds  path's bounding box in device space (union of all subpaths).
     */
    void fillComplexPath (const juce::Path& path,
                         const jam::Array<jam::Array<juce::Point<float>>>& polygon,
                         juce::Rectangle<float> deviceBounds);

    /** @brief Begins the isolated scratch render pass via VulkanGraphics'
     *  beginRenderPass(vk::Framebuffer, vk::Extent2D) SSOT — reuses the same CLEAR-op
     *  render pass object as the main scene, targeting VulkanWindingScratch's own
     *  framebuffer/capacity extent instead (LOAD_OP_CLEAR on both attachments, no
     *  persistence across calls); this also marks VulkanGraphics::renderPassActive true,
     *  so the eventual context.endRenderPass() correctly ends this scratch pass.
     *  Issues the windingStencilAccumulate
     *  draw (per-subpath fan geometry, INCR_WRAP/DECR_WRAP, colorWriteMask=0) followed by
     *  the windingCoverOpaque draw (bbox quad, stencil-tested against the value
     *  just accumulated — same subpass, no barrier needed, matching the fixed-function
     *  stencil ordering clipToPath's own write-then-test sequence already relies on),
     *  then ends the scratch render pass via context.endRenderPass() (not a raw
     *  vk::CommandBuffer::endRenderPass call) so VulkanGraphics::renderPassActive stays correct. Both draws
     *  share a viewport shifted so that
     *  window device-space position deviceBounds.getPosition() lands at the scratch
     *  framebuffer's pixel (0,0) — every vertex/record keeps its ordinary full-window
     *  device-space coordinates (no bbox-local retranslation, no swapping the shared
     *  projection storage buffer mid-frame).
     *  @param path          The path being filled — path.isUsingNonZeroWinding() selects
     *                       the cover draw's compareMask (0xFF nonzero, 0x01 even-odd;
     *                       both derived from the SAME accumulated value — INCR_WRAP/
     *                       DECR_WRAP wraparound is modular arithmetic mod 256, and 256 is
     *                       even, so the accumulated value's LSB always equals the true
     *                       winding number's parity regardless of sign or wraparound).
     *  @param range         The packed fan geometry range (see appendFanTriangulatedRing).
     *  @param indexCount    Total index count of the packed fan geometry.
     *  @param deviceBounds  path's bounding box in device space — also the cover draw's
     *                       VulkanPrimitiveRecord geometry.
     *  @param scratchExtent path's bounding box rounded up to whole pixels — the scratch
     *                       target's required extent and this call's scissor rect.
     */
    void recordWindingAccumulateAndCoverDrawCommands (const juce::Path& path,
                                                      const PackedPathRange& range,
                                                      int indexCount,
                                                      juce::Rectangle<float> deviceBounds,
                                                      vk::Extent2D scratchExtent);

    /** @brief Transitions the scratch color image for sampling (mirrors
     *  transitionTransparencyLayerForSampling exactly), resumes the correct render
     *  target via resumeRenderPassForNestingLevel(transparencyNestingLevel) — the main
     *  scene, or the enclosing transparency layer's own offscreen framebuffer if this
     *  fillComplexPath() call happened while nested inside beginTransparencyLayer() —
     *  and composites the finished scratch-color output back onto it as a textured
     *  quad via the existing imageInstanced/imageInstancedStencil pipeline
     *  — gated by the OUTER active clip exactly like any other content draw
     *  (recordImageDrawCommands's selection logic); the scratch target's own stencil
     *  bits never participate in this draw. No-op if the bindless array was at capacity
     *  when the scratch target was (re)created (capacity bound, not an architecture branch).
     *  @param deviceBounds  path's bounding box in device space — the composite quad's geometry.
     *  @param scratchExtent path's bounding box rounded up to whole pixels — scopes the
     *                       composite quad's uvRect to the scratch target's currently-used
     *                       sub-rect (the underlying image may be larger due to monotonic growth).
     */
    void recordWindingCompositeDrawCommands (juce::Rectangle<float> deviceBounds, vk::Extent2D scratchExtent);

    // ---- Transparency layer helpers (see jam_VulkanLowLevelGraphicsContextTransparency.cpp) ----

    /** @brief Ensures the offscreen target for @p level exists, then clears its
     *  color image and clears-or-copies its stencil image via transfer commands
     *  (run OUTSIDE any active render pass — legal only because
     *  beginTransparencyLayer() already ended the previously active pass before
     *  calling this method), before resuming a render pass into it via
     *  context.resumeRenderPass(target.framebuffer, extent) — both attachments are
     *  already correct by the time the render pass begins, and this marks
     *  VulkanGraphics::renderPassActive true so the eventual context.endRenderPass()
     *  correctly ends this offscreen pass regardless of nesting depth.
     *  When currentState.stencilClipDepth is nonzero, the outer clip's shaped mask
     *  is copied in from VulkanGraphics::stencilImage (the main scene's clip-depth
     *  stencil) so content drawn inside the layer still respects the outer clip's
     *  actual shape, not just its depth value; otherwise the stencil is cleared to zero.
     *  @param level  Transparency nesting level.
     */
    void beginOffscreenTransparencyRenderPass (int level);

    /** @brief Resumes rendering with LOAD op after an offscreen pass has closed,
     *  branching on @p nestingLevel: a nested close (nestingLevel > 0) resumes the
     *  PARENT transparency level's own offscreen framebuffer — its content is already
     *  correct (the just-closed child's composite draws onto it), nothing to re-clear;
     *  closing the outermost level (== 0) resumes the true main scene target instead.
     *  Shared SSOT for endTransparencyLayer() (closing a transparency layer) and
     *  recordWindingCompositeDrawCommands() (closing the winding-scratch pass, which
     *  may itself be nested inside an active transparency layer) — both need this
     *  identical branch.
     *  @param nestingLevel  Current transparency nesting depth at the point of resume.
     */
    void resumeRenderPassForNestingLevel (int nestingLevel);

    /** @brief Transitions the offscreen target's resolve image for sampling during
     *  the composite draw. A same-layout (SHADER_READ_ONLY_OPTIMAL →
     *  SHADER_READ_ONLY_OPTIMAL) memory-dependency barrier — the shared
     *  renderPass already declares finalLayout = SHADER_READ_ONLY_OPTIMAL for this
     *  resolve attachment, so no actual layout transition happens here; this barrier
     *  only guarantees FRAGMENT_SHADER-stage read visibility of the resolve write,
     *  which the render pass's implicit vk::SubpassExternal dependency does not by
     *  itself provide.
     *  @param target  The transparency target to transition.
     */
    void transitionTransparencyLayerForSampling (VulkanTransparencyStack::TransparencyLayer& target) const;

    /** @brief Binds the persistent bindless texture descriptor set + record descriptor
     *  set (set 2), pushes @p layerOpacity, selects the imageInstanced pipeline, and
     *  issues the full-screen composite draw via gl_InstanceIndex (instanced.vert, zero
     *  vertex attributes). The offscreen target's bindless array slot is baked
     *  into the record's textureIndex by compositeTransparencyLayer() — no per-call
     *  descriptor set is bound here.
     *  @param recordIndex   The record index written by appendPrimitiveRecord() for the full-screen quad.
     *  @param extent        The swapchain extent (used for viewport/scissor).
     *  @param layerOpacity  Opacity applied when compositing the layer.
     */
    void recordTransparencyCompositeDrawCommands (int recordIndex, vk::Extent2D extent, float layerOpacity);

    /** @brief Draws @p target as a full-screen textured quad at @p layerOpacity onto
     *  the now-resumed main render pass.
     *  @param target        The transparency target to composite.
     *  @param layerOpacity  Opacity applied when compositing the layer.
     */
    void compositeTransparencyLayer (VulkanTransparencyStack::TransparencyLayer& target, float layerOpacity);

    // ---- Glyph helpers (see jam_VulkanLowLevelGraphicsContextGlyph.cpp) ----

    /** @brief Uploads any dirty atlas slots (mono and/or emoji) to their GPU images.
     *  Brackets the upload with endRenderPass()/resumeRenderPass() since transfer
     *  commands are illegal inside an active render pass. No-op if neither slot is dirty.
     *  @param atlas      The CPU-side glyph atlas.
     *  @param atlasDim   Atlas image side length in texels (square).
     */
    void uploadDirtyAtlasSlots (jam::GlyphAtlas& atlas, int atlasDim);

    /** @brief Packs each atlas type's glyphs into the per-frame VulkanPrimitiveRecordBuffer,
     *  one type at a time via packGlyphQuadType(), recording the instance range each
     *  type occupies.
     *  @param glyphs            OpenType glyph indices.
     *  @param positions         Per-glyph baseline positions in user space.
     *  @param typeface          The typeface owning the glyphs (identity for the atlas key).
     *  @param composedTransform currentTransform composed with drawGlyphs()'s explicit transform.
     *  @param rotation          Rotation angle, in radians, extracted from composedTransform
     *                            (drawGlyphs()'s `atan2 (composedTransform.mat10, composedTransform.mat00)`);
     *                            0.0f for untransformed text. Threaded into every atlas key.
     *  @param atlas             The CPU-side glyph atlas.
     *  @returns                  The instance range written for each atlas type.
     */
    jam::HashMap<jam::GlyphAtlas::Type, InstanceRange> packGlyphQuadsIntoPrimitiveRecords (
        juce::Span<const uint16_t> glyphs,
        juce::Span<const juce::Point<float>> positions,
        juce::Typeface* typeface,
        const juce::AffineTransform& composedTransform,
        float rotation,
        jam::GlyphAtlas& atlas);

    /** @brief Resolves every glyph in @p glyphs through GlyphAtlas::getOrRasterize() and
     *  writes a VulkanPrimitiveRecord directly into the record buffer for each glyph
     *  whose resolved region belongs to @p type (whitespace glyphs and glyphs
     *  belonging to the other atlas type are skipped), starting at the current record
     *  cursor and advancing it — one contiguous range per type, shared per-type body of
     *  packGlyphQuadsIntoPrimitiveRecords()'s loop. Each written record's bindless
     *  index is read via atlas.getTexture(type).getBindlessIndex(context.getNativeHandle())
     *  — this window's own registered slot assignment, now owned by the atlas's own
     *  VulkanBindlessTexture's registry rather than by context (VulkanGraphics).
     *  @param type               Which atlas type this pass writes records for.
     *  @param glyphs             OpenType glyph indices.
     *  @param positions          Per-glyph baseline positions in user space.
     *  @param typeface           The typeface owning the glyphs (identity for the atlas key).
     *  @param composedTransform  currentTransform composed with drawGlyphs()'s explicit transform.
     *  @param rotation           Rotation angle, in radians, extracted from composedTransform
     *                             by packGlyphQuadsIntoPrimitiveRecords()'s caller; 0.0f for
     *                             untransformed text.
     *  @param atlas              The CPU-side glyph atlas.
     *  @param records            Mapped VulkanPrimitiveRecordBuffer base pointer.
     *  @param recordCursor       Running record cursor, advanced by 1 per written record.
     *  @param capacityRecords    Record capacity of @p records, asserted against before each write.
     *  @returns                   The instance range this type occupies.
     */
    InstanceRange packGlyphQuadType (jam::GlyphAtlas::Type type,
                                     juce::Span<const uint16_t> glyphs,
                                     juce::Span<const juce::Point<float>> positions,
                                     juce::Typeface* typeface,
                                     const juce::AffineTransform& composedTransform,
                                     float rotation,
                                     jam::GlyphAtlas& atlas,
                                     VulkanPrimitiveRecord* records, int& recordCursor, int capacityRecords);

    /** @brief Binds the projection + record descriptor sets, sets viewport/scissor, and
     *  configures stencil test-only state (glyph quads read the stencil but never write it).
     *  @param cmd     The command buffer to record into.
     *  @param layout  The pipeline layout owning descriptor slot 0.
     */
    void bindGlyphFrameState (vk::CommandBuffer cmd, vk::PipelineLayout layout);

    /** @brief Selects the glyphQuad pipeline (mono/emoji × stencil/no-stencil), binds
     *  the atlas descriptor set for @p type, and issues its instanced draw.
     *  @param cmd     The command buffer to record into.
     *  @param layout  The pipeline layout owning descriptor slot 1 (atlas sampler).
     *  @param type    Which atlas type's quads to draw.
     *  @param range   The instance range for @p type, as packed by packGlyphQuadsIntoPrimitiveRecords().
     */
    void issueGlyphTypeDrawCall (vk::CommandBuffer cmd, vk::PipelineLayout layout,
                                 jam::GlyphAtlas::Type type, const InstanceRange& range);

    /** @brief Binds shared glyph draw state once, then issues one instanced draw per
     *  non-empty atlas type.
     *  @param instanceRangeByType  The instance range for each atlas type.
     */
    void issueGlyphDrawCalls (const jam::HashMap<jam::GlyphAtlas::Type, InstanceRange>& instanceRangeByType);

#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
    // ---- VulkanShader render helpers (see jam_VulkanLowLevelGraphicsContextRender.cpp) ----

    /** @brief Binds the Image pass's own dedicated gather-target pipeline/
     *  descriptor set, pushes baseUniforms with this pass's own
     *  channels[]/opacity filled in, and draws the shared fullscreen triangle
     *  into execution.getImagePassGatherTarget()'s offscreen framebuffer, at
     *  this shader's own scaled extent (baseUniforms.iResolution); when
     *  execution.hasMesh() is true, appends VulkanGraphics::
     *  recordMeshGatherDrawCommands()'s own material-range/feature-edge
     *  draws directly into that SAME render-pass instance, immediately after
     *  (a no-op for every meshless shader); then binds the background
     *  combine pipeline (VulkanGraphics::getOrCreateBackgroundCombinePipeline())
     *  and draws the fullscreen triangle again, into the still-active scene
     *  render pass, compositing the gather target's result over the scene —
     *  the SAME target either way, mesh present or not (see this method's
     *  own doc comment, jam_VulkanLowLevelGraphicsContextRender.cpp). Takes
     *  no VulkanShader reference — every value it needs (execution, baseUniforms,
     *  opacity) already arrived explicitly through its parameters (VulkanShader
     *  itself carries no opacity field, jam_VulkanShader.h's VulkanShader doc
     *  comment).
     *  @param execution     shader's built execution — already ensured ready by
     *                       renderShader() before this is called.
     *  @param baseUniforms  This renderShader() call's stamped uniforms, returned
     *                       by VulkanGraphics::recordShaderBufferPasses() — iTime/
     *                       iTimeDelta/iFrame/iResolution/iMouse/iScene reused
     *                       verbatim (SSOT, one stamp per call); only
     *                       channels[]/opacity vary per pass.
     *  @param opacity       renderShader()'s own opacity parameter.
     *  @param camera        renderShader()'s own @p camera parameter — read
     *                       ONLY when execution.hasMesh() is true, to build
     *                       the mesh's own view/projection/normal matrices
     *                       for the appended VulkanGraphics::
     *                       recordMeshGatherDrawCommands() call.
     */
    void recordShaderImagePassDrawCommands (VulkanShaderInstance& execution,
                                            const VulkanShaderUniforms& baseUniforms, float opacity,
                                            const VulkanOrbitCamera& camera);
#endif

    // ---- VulkanGraphics state ----
    // VulkanTransformState (mirrors juce::RenderingHelpers::TranslationOrTransform)
    // lives at namespace scope in jam_VulkanTransformState.h — no
    // dependency on this class, included ahead of this header in jam_vulkan.h.

    struct State
    {
        /** @brief Cumulative user-space to device-space coordinate mapping. */
        VulkanTransformState currentTransform;

        /** @brief Active clip region in device (physical pixel) space. */
        juce::RectangleList<int> deviceSpaceClipList;

        /** @brief Global opacity multiplier applied to all draw operations. */
        float opacity { 1.0f };

        /** @brief Current fill type (solid colour, gradient, or tiled image). */
        juce::FillType fill { juce::Colours::black };

        /** @brief Current font used for glyph draw operations. */
        juce::Font font { juce::FontOptions{} };

        /** @brief Resampling quality for image draw operations. */
        juce::Graphics::ResamplingQuality quality { juce::Graphics::highResamplingQuality };

        /** @brief Nesting depth of active path/image-alpha stencil clips. 0 means no
         *  active stencil clip; > 0 gates stencil-test pipeline selection and supplies
         *  the stencil reference value. Lives inside State (not a sibling class member)
         *  so saveState()/restoreState() cover it by construction — a clip established
         *  inside a save/restore block can no longer leak stencil-gating onto draws
         *  issued after the matching restoreState() (fixes defect #1). */
        int stencilClipDepth { 0 };

        /** @brief Bindless slot of the active alpha mask texture. Initialised to
         *  noMaskIndex; clipToImageAlpha overwrites it with the caller-supplied
         *  mask's bindless slot — sampled only by masked_image.frag via
         *  nonuniformEXT(maskTextureIndex). Lives inside State so
         *  saveState()/restoreState() cover it by construction. */
        uint32_t maskTextureIndex { noMaskIndex };
    };

    // ---- Members ----

    /** @brief The VulkanGraphics owning the swapchain, pipelines, and command buffer for this window. */
    VulkanGraphics& context;

    /** @brief Display scale factor (physical pixels / logical pixels). */
    float scale;

    /** @brief Stack of saved graphics states, grown by saveState() and drained by restoreState(). */
    jam::Array<State> stateStack;

    /** @brief Currently active graphics state (transform, clip, fill, opacity, font). */
    State currentState;

    /** @brief Monotonically increasing frame identifier, returned by getFrameId(). */
    uint64_t frameId { 0 };

    /** @brief Fixed orthographic projection mapping device-space pixels to Vulkan NDC. */
    glm::mat4 orthoProjection;

    /** @brief Nesting depth of active transparency layers. */
    int transparencyNestingLevel { 0 };

    /** @brief Opacity values for each active transparency layer nesting level. */
    jam::Array<float> transparencyOpacityStack;

    /** @brief Run-scoped cell-run state set by setCellRun(), consumed and
     *  cleared by drawGlyphs() (packGlyphQuadType()) — see setCellRun()'s own
     *  doc comment. */
    jam::PendingCellRun pendingCellRun {};

    // This LLGC's own render-target extent.
    vk::Extent2D targetExtent {};

    // Gradient-fill LUT texture, rebuilt only when lutGradient changes.
    juce::Image lut;

    // The opacity-baked gradient lut was last built from — compared via
    // ColourGradient::operator== to skip rebuild when unchanged.
    juce::ColourGradient lutGradient;

    // The composed transform lut was last built with — compared via
    // AffineTransform::operator== to skip rebuild when unchanged.
    juce::AffineTransform lutTransform;
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
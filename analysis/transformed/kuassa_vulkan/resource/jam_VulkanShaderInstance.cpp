//
// Build orchestration file: computeScaledExtent()/build() (the top-level
// per-content-hash orchestrator — resolves passAliases/externalTextureChannels,
// dispatches to buildSlangPassResources()/buildMeshResources() below, then
// builds every buffer pass's own target via buildRenderResources())/
// buildRenderResources()/createFullscreenPipeline() (buffer-pass/Image-pass
// render target + pipeline construction), stamping (stampUniforms()/
// stampChannels()), and the remaining accessors (getChannelBindlessIndex()).
// Two sibling files split out of this one by concern:
// jam_VulkanShaderInstanceSlang.cpp (buildSlangPassResources() and its own
// buildSlangPassDescriptorSetLayout()/buildSlangPassPipelineLayout() steps,
// plus refreshSlangPass()) and jam_VulkanShaderInstanceMesh.cpp
// (buildMeshResources() and its own buildMeshGpuResources()/
// createMeshDescriptorPoolAndSet() steps, plus buildMeshHookPipelines()/
// createMeshHookPipeline() — this execution's own hooked mesh pipelines,
// mesh_shader= is a vertex-ANIMATION HOOK into the engine's own
// default mesh look, mirroring Shadertoy's mainImage paradigm one level
// down) — build() calls into both, via buildMeshIfDeclared().

namespace jam
{
/*____________________________________________________________________________*/
// Minimum pixel extent any built target is ever clamped to — never below
// 1x1, shared by computeScaledExtent() and computePassExtent() below (the
// same clamp both already need, hoisted once rather than duplicated).
static constexpr uint32_t minimumPixelExtent { 1 };

// Per-scale-type axis-extent rule dispatch — jam::LookupTable direct-indexed
// by jam::VulkanShaderFormat's own scale-type ordinal (source/viewport/
// absolute, jam_vulkan/bimap/jam_VulkanShaderFormat.h; mirrors jam_VulkanShaderCompiler.cpp's
// optimizationLevelLut dispatch-table
// idiom) — RetroArch's own scale_typeN vocabulary (jam_VulkanShaderPreset.h),
// resolved independently per axis (Pass::scaleTypeX/scaleTypeY).
using ScaleAxisRule = uint32_t (*) (float scale, uint32_t previousAxisExtent, uint32_t viewportAxisExtent);

static uint32_t sourceScaleAxis (float scale, uint32_t previousAxisExtent, uint32_t /*viewportAxisExtent*/) noexcept
{
    return juce::jmax (minimumPixelExtent, static_cast<uint32_t> (static_cast<float> (previousAxisExtent) * scale));
}

static uint32_t viewportScaleAxis (float scale, uint32_t /*previousAxisExtent*/, uint32_t viewportAxisExtent) noexcept
{
    return juce::jmax (minimumPixelExtent, static_cast<uint32_t> (static_cast<float> (viewportAxisExtent) * scale));
}

static uint32_t absoluteScaleAxis (float scale, uint32_t /*previousAxisExtent*/, uint32_t /*viewportAxisExtent*/) noexcept
{
    return juce::jmax (minimumPixelExtent, static_cast<uint32_t> (scale));
}

static constexpr jam::LookupTable<int, ScaleAxisRule, 3> scaleAxisRuleLut
{
    {
        { VulkanShaderFormat::source,   sourceScaleAxis },
        { VulkanShaderFormat::viewport, viewportScaleAxis },
        { VulkanShaderFormat::absolute, absoluteScaleAxis },
    }
};

/*____________________________________________________________________________*/
// Computes one buffer pass's own render-target extent from its preset's own
// per-axis scale directive (jam_VulkanShaderPreset.h's own Pass doc comment)
// -- source resolves against @p previousExtent (the previous pass's own
// already-computed extent in the chain, or build()'s own chainInputExtent
// for pass 0), viewport against @p viewportExtent (this execution's own
// scaledExtent), absolute to an exact pixel count (Pass::scaleX/Y hold the
// pixel count as a float, static_cast here). A pass ordinal at or past
// preset.passes' own size -- this VulkanShader's preset declares no entry for it,
// true of every buffer pass on every VulkanShaderFormat::shadertoy shader, whose
// preset is always empty -- falls back to @p viewportExtent unchanged,
// exactly today's pre-existing scaledExtent-for-every-pass behavior.
static vk::Extent2D computePassExtent (const VulkanShaderPreset& preset, size_t passOrdinal,
                                       vk::Extent2D previousExtent, vk::Extent2D viewportExtent) noexcept
{
    vk::Extent2D passExtent { viewportExtent };

    if (passOrdinal < static_cast<size_t> (preset.passes.size()))
    {
        const auto& presetPass { preset.passes.at (static_cast<int> (passOrdinal)) };

        passExtent = vk::Extent2D {
            scaleAxisRuleLut[presetPass.scaleTypeX] (
                presetPass.scaleX, previousExtent.width, viewportExtent.width),
            scaleAxisRuleLut[presetPass.scaleTypeY] (
                presetPass.scaleY, previousExtent.height, viewportExtent.height)
        };
    }

    return passExtent;
}

/*____________________________________________________________________________*/
// A buffer pass's own mip level count when its downstream consumer pass
// declares mipmap_inputN (RetroArch's own "the texture feeding INTO pass N
// needs mip levels" directive, jam_VulkanShaderPreset.h's own Pass::
// mipmapInput doc comment) -- VulkanRenderResources::numMipLevels' own doc comment
// for the exact consumer-ordinal derivation. Full chain down to a 1x1 mip,
// matching every standard GPU mip-chain convention (vkCmdBlitImage's own
// halved-per-level contract, VulkanGraphics::recordMipChainGeneration()).
static uint32_t computeMipLevelCount (vk::Extent2D extent) noexcept
{
    // extent's own axes are already juce::jmax-clamped to minimumPixelExtent
    // by every ScaleAxisRule above (this function's only caller,
    // computePassExtent()'s own result) -- findHighestSetBit()'s "input value
    // of 0 is illegal" precondition (juce_MathsFunctions.h) is never at risk.
    const uint32_t largestDimension { juce::jmax (extent.width, extent.height) };

    // juce::findHighestSetBit() (JUCE juce_MathsFunctions.h) -- integer index
    // of the highest set bit (n=3 -> 1, n=7 -> 2), the exact floor(log2(n))
    // value with no float rounding exposure; +1 is this function's own
    // "full chain down to a 1x1 mip" count (doc comment above).
    return static_cast<uint32_t> (juce::findHighestSetBit (largestDimension) + 1);
}

/*____________________________________________________________________________*/
//==============================================================================
// Constructor / Destructor
//==============================================================================

VulkanShaderInstance::VulkanShaderInstance (VulkanDevice& vulkanDevice)
    : device (vulkanDevice)
{
}

VulkanShaderInstance::~VulkanShaderInstance()
{
    for (auto& target : bufferPassTargets)
    {
        for (auto& framebuffer : target->framebuffers)
            device.getDevice().destroyFramebuffer (framebuffer, nullptr);

        device.getDevice().destroyPipeline (target->pipeline, nullptr);
    }

    for (auto& framebuffer : imagePassGatherTarget.framebuffers)
        device.getDevice().destroyFramebuffer (framebuffer, nullptr);

    device.getDevice().destroyPipeline (imagePassGatherTarget.pipeline, nullptr);

    // VulkanMesh-backed material-range draw —
    // the mesh's own material-range/feature-edge draw has no render
    // target/pipeline of its own to destroy here anymore: it renders
    // directly into imagePassGatherTarget (destroyed above) via the
    // engine-owned mesh pipelines (VulkanGraphics-owned, VulkanGraphics::
    // getOrCreateMeshPipeline() et al.). meshDescriptorPool destroy frees
    // meshDescriptorSet with it — no separate free needed (mirrors
    // slangDescriptorPool's identical "destroying the pool frees every
    // descriptor set allocated from it" note below) — a no-op (empty/null
    // handle) for every meshless execution.
    device.getDevice().destroyDescriptorPool (meshDescriptorPool, nullptr);

    // This execution's own hooked mesh pipelines (buildMeshHookPipelines()) —
    // unlike the engine-default mesh pipelines above, these are per-instance
    // (this VulkanShader's own compiled hooked SPIR-V), so they are destroyed here,
    // never by VulkanGraphics. A documented no-op (null handles) for every
    // execution with no mesh_shader= connection or whose hooked vertex-stage
    // compile/pipeline build failed.
    device.getDevice().destroyPipeline (meshHookFillPipeline, nullptr);
    device.getDevice().destroyPipeline (meshHookTransparentFillPipeline, nullptr);
    device.getDevice().destroyPipeline (meshHookEdgePipeline, nullptr);

    // Slang-only resources — empty/no-op for a Shadertoy-format execution
    // (slangPasses stays empty, slangDescriptorPool stays null; every
    // destroy call below is a documented no-op against an empty handle,
    // the same tolerance every other destroy call in this destructor
    // already relies on). Destroying slangDescriptorPool frees every
    // descriptor set allocated from it — no separate per-set free needed.
    for (auto& passResources : slangPasses)
    {
        device.getDevice().destroyPipelineLayout (passResources->pipelineLayout, nullptr);
        device.getDevice().destroyDescriptorSetLayout (passResources->descriptorSetLayout, nullptr);
    }

    device.getDevice().destroyDescriptorPool (slangDescriptorPool, nullptr);

    // RAII members self-destruct: bufferPassTargets' and imagePassGatherTarget's
    // owned Images (vector<VulkanImage> / Owner<VulkanRenderResources> destruction),
    // originalHistoryImages' own Images (empty, no-op, when the
    // OriginalHistoryN feature is inactive for this execution), and each
    // slangPasses entry's own VulkanBuffer (uniformBuffer).
}

//==============================================================================
// Build
//==============================================================================

vk::Extent2D VulkanShaderInstance::computeScaledExtent (vk::Extent2D sceneExtent, float resolutionScale) noexcept
{
    return vk::Extent2D {
        juce::jmax (minimumPixelExtent, static_cast<uint32_t> (static_cast<float> (sceneExtent.width) * resolutionScale)),
        juce::jmax (minimumPixelExtent, static_cast<uint32_t> (static_cast<float> (sceneExtent.height) * resolutionScale))
    };
}

bool VulkanShaderInstance::build (const VulkanShader& shader, float resolutionScale, vk::Format colorFormat, vk::Extent2D sceneExtent,
                             const std::function<vk::RenderPass (vk::Format)>& offscreenRenderPassFactory,
                             vk::CommandBuffer commandBuffer, vk::DescriptorSetLayout meshDescriptorSetLayout,
                             vk::PipelineLayout meshPipelineLayout, vk::PipelineLayout pipelineLayout,
                             vk::ShaderModule sharedVertModule, vk::Extent2D chainInputExtent)
{
    const vk::Extent2D scaledExtent { computeScaledExtent (sceneExtent, resolutionScale) };

    // Stored before any pipeline is built — buildRenderResources() (called
    // below, for every buffer pass and again for the Image pass's own gather
    // target) reads these members rather than taking them as parameters
    // every call.
    storedPipelineLayout = pipelineLayout;
    storedVertModule = sharedVertModule;
    slangShader = shader.format == VulkanShaderFormat::slang;

    buildPassAliasesAndExternalTextureChannels (shader);

    bool built { true };

    // A slang-format shader's own per-pass descriptor set/UBO/dedicated
    // pipeline layout must exist before buildRenderResources() below can
    // build a pass's pipeline against its own layout instead of the shared
    // pipelineLayout parameter — see this method's own doc comment.
    if (slangShader)
        built = buildSlangPassResources (shader.passes, shader.preset);

    // RetroArch's OriginalHistoryN depth-detection scan (slang only) —
    // buildOriginalHistoryRing() is itself a no-op (returns true unchanged)
    // for a VulkanShaderFormat::shadertoy execution — see its own doc comment.
    if (built)
        built = buildOriginalHistoryRing (colorFormat, sceneExtent);

    if (built)
        built = buildBufferPassTargets (shader, colorFormat, scaledExtent, chainInputExtent, offscreenRenderPassFactory);

    if (built)
        built = buildImagePassTarget (shader, colorFormat, scaledExtent, offscreenRenderPassFactory);

    // VulkanMesh-backed material-range draw — gated on @p built (the rest of this
    // execution, above, already built successfully) but its OWN result feeds
    // meshReady directly, never ANDed back into @p built (buildMeshIfDeclared()'s
    // own doc comment; buildMeshResources()'s own doc comment for the full
    // graceful-last-good rationale).
    if (built)
        buildMeshIfDeclared (shader, commandBuffer, meshDescriptorSetLayout, meshPipelineLayout);

    return finalizeBuild (built, shader.contentHash, scaledExtent);
}

void VulkanShaderInstance::buildMeshIfDeclared (const VulkanShader& shader, vk::CommandBuffer commandBuffer,
                                          vk::DescriptorSetLayout meshDescriptorSetLayout,
                                          vk::PipelineLayout meshPipelineLayout)
{
    // A no-op for every shader.meshPath-empty execution (every project before
    // this feature) — meshReady stays at its own default false. The mesh's
    // own material-range/feature-edge draw has no render target/pipeline of
    // its own to build — it draws directly into imagePassGatherTarget
    // (already built above, by buildImagePassTarget()) via the engine-owned
    // mesh pipelines (VulkanGraphics::getOrCreateMeshPipeline() et al.) or this
    // execution's own hooked ones.
    if (shader.meshPath.isNotEmpty())
        meshReady = buildMeshResources (shader.meshShapes, commandBuffer, meshDescriptorSetLayout);

    // mesh_shader= is a vertex-ANIMATION HOOK into the engine's own
    // default mesh look — the hooked pipelines are built only when actual
    // mesh geometry exists to draw (meshReady) AND this VulkanShader declares a
    // mesh_shader= connection (shader.meshShaderSource non-empty). A no-op
    // otherwise — every meshHookXxxPipeline stays at its own null default,
    // hasMeshHook() stays false, VulkanGraphics::recordMeshMaterialRangeDraws()
    // falls back to the engine-default, unanimated pipelines.
    if (meshReady and shader.meshShaderSource.isNotEmpty())
        buildMeshHookPipelines (shader, meshPipelineLayout);
}

bool VulkanShaderInstance::finalizeBuild (bool built, uint64_t shaderContentHash, vk::Extent2D scaledExtent)
{
    if (built)
    {
        contentHash = shaderContentHash;
        builtExtent = scaledExtent;
        startTimeMs = juce::Time::getMillisecondCounterHiRes();
        lastStampTimeMs = startTimeMs;
        frameCounter = 0;
    }

    ready = built;

    return ready;
}

void VulkanShaderInstance::buildPassAliasesAndExternalTextureChannels (const VulkanShader& shader)
{
    // aliasN (jam_VulkanShaderPreset.h, already carrying VulkanShaderCompiler::
    // compile()'s own #pragma name fallback overlay for an entry declaring
    // no aliasN of its own) resolves through the SAME ordinal space
    // PassOutputN/PassFeedbackN already do (VulkanGraphics::
    // getSlangTextureBindings()'s own alias-normalization step,
    // jam_VulkanGraphicsSlangPass.cpp) -- built once here regardless of
    // format, since shader.preset.passes is always empty for a
    // VulkanShaderFormat::shadertoy shader (this loop is simply a no-op for it).
    for (int ordinal = 0; ordinal < shader.preset.passes.size(); ++ordinal)
        if (const auto& alias { shader.preset.passes.at (ordinal).alias }; alias.isNotEmpty())
            passAliases.emplace (alias, ordinal);

    // Named external LUT/textures (shader.preset.textures) -- pure
    // declaration-order push-constant slot: texture K's own slot is always
    // bufferPassCount + K (jam::VulkanShaderCompiler::compile()'s own
    // identical formula, jam_VulkanShaderCompiler.cpp's channelMacros()),
    // computed HERE rather than carried as a field, then copied into
    // externalTextureChannels so stampChannels() never needs this
    // execution's own VulkanShader reference at stamp time (see
    // externalTextureChannels' own doc comment). Built once here regardless
    // of format, mirroring passAliases' own identical loop immediately
    // above -- a no-op for every preset declaring no textures at all.
    const int bufferPassCount { static_cast<int> (shader.passes.size()) - 1 };

    for (int index = 0; index < shader.preset.textures.size(); ++index)
        externalTextureChannels.emplace (shader.preset.textures.at (index).name,
                                          bufferPassCount + index);
}

bool VulkanShaderInstance::buildOriginalHistoryRing (vk::Format colorFormat, vk::Extent2D sceneExtent)
{
    bool built { true };

    // Every pass's own reflected texture list (already built by
    // buildSlangPassResources(), this method's own required predecessor in
    // build()'s call sequence) is scanned for the VulkanShaderReflection::
    // getOriginalHistoryPrefix() vocabulary; the highest ordinal found across the
    // WHOLE pass chain sizes the ring buffer below. OriginalHistory0 needs
    // no ring slot (VulkanGraphics::getSlangTextureBindings() resolves it
    // identically to Original), so a chain declaring only OriginalHistory0
    // (or none at all) leaves originalHistoryImages empty
    // (getOriginalHistoryDepth() == 0, feature inactive).
    if (slangShader)
    {
        int maxOriginalHistoryDepth { 0 };

        for (auto& passResources : slangPasses)
            for (auto& texture : passResources->reflection.textures)
                if (texture.name.startsWith (VulkanShaderReflection::getOriginalHistoryPrefix()))
                    maxOriginalHistoryDepth = juce::jmax (maxOriginalHistoryDepth,
                        texture.name.substring (VulkanShaderReflection::getOriginalHistoryPrefix().length()).getIntValue());

        if (maxOriginalHistoryDepth > 0)
        {
            // Sized maxOriginalHistoryDepth + 1 — see originalHistoryImages'
            // own doc comment for why the ring always carries one more slot
            // than the deepest requested read. Built at @p sceneExtent (the
            // full, un-scaled scene extent — this ring's own source,
            // VulkanGraphics::straightAlphaImage, is swapchain-sized, never
            // resolutionScale-scaled) and @p colorFormat (swapchain-
            // mirroring, same as every other unconditionally-colorFormat-
            // built target in this class); eTransferDst instead of
            // eColorAttachment — this ring is written by vkCmdCopyImage
            // (VulkanGraphics::recordOriginalHistoryCopy()), never drawn into
            // directly.
            originalHistoryImages.resize (maxOriginalHistoryDepth + 1);

            for (auto& ringImage : originalHistoryImages)
            {
                ringImage = VulkanImage::create2D (device.getAllocator(), device.getDevice(), colorFormat, sceneExtent,
                                             vk::SampleCountFlagBits::e1,
                                             vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                                             vk::ImageAspectFlagBits::eColor);
                built = built and ringImage.isValid();
            }
        }
    }

    return built;
}

bool VulkanShaderInstance::buildBufferPassTargets (const VulkanShader& shader, vk::Format colorFormat, vk::Extent2D scaledExtent,
                                             vk::Extent2D chainInputExtent,
                                             const std::function<vk::RenderPass (vk::Format)>& offscreenRenderPassFactory)
{
    const auto& passes { shader.passes };

    // Every buffer pass is unconditionally ping-pong-feedback-capable
    // (VulkanShaderPass's own doc comment, jam_VulkanShaderPass.h).
    static constexpr int pingPongImageCount { 2 };

    // Running "source"-typed basis for the NEXT pass in the chain — seeded
    // with chainInputExtent for pass 0 (this shader's own virtual/actual
    // input, per build()'s own @p chainInputExtent doc comment), then
    // replaced with each buffer pass's own just-computed extent so pass N+1's
    // own "source" scale-type directive resolves against pass N's real
    // extent, never the shared scaledExtent (per the per-pass render-target
    // extent contract).
    vk::Extent2D previousChainExtent { chainInputExtent };
    bool built { true };

    // Every pass but the last (the mandatory Image pass) is a buffer pass —
    // VulkanShader's ctor guarantees the Image pass is always the final entry.
    for (size_t passIndex = 0; built and passIndex + 1 < passes.size(); ++passIndex)
    {
        auto bufferTarget { std::make_unique<VulkanRenderResources>() };
        const vk::PipelineLayout passPipelineLayout {
            slangShader ? slangPasses.at (passIndex)->pipelineLayout : storedPipelineLayout
        };

        const vk::Extent2D passExtent { computePassExtent (shader.preset, passIndex, previousChainExtent, scaledExtent) };

        // srgb_framebufferN/float_framebufferN (jam_VulkanShaderPreset.h,
        // parse()'s own FBO_SCALE_FLAG_VALID gate) — srgb wins when both are
        // set (RetroArch shader_vulkan.cpp:2018-2025, if/else-if, srgb
        // checked first). A pass ordinal at or past shader.preset.passes'
        // own size (every VulkanShaderFormat::shadertoy buffer pass, whose preset
        // is always empty) falls back to a default-constructed Pass, whose
        // own srgbFramebuffer/floatFramebuffer are both false — today's
        // exact @p colorFormat-for-every-pass behavior, mirroring
        // computePassExtent()'s own out-of-range fallback immediately above.
        const VulkanShaderPreset::Pass passPreset { passIndex < static_cast<size_t> (shader.preset.passes.size())
            ? shader.preset.passes.at (static_cast<int> (passIndex)) : VulkanShaderPreset::Pass {} };

        const vk::Format passFormat { passPreset.srgbFramebuffer ? vk::Format::eR8G8B8A8Srgb
            : passPreset.floatFramebuffer ? vk::Format::eR16G16B16A16Sfloat : colorFormat };

        // mipmap_inputN (jam_VulkanShaderPreset.h's own Pass::mipmapInput doc
        // comment) is declared on the CONSUMER of the texture that needs mip
        // levels, not on the pass that produces it — this pass's own ordinal
        // + 1 is exactly that consumer's own preset ordinal, whether that
        // consumer is the next buffer pass or the mandatory Image pass
        // (whose own preset ordinal always equals passes.size() - 1, i.e.
        // the buffer pass count -- VulkanRenderResources::numMipLevels' own doc
        // comment). A preset declaring no entry at ordinal (passIndex + 1) at
        // all (every VulkanShaderFormat::shadertoy shader, or a slang preset
        // shorter than its own pass chain) falls back to false — the same
        // "absent means no mips" contract as the un-consumed mipmap_input0
        // (ordinal 0 is never read here — passIndex starts at 0, so this
        // lookup's ordinal starts at 1 — its own consumer would-be producer,
        // this engine's own external chain input/straight-alpha scene image,
        // predates any VulkanShader and carries no mip infrastructure at all;
        // graceful base-level fallback, not a bug).
        const bool consumerDeclaresMipmapInput { (passIndex + 1) < static_cast<size_t> (shader.preset.passes.size())
            and shader.preset.passes.at (static_cast<int> (passIndex + 1)).mipmapInput };

        const uint32_t passMipLevels { consumerDeclaresMipmapInput ? computeMipLevelCount (passExtent) : 1 };

        built = buildRenderResources (*passes.at (passIndex), passFormat, passExtent,
                                     offscreenRenderPassFactory (passFormat), pingPongImageCount, passMipLevels,
                                     passPipelineLayout, *bufferTarget);
        previousChainExtent = passExtent;
        bufferPassTargets.add (std::move (bufferTarget));
    }

    return built;
}

bool VulkanShaderInstance::buildImagePassTarget (const VulkanShader& shader, vk::Format colorFormat, vk::Extent2D scaledExtent,
                                           const std::function<vk::RenderPass (vk::Format)>& offscreenRenderPassFactory)
{
    const auto& passes { shader.passes };

    // The Image pass's own offscreen gather target, built the exact same
    // way as every buffer pass's own target above, just from the VulkanImage
    // pass's own compiled spirv/vertexSpirv (see imagePassGatherTarget's
    // own doc comment, jam_VulkanShaderInstance.h). imageCount is 2
    // (history-capable ping-pong pair) only when this is a real .slang
    // shader whose Image pass's own reflection (slangPasses is already
    // fully built above, by buildSlangPassResources()) declares a
    // self-feedback read of its own output — RetroArch's real-world
    // "PassFeedback<final pass ordinal>" OR "<alias>Feedback" vocabulary,
    // sampled by a 1-pass feedback preset (feedback.slangp
    // -> feedback.slang) via VulkanGraphics::getSlangTextureBindings()'s own
    // self-feedback resolution branch (jam_VulkanGraphicsSlangPass.cpp) —
    // this gate must accept the same two spellings that resolution accepts,
    // or an alias-feedback shader builds a single-image gather target that
    // resolution can never fire a self-feedback read against.
    // Every other shader (VulkanShaderFormat::shadertoy, or a slang shader
    // whose Image pass declares no such texture) needs no history at
    // all — a single fullscreen write every frame, no self-read — so
    // imageCount stays 1 (VulkanRenderResources' own doc comment: a second
    // half would sit permanently unused).
    bool imagePassDeclaresSelfFeedback { false };

    if (slangShader)
    {
        const juce::String imagePassSelfFeedbackName {
            VulkanShaderReflection::getPassFeedbackPrefix() + juce::String (passes.size() - 1) };

        // The Image pass's own alias (passAliases already fully populated
        // above, before any pipeline is built) spells self-feedback the
        // SAME way VulkanGraphics::getSlangTextureBindings() resolves it —
        // "<alias>" + VulkanShaderReflection::getFeedbackSuffix(), RetroArch's own
        // author-alias PASS_FEEDBACK vocabulary (that function's own doc
        // comment, jam_VulkanGraphicsSlangPass.cpp) — never populated
        // when the Image pass declares no aliasN of its own (every
        // VulkanShaderFormat::shadertoy shader, whose preset is always empty).
        juce::String imagePassAliasFeedbackName;

        for (auto& [alias, ordinal] : passAliases)
            if (ordinal == static_cast<int> (passes.size() - 1))
                imagePassAliasFeedbackName = alias + VulkanShaderReflection::getFeedbackSuffix();

        for (auto& texture : slangPasses.at (passes.size() - 1)->reflection.textures)
            imagePassDeclaresSelfFeedback = imagePassDeclaresSelfFeedback
                                           or texture.name == imagePassSelfFeedbackName
                                           or texture.name == imagePassAliasFeedbackName;
    }

    const int gatherImageCount { imagePassDeclaresSelfFeedback ? 2 : 1 };
    const vk::PipelineLayout imagePassPipelineLayout {
        slangShader ? slangPasses.at (passes.size() - 1)->pipelineLayout : storedPipelineLayout
    };

    // The Image pass's own gather target STAYS at scaledExtent AND
    // @p colorFormat unconditionally, never a preset-relative
    // computePassExtent()/srgb_framebufferN/float_framebufferN
    // resolution — RetroArch's own final pass always renders to the
    // viewport at the engine's own format (this engine's resolution
    // control IS that viewport; Pass::build()'s own !final_pass
    // framebuffer guard discards a final-pass format request outright,
    // shader_vulkan.cpp:2942-2946, verified this session) — see
    // VulkanGraphics::getOrCreateShaderInstance()'s own doc comment,
    // jam_VulkanGraphicsSlangPass.cpp. numMipLevels stays 1
    // unconditionally — this engine has no pass past the Image pass to
    // ever declare mipmap_inputN against it (VulkanRenderResources::
    // numMipLevels' own doc comment).
    static constexpr uint32_t imagePassMipLevels { 1 };

    return buildRenderResources (*passes.back(), colorFormat, scaledExtent,
                                 offscreenRenderPassFactory (colorFormat), gatherImageCount, imagePassMipLevels,
                                 imagePassPipelineLayout, imagePassGatherTarget);
}

bool VulkanShaderInstance::buildRenderResources (const VulkanShaderPass& pass, vk::Format colorFormat, vk::Extent2D passExtent,
                                           vk::RenderPass offscreenRenderPass, int imageCount, uint32_t numMipLevels,
                                           vk::PipelineLayout passPipelineLayout, VulkanRenderResources& outTarget) const
{
    outTarget.images.resize (imageCount);
    outTarget.framebuffers.resize (imageCount);

    // jam::Array has no assign(count, value) — resize() alone zero-inits
    // freshly-grown slots (ElementType{} == 0, not the -1 "unassigned"
    // sentinel every bindlessIndex slot must start at), so the sentinel is
    // written explicitly per slot after resizing.
    outTarget.bindlessIndex.resize (imageCount);

    for (int slot = 0; slot < imageCount; ++slot)
        outTarget.bindlessIndex.set (slot, -1);

    outTarget.extent = passExtent;
    outTarget.format = colorFormat;
    outTarget.renderPass = offscreenRenderPass;
    outTarget.numMipLevels = numMipLevels;

    // One shared depth image for this WHOLE target — every half in images[]
    // below references this SAME depth attachment (depth is transient,
    // re-cleared every draw by the shared offscreen render pass's own
    // loadOp=eClear depth attachment, never read back across frames, so one
    // depth image safely serves whichever half is currently being written —
    // VulkanRenderResources::depthImage's own doc comment). Built here,
    // unconditionally, for every target this method builds (a buffer pass's
    // own ping-pong pair, and the mandatory Image pass's own gather target
    // alike) — the shared offscreen render pass shape (@p offscreenRenderPass)
    // now carries this attachment unconditionally, so every framebuffer built
    // against it must supply one (VUID-VkFramebufferCreateInfo-attachmentCount-00876).
    outTarget.depthImage = VulkanImage::create2D (
        device.getAllocator(), device.getDevice(), offscreenDepthFormat, passExtent, vk::SampleCountFlagBits::e1,
        vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageAspectFlagBits::eDepth);

    bool built { outTarget.depthImage.isValid() };

    // eTransferSrc/eTransferDst added only when this target's own mip chain
    // needs generating (VulkanGraphics::recordMipChainGeneration()'s own per-level
    // vkCmdBlitImage source/destination requirement) — every non-mipped
    // target (numMipLevels == 1, every call site's own behavior before this
    // sweep) stays at its exact pre-existing usage flags.
    const vk::ImageUsageFlags mipUsage { numMipLevels > 1
        ? vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
        : vk::ImageUsageFlags {} };

    for (int half = 0; built and half < imageCount; ++half)
    {
        outTarget.images.at (half) = VulkanImage::create2D (
            device.getAllocator(), device.getDevice(), colorFormat, passExtent, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | mipUsage,
            vk::ImageAspectFlagBits::eColor, numMipLevels);

        built = outTarget.images.at (half).isValid();

        if (built)
        {
            // getAttachmentView(), never getView() — a framebuffer attachment
            // must reference exactly one mip level
            // (VUID-VkFramebufferCreateInfo-pAttachments-00891), which
            // view's own full-range subresourceRange (levelCount ==
            // numMipLevels) can no longer guarantee once numMipLevels > 1.
            // Second attachment is outTarget.depthImage's own view, SHARED
            // across every half's own framebuffer (built once, above).
            const std::array<vk::ImageView, 2> attachments {
                outTarget.images.at (half).getAttachmentView(), outTarget.depthImage.getAttachmentView()
            };

            const vk::FramebufferCreateInfo fbInfo { {}, offscreenRenderPass, attachments,
                                                      passExtent.width, passExtent.height, 1 };

            const vk::Result createFramebufferResult { device.getDevice().createFramebuffer (
                &fbInfo, nullptr, &outTarget.framebuffers.at (half)) };
            built = createFramebufferResult == vk::Result::eSuccess;
        }
    }

    if (built)
    {
        vk::ShaderModule fragModule { VulkanPipelines::createShaderModule (
            device.getDevice(), static_cast<const char*> (pass.spirv.getData()),
            static_cast<int> (pass.spirv.getSize())) };

        built = fragModule != nullptr;

        // This pass's own vertex stage (VulkanShaderPass::vertexSpirv non-empty) gets
        // a transient dedicated module, created/destroyed the same way as
        // fragModule above; otherwise the shared storedVertModule is used
        // as-is (today's behavior for every Shadertoy-compiled pass) and no
        // per-pass vertex module is created or destroyed here.
        const bool hasOwnVertexStage { built and not pass.vertexSpirv.isEmpty() };
        vk::ShaderModule vertModule { storedVertModule };

        if (hasOwnVertexStage)
        {
            vertModule = VulkanPipelines::createShaderModule (
                device.getDevice(), static_cast<const char*> (pass.vertexSpirv.getData()),
                static_cast<int> (pass.vertexSpirv.getSize()));
            built = vertModule != nullptr;
        }

        if (built)
        {
            outTarget.pipeline = createFullscreenPipeline (vertModule, fragModule, offscreenRenderPass,
                                                            vk::SampleCountFlagBits::e1,
                                                            VulkanPipelines::opaqueBlendAttachment(), passPipelineLayout,
                                                            hasOwnVertexStage);
            built = outTarget.pipeline != nullptr;
        }

        // This target's pipeline is built once, eagerly, against the single
        // shared offscreen render pass — no second target ever requests this
        // same bytecode again, so the module is destroyed immediately after
        // pipeline creation (createCompositePipeline()'s exact
        // destroy-after-use convention).
        device.getDevice().destroyShaderModule (fragModule, nullptr);

        // Only destroy vertModule when THIS call actually created a transient
        // one above — storedVertModule is long-lived, owned by VulkanGraphics
        // (jam_VulkanGraphics.cpp), and must never be destroyed here.
        if (hasOwnVertexStage)
            device.getDevice().destroyShaderModule (vertModule, nullptr);
    }

    return built;
}

vk::Pipeline VulkanShaderInstance::createFullscreenPipeline (vk::ShaderModule vertModule, vk::ShaderModule fragModule,
                                                        vk::RenderPass targetRenderPass,
                                                        vk::SampleCountFlagBits sampleCount,
                                                        const vk::PipelineColorBlendAttachmentState& blendAttachment,
                                                        vk::PipelineLayout pipelineLayout,
                                                        bool hasOwnVertexStage) const
{
    const vk::PipelineShaderStageCreateInfo stages[2] {
        { {}, vk::ShaderStageFlagBits::eVertex, vertModule, "main" },
        { {}, vk::ShaderStageFlagBits::eFragment, fragModule, "main" }
    };

    const FullscreenPipelineFixedFunctionState fixedState {
        createFullscreenPipelineFixedFunctionState (sampleCount, hasOwnVertexStage) };

    const vk::PipelineVertexInputStateCreateInfo vertexInputInfo {
        buildFullscreenVertexInputInfo (fixedState, hasOwnVertexStage) };
    const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo { {}, fixedState.topology };

    // Blend contract chosen by the caller per target (see this function's doc
    // comment, jam_VulkanShaderInstance.h) — this function itself stays
    // agnostic to which one it was handed.
    vk::PipelineColorBlendStateCreateInfo colorBlendInfo {};
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &blendAttachment;

    const vk::PipelineDynamicStateCreateInfo dynamicStateInfo { {}, fixedState.dynamicStates };

    const vk::GraphicsPipelineCreateInfo pipelineInfo { buildFullscreenPipelineCreateInfo (
        stages, fixedState, vertexInputInfo, inputAssemblyInfo, colorBlendInfo, dynamicStateInfo,
        pipelineLayout, targetRenderPass) };

    vk::Pipeline pipeline {};
    const vk::Result createPipelineResult { device.getDevice().createGraphicsPipelines (
        nullptr, 1, &pipelineInfo, nullptr, &pipeline) };
    jassert (createPipelineResult == vk::Result::eSuccess);
    juce::ignoreUnused (createPipelineResult);

    return pipeline;
}

vk::PipelineVertexInputStateCreateInfo VulkanShaderInstance::buildFullscreenVertexInputInfo (
    const FullscreenPipelineFixedFunctionState& fixedState, bool hasOwnVertexStage) const
{
    // hasOwnVertexStage false (Shadertoy): zero vertex bindings/attributes,
    // VulkanPipelines::fullscreenTriangleVertexCount vertices generated purely from
    // gl_VertexIndex — today's exact, unchanged behavior. true (slang): a
    // real slang vertex stage rejects an empty vertex input state outright —
    // binds fixedState's OWN slangQuadBinding/slangQuadAttributes storage.
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};

    if (hasOwnVertexStage)
    {
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &fixedState.slangQuadBinding;
        vertexInputInfo.vertexAttributeDescriptionCount = 2;
        vertexInputInfo.pVertexAttributeDescriptions = fixedState.slangQuadAttributes.data();
    }

    return vertexInputInfo;
}

vk::GraphicsPipelineCreateInfo VulkanShaderInstance::buildFullscreenPipelineCreateInfo (
    const vk::PipelineShaderStageCreateInfo (&stages)[2], const FullscreenPipelineFixedFunctionState& fixedState,
    const vk::PipelineVertexInputStateCreateInfo& vertexInputInfo,
    const vk::PipelineInputAssemblyStateCreateInfo& inputAssemblyInfo,
    const vk::PipelineColorBlendStateCreateInfo& colorBlendInfo,
    const vk::PipelineDynamicStateCreateInfo& dynamicStateInfo,
    vk::PipelineLayout pipelineLayout, vk::RenderPass targetRenderPass) const
{
    vk::GraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &fixedState.viewportInfo;
    pipelineInfo.pRasterizationState = &fixedState.rasterizerInfo;
    pipelineInfo.pMultisampleState = &fixedState.multisampleInfo;
    pipelineInfo.pDepthStencilState = &fixedState.depthStencilInfo;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = targetRenderPass;
    pipelineInfo.subpass = 0;

    return pipelineInfo;
}

VulkanShaderInstance::FullscreenPipelineFixedFunctionState VulkanShaderInstance::createFullscreenPipelineFixedFunctionState (
    vk::SampleCountFlagBits sampleCount, bool hasOwnVertexStage) const
{
    FullscreenPipelineFixedFunctionState state {};

    // Slang (hasOwnVertexStage true): a real slang vertex stage declares its
    // own two mandatory attributes (Position/TexCoord,
    // RetroArch's own "I/O interface variables") — binding 0 =
    // SlangQuadVertex's own layout (24-byte stride), matching
    // VulkanGraphics::slangQuadVertexBuffer's content (VulkanShaderInstance::
    // slangQuadVertices) exactly. Left at its own default (unused) for
    // Shadertoy — the caller only ever points at these when hasOwnVertexStage
    // is true.
    state.slangQuadBinding = { 0, sizeof (SlangQuadVertex), vk::VertexInputRate::eVertex };
    state.slangQuadAttributes = {{
        { 0, 0, vk::Format::eR32G32B32A32Sfloat, 0 },                // Position
        { 1, 0, vk::Format::eR32G32Sfloat, sizeof (float) * 4 }      // TexCoord
    }};

    state.topology = hasOwnVertexStage ? vk::PrimitiveTopology::eTriangleStrip : vk::PrimitiveTopology::eTriangleList;

    state.viewportInfo.viewportCount = 1;
    state.viewportInfo.scissorCount = 1;

    state.rasterizerInfo.polygonMode = vk::PolygonMode::eFill;
    state.rasterizerInfo.cullMode = vk::CullModeFlagBits::eNone;
    state.rasterizerInfo.frontFace = vk::FrontFace::eCounterClockwise;
    state.rasterizerInfo.lineWidth = 1.0f;

    state.multisampleInfo.rasterizationSamples = sampleCount;

    // A VulkanShader pass draw never touches depth or stencil — VulkanPipelines::noStencilState()'s
    // untested, unwritten state. Harmless against the offscreen/composite render passes
    // (no depth/stencil attachment; the state is simply ignored there) and required
    // against the scene render pass (jam_VulkanGraphicsSetupRenderPass.cpp's stencil
    // attachment) so its own clip-as-data stencil interaction — written and tested by
    // the Path pipelines, never by a shader draw — survives untouched underneath.
    state.depthStencilInfo = VulkanPipelines::noStencilState();

    state.dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    return state;
}

//==============================================================================
// Accessors
//==============================================================================

int VulkanShaderInstance::getChannelBindlessIndex (int channelIndex) const noexcept
{
    int index { -1 };

    if (channelIndex >= 0 and channelIndex < getBufferPassCount())
    {
        const auto& target { *bufferPassTargets.at (static_cast<size_t> (channelIndex)) };
        index = target.bindlessIndex.at (target.currentReadHalf);
    }

    return index;
}

void VulkanShaderInstance::stampChannels (VulkanShaderUniforms& uniforms, int32_t sceneOrNoSceneFallback, void* windowHandle) const noexcept
{
    for (int32_t ordinal = 0; ordinal < VulkanShaderUniforms::maxChannelCount; ++ordinal)
    {
        const int bufferBindlessIndex { getChannelBindlessIndex (ordinal) };
        uniforms.channels[ordinal] = bufferBindlessIndex >= 0 ? bufferBindlessIndex : sceneOrNoSceneFallback;
    }

    // Named external LUT/textures — each entry's own already-resolved
    // channel slot (externalTextureChannels) is overwritten with its own
    // VulkanBindlessTexture's per-window bindless index (externalTextures,
    // VulkanBindlessTexture::getBindlessIndex (windowHandle)) — a texture whose
    // decode/upload/slot-assignment failed (index -1) is simply skipped,
    // leaving that slot at whichever value the loop above already stamped
    // it to (graceful degradation — see this method's own doc comment).
    for (auto& [textureName, channelSlot] : externalTextureChannels)
    {
        if (externalTextures.contains (textureName))
        {
            const int lutBindlessIndex { externalTextures.at (textureName).getBindlessIndex (windowHandle) };

            if (lutBindlessIndex >= 0)
                uniforms.channels[channelSlot] = lutBindlessIndex;
        }
    }
}

VulkanShaderUniforms VulkanShaderInstance::stampUniforms (vk::Extent2D targetExtent, const std::array<float, 4>& mouse) noexcept
{
    const double nowMs { juce::Time::getMillisecondCounterHiRes() };
    static constexpr double millisecondsPerSecond { 1000.0 };
    static constexpr size_t iMouseComponentCount { 4 };

    VulkanShaderUniforms uniforms {};
    uniforms.iResolution[0] = static_cast<float> (targetExtent.width);
    uniforms.iResolution[1] = static_cast<float> (targetExtent.height);
    uniforms.iTime = static_cast<float> ((nowMs - startTimeMs) / millisecondsPerSecond);
    uniforms.iTimeDelta = static_cast<float> ((nowMs - lastStampTimeMs) / millisecondsPerSecond);
    uniforms.iFrame = frameCounter;

    for (size_t component = 0; component < iMouseComponentCount; ++component)
        uniforms.iMouse[component] = mouse.at (component);

    lastStampTimeMs = nowMs;
    ++frameCounter;

    return uniforms;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
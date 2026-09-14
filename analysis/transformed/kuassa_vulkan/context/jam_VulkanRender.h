/** @file jam_VulkanRender.h
 *  @brief render() — the shader draw injection seam into juce::Graphics.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief Emits shader's compiled draw through g's active VulkanLowLevelGraphicsContext,
 *  in paint order — the same way any other juce::Graphics draw call reaches
 *  this engine. shader's Image pass renders into its own offscreen gather
 *  target (VulkanShaderInstance::imagePassGatherTarget) first, then the background
 *  combine pass composites that target's result into g's current clip bounds
 *  in the scene (see VulkanLowLevelGraphicsContext::recordShaderImagePassDrawCommands()'s
 *  own doc comment, jam_VulkanLowLevelGraphicsContextRender.cpp).
 *
 *  Downcasts g's internal context to jam::VulkanLowLevelGraphicsContext and, on
 *  a match, forwards to its renderShader() entry method
 *  (jam_VulkanLowLevelGraphicsContextRender.cpp). A non-match is a silent no-op.
 *  @param g                The juce::Graphics currently painting.
 *  @param shader           The compiled shader to render.
 *  @param opacity          Caller's own blend opacity, [0, 1] (VulkanShader carries
 *                          no opacity field, see jam_VulkanShader.h's VulkanShader
 *                          doc comment).
 *  @param resolutionScale  Caller's own intermediate-pass extent fraction,
 *                          [0, 1] (likewise not a VulkanShader field).
 *  @param mouse            Caller's own iMouse value, sign-encoded per
 *                          Shadertoy convention (see VulkanShaderInstance::
 *                          stampUniforms()'s own doc comment,
 *                          resource/jam_VulkanShaderInstance.h) — forwarded
 *                          verbatim to VulkanLowLevelGraphicsContext::renderShader().
 *  @param camera           Caller's own jam::VulkanOrbitCamera — forwarded
 *                          verbatim to VulkanLowLevelGraphicsContext::renderShader(),
 *                          read only when shader carries a mesh. Mirrors
 *                          @p mouse's own "always supplied, read only if the
 *                          shader cares" contract — jam::
 *                          VulkanShaderComponent owns one unconditionally.
 */
inline void render (juce::Graphics& g, const VulkanShader& shader, float opacity, float resolutionScale,
                    const std::array<float, 4>& mouse, const VulkanOrbitCamera& camera)
{
    if (auto* vulkanContext { dynamic_cast<VulkanLowLevelGraphicsContext*> (&g.getInternalContext()) })
        vulkanContext->renderShader (shader, opacity, resolutionScale, mouse, camera);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
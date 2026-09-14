/** @file jam_VulkanShaderTextureBindings.h
 *  @brief Resolved GPU resources for one slang pass's own reflected texture
 *         names — the Part 1 output of VulkanGraphics::getSlangTextureBindings(),
 *         directly consumable by VulkanShaderReflection::populateUniformBuffer()/
 *         populatePushConstantBuffer() and by a slang pass's own per-frame
 *         descriptor-image rewrite.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief One slang pass's reflected texture names (VulkanShaderReflection::
 *  TextureResource::name — "PassOutputN"/"PassFeedbackN"/"Source"/"Original")
 *  resolved to their actual GPU resources for THIS draw: the already-bindless-
 *  registered vk::ImageView a fresh descriptor write should point at, and the
 *  pixel extent VulkanShaderReflection's "<X>Size" member population needs.
 *
 *  Built fresh every frame a slang pass draws (VulkanGraphics::
 *  getSlangTextureBindings()) — never cached: a name resolving to a
 *  buffer-pass ping-pong target's CURRENT readable half changes identity
 *  every frame that buffer pass records (VulkanRenderResources::currentReadHalf),
 *  so a stale VulkanShaderTextureBindings would hand a slang pass's descriptor
 *  rewrite a no-longer-current vk::ImageView.
 *
 *  A name absent from either map (an unresolved texture reference — e.g.
 *  "Source"/"Original" requested by a background-mode shader, which has no
 *  resolved scene to offer) is left unresolved — no entry, no error, no
 *  assert: mirrors VulkanShaderReflection::populateMemberBuffer()'s own "unknown
 *  member stays zero, no failure" graceful-degradation contract. The
 *  caller's descriptor-rewrite loop and populateUniformBuffer()/
 *  populatePushConstantBuffer()'s own "<X>Size" lookup both already tolerate
 *  a missing key this same way.
 */
struct VulkanShaderTextureBindings
{
    /** @brief Resolved name -> vk::ImageView, for a slang pass's per-frame
     *  descriptor-image rewrite (VulkanGraphics::getOrCreateLinearSampler() is the
     *  fixed sampler paired with every entry here — see
     *  VulkanShaderInstance::refreshSlangPass()'s doc comment for why no per-
     *  texture filter choice is threaded through this sprint). */
    jam::HashMap<juce::String, vk::ImageView> views;

    /** @brief Resolved name -> pixel extent, threaded directly into
     *  VulkanShaderReflection::populateUniformBuffer()/populatePushConstantBuffer()'s
     *  own textureExtents parameter — the same resolved dimensions the
     *  matching views entry's own vk::ImageView was built at. */
    jam::HashMap<juce::String, vk::Extent2D> extents;
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
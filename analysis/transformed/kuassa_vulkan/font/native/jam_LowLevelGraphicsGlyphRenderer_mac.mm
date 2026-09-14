// macOS native clip query for jam::VulkanEngine::createContext()'s CPU lane —
// needs actual Objective-C syntax ([NSGraphicsContext currentContext]), so it
// cannot live in a plain .cpp. Compiled only under JUCE_MAC (jam_vulkan.mm),
// mirroring jam_VulkanMetal_mac.mm's identical split for
// createMetalLayerForView()/getScaleFactor()/updateMetalLayerFrame().

juce::Rectangle<int> jam::currentPeerDirtyBounds (juce::ComponentPeer& peer)
{
    NSGraphicsContext* nsContext { [NSGraphicsContext currentContext] };
    jassert (nsContext != nullptr);

    CGContextRef cg { nsContext.CGContext };
    const CGRect clipBox { CGContextGetClipBoundingBox (cg) };

    // JUCE's NSView is flipped (isFlipped == true), so CG user space here is
    // already top-left-origin, y-down — the same space clipBox lands in
    // untouched. createContext()'s own CTM concat on this same CGContextRef
    // runs after this read and exists solely to pre-compensate
    // CoreGraphicsContext's internal flip, not to correct clipBox.
    const juce::Rectangle<float> clip (
        static_cast<float> (clipBox.origin.x),
        static_cast<float> (clipBox.origin.y),
        static_cast<float> (clipBox.size.width),
        static_cast<float> (clipBox.size.height));

    const auto rounded { clip.getSmallestIntegerContainer() };

    return rounded;
}

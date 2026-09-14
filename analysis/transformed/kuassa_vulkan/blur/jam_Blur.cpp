namespace jam
{

const juce::Image& Blur::render (const juce::Image& source)
{
    jassert (source.isValid());

    juce::ImagePixelData* const sourceRoot { jam::VulkanGraphics::getImageSourceRoot (source).root };

    if (sourceRoot != boundSourceRoot)
    {
        bindSource (sourceRoot);
        dirty = true;
    }

    if (dirty)
    {
        jassert (VulkanEngine::getInstance() != nullptr);

        VulkanEngine::getInstance()->blurImage (destination, source, radius);
        dirty = false;
    }

    return destination;
}

} // namespace jam

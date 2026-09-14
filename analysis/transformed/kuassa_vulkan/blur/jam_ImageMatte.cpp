namespace jam
{

void ImageMatte::updateMatte()
{
    if (clipImage.isValid() and not clipImage.getBounds().isEmpty())
    {
        auto* engine { VulkanEngine::getInstance() };

        if (featherRadius > 0.0f)
        {
            if (engine != nullptr)
                engine->featherImage (matte, clipImage, featherRadius, featherCurve);
            else
            {
                matte = clipImage.createCopy();
                juce::Image::BitmapData matteData { matte, juce::Image::BitmapData::readWrite };
                Graphics::applyMatteFeather (matteData, featherRadius, featherCurve);
            }
        }
        else
        {
            matte = clipImage;
        }
    }
}

void ImageMatte::applyEffect (juce::Image& sourceImage, juce::Graphics& destContext, float, float alpha)
{
    if (matte.isValid())
    {
        if (auto* vulkan { VulkanEngine::getInstance() }; vulkan != nullptr and vulkan->isGpuAvailable())
        {
            juce::Graphics::ScopedSaveState saveState (destContext);
            const auto matteTransform { juce::AffineTransform::scale (
                static_cast<float> (sourceImage.getWidth())  / static_cast<float> (matte.getWidth()),
                static_cast<float> (sourceImage.getHeight()) / static_cast<float> (matte.getHeight())) };
            destContext.reduceClipRegion (matte, matteTransform);
            destContext.setOpacity (alpha);
            destContext.drawImageAt (sourceImage, 0, 0);
        }
        else
        {
            Graphics::applyMatte (destination, sourceImage, matte);
            destContext.setOpacity (alpha);
            destContext.drawImageAt (destination, 0, 0);
        }
    }
    else
    {
        destContext.setOpacity (alpha);
        destContext.drawImageAt (sourceImage, 0, 0);
    }
}

} // namespace jam

namespace jam
{
/*____________________________________________________________________________*/

static constexpr int fullCoverage { 255 };

void InnerShadow::invertCoverage (juce::Image& image)
{
    jassert (image.getFormat() == juce::Image::SingleChannel);

    juce::Image::BitmapData data { image, juce::Image::BitmapData::readWrite };

    for (int y { 0 }; y < data.height; ++y)
    {
        uint8_t* line { data.getLinePointer (y) };

        for (int x { 0 }; x < data.width; ++x)
            line[x] = static_cast<uint8_t> (fullCoverage - line[x]);
    }
}

void InnerShadow::render (juce::Graphics& g, const juce::Path& path)
{
    if (mask.update (path))
    {
        invertCoverage (mask.image);

        juce::Image::BitmapData maskData { mask.image, juce::Image::BitmapData::readWrite };
        jam::stackBlurSingleChannel (maskData, mask.radius);
    }

    juce::Graphics::ScopedSaveState savedState (g);

    g.reduceClipRegion (path);
    g.setColour (colour);
    g.drawImageAt (mask.image, mask.maskOrigin.x + offset.x, mask.maskOrigin.y + offset.y, true);
}

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */

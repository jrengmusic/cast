namespace jam
{
/*____________________________________________________________________________*/

void DropShadow::render (juce::Graphics& g, const juce::Path& path)
{
    if (mask.update (path))
    {
        juce::Image::BitmapData maskData { mask.image, juce::Image::BitmapData::readWrite };
        jam::stackBlurSingleChannel (maskData, mask.radius);
    }

    g.setColour (colour);
    g.drawImageAt (mask.image, mask.maskOrigin.x + offset.x, mask.maskOrigin.y + offset.y, true);
}

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */

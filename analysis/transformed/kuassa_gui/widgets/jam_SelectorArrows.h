/**
 * @file jam_SelectorArrows.h
 * @brief Cached SVG path data for the Selector/ComboBox open/closed arrow glyph.
 */

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Static namespace-like accessor for the closed/open arrow glyph SVG path data. */
struct SelectorArrows
{
    /** @return SVG path data for the closed (collapsed) arrow glyph, loaded once and cached. */
    static const juce::String& closed()
    {
        static auto path { loadPath ("closed") };
        return path;
    }

    /** @return SVG path data for the open (expanded) arrow glyph, loaded once and cached. */
    static const juce::String& open()
    {
        static auto path { loadPath (Id::open) };
        return path;
    }

private:
    static juce::String loadPath (juce::StringRef elementId)
    {
        juce::String result;

        if (auto svg { jam::Xml::getFromBinary (files::selectorArrows) })
        {
            for (auto* child : svg->getChildIterator())
            {
                if (child->getStringAttribute (Id::id) == elementId)
                {
                    result = child->getStringAttribute (Id::d);
                    break;
                }
            }
        }

        return result;
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam

namespace jam
{
/*____________________________________________________________________________*/

//==============================================================================
/** @brief Binary-resource fetcher signature -- resolves a named resource to a (pointer, size) pair. */
using ResourceFetcher = std::pair<const void*, int> (*) (const char*);

/** @brief Ordered set of ResourceFetcher entries StyleManager tries, in order, when loading a font file. */
using ResourceFetchers = std::vector<ResourceFetcher>;

/**
 * @brief Resolves a typeface from an embedded binary resource.
 *
 * @param fontFileName The font filename to resolve, as looked up via fetcher.
 * @param fetcher The binary resource fetcher supplying (pointer, size) for fontFileName.
 * @return The resolved typeface, or the default LookAndFeel typeface if the resource is unavailable.
 */
juce::Typeface::Ptr getFontFromBinary (const juce::String& fontFileName,
                                       ResourceFetcher fetcher);

/**
 * @class StyleManager
 * @brief Runtime style registry parsed from a CSS stylesheet -- fonts, colours, metrics, and window styles.
 *
 * @details Binds to the process-static typed stylesheet Document returned by Css::getOrCreate()
 *          for Id::styleSheet. Fonts and colours declared in the stylesheet are registered
 *          into runtime lookup tables (fonts, colours) at construction; metrics and window
 *          styles are read directly from the typed sections on demand. Appearance
 *          (light/dark) selects which colour alias backs each JUCE colour ID via the appearance map.
 */
class StyleManager : public Instance<StyleManager>
{
public:
    /**
     * @brief Constructs a StyleManager over the stylesheet Document, registering fonts, colours,
     * and appearance aliases from the mermaid contract sheet first, then the project stylesheet --
     * project registrations overwrite the contract's under the same alias.
     *
     * @param newFetchers The ordered ResourceFetchers tried, in order, when resolving font binaries.
     */
    explicit StyleManager (ResourceFetchers newFetchers);

    //==============================================================================
    /**
     * @brief Retrieves the registered juce::FontOptions for a font alias.
     *
     * @param fontAlias The font alias registered via the stylesheet's fonts section.
     * @return The registered FontOptions, or a default-height fallback if the alias is unknown.
     * @note Asserts (jassertfalse) when the alias has no registered font.
     */
    juce::FontOptions getFont (juce::StringRef fontAlias) const;

    /**
     * @brief Retrieves a registered colour by its stylesheet colour alias.
     *
     * @param colourAlias The colour alias registered via the stylesheet's colours section.
     * @return The registered Colour, or juce::Colours::magenta if the alias is unknown.
     */
    juce::Colour getColour (const juce::Identifier& colourAlias) const;

    /**
     * @brief Applies the light/dark appearance section, then the general appearance section, to a
     * LookAndFeel -- from the project stylesheet.
     *
     * @param laf The LookAndFeel to receive the resolved colours.
     * @param lightOrDark The appearance variant ID (e.g. light or dark) selecting the overriding section.
     */
    void setAppearance (juce::LookAndFeel& laf,
                        juce::StringRef lightOrDark);

    static bool isDark (juce::StringRef appearance);

    /**
     * @brief Applies the light/dark appearance section, then the general appearance section, to a
     * LookAndFeel -- from the mermaid contract sheet first, then the project stylesheet.
     *
     * @param styleSheet The lexicon-registered CSS resource name resolved via Css::getOrCreate() and applied before the project stylesheet.
     * @param laf The LookAndFeel to receive the resolved colours.
     * @param lightOrDark The appearance variant ID (e.g. light or dark) selecting the overriding section.
     */
    void setAppearanceForStyle (juce::StringRef styleSheet, juce::LookAndFeel& laf, juce::StringRef lightOrDark);

    /**
     * @brief Applies a per-widget-style colour section, identified by styleId, to a LookAndFeel --
     * from the project stylesheet.
     *
     * @param styleId The widget style ID identifying the colour section in the typed style section.
     * @param lookAndFeel The LookAndFeel to receive the resolved colours.
     */
    void setColourForStyle (juce::StringRef styleId, juce::LookAndFeel* lookAndFeel);

    /** @brief Registers a stylesheet by name, resolved via Css::getOrCreate(), into fonts,
     *  colours, and appearance -- overwriting any existing registration under the same key. */
    void registerStyle (juce::StringRef styleSheet);

    /** @brief Registers every entry of colourIds into colourScheme. */
    void registerColourIds (const jam::HashMap<int, juce::Identifier>& colourIds);

    /**
     * @brief Iterates all registered font aliases and dispatches each to a LookAndFeel
     *        via the provided setFont map.
     *
     * Keeps the font map private — callers supply the dispatch map; StyleManager
     * owns the iteration.
     *
     * @param laf      The LookAndFeel target to receive the font assignments.
     * @param styleId  The component/style ID used as the key in setFontMap.
     * @param setFontMap  The Registry setFont Function::Map to dispatch through.
     */
    void applyFontsTo (juce::LookAndFeel* laf,
                       juce::StringRef styleId,
                       jam::Function::Map<juce::String, void>& setFontMap);

    /**
     * @brief Reads a numeric metric from the stylesheet's metrics section.
     *
     * @tparam NumberType The numeric type to return (integer or floating-point).
     * @param metricId The metric ID looked up in the metrics section.
     * @return The metric value cast to NumberType, or a default-constructed NumberType if metricId is unregistered.
     */
    template <typename NumberType = int>
    NumberType getMetrics (juce::StringRef metricId) const noexcept
    {
        if (jam::Map::contains (metrics, metricId))
        {
            auto& [alias, value] { *metrics.find (metricId) };
            return static_cast<NumberType> (value);
        }

        return {};
    }

    /**
     * @brief Reads a numeric style value from the WINDOW section.
     *
     * @tparam NumberType The numeric type to return (integer or floating-point).
     * @param styleId The style ID looked up in the WINDOW section.
     * @return The style value cast to NumberType, or a default-constructed NumberType if styleId is unregistered.
     * @note Asserts (jassertfalse) when styleId cannot be resolved.
     */
    template <typename NumberType = int>
    NumberType getWindowStyle (juce::StringRef styleId) const noexcept
    {
        auto* windowSection { style.root->getChildByID (Id::window) };

        const juce::Identifier leaf { juce::String { styleId } };

        if (windowSection->contains (leaf))
            return static_cast<NumberType> (*windowSection->get<double> (leaf));

        return {};
    }

    /**
     * @brief Reads a string style value from the WINDOW section.
     *
     * @param styleId The style ID looked up in the WINDOW section.
     * @return The style value string, or an empty string if styleId is unregistered.
     */
    juce::String getWindowStyleString (juce::StringRef styleId) const noexcept
    {
        auto* windowSection { style.root->getChildByID (Id::window) };

        const juce::Identifier leaf { juce::String { styleId } };

        if (windowSection->contains (leaf))
            return *windowSection->get<juce::String> (leaf);

        return {};
    }

    /**
     * @brief Reads a string style leaf from the fonts section.
     *
     * @param styleId The leaf name looked up on the fonts section.
     * @return The leaf value string, or an empty string if unregistered.
     */
    juce::String getFontsStyleString (juce::StringRef styleId) const noexcept
    {
        if (auto* fontsSection { style.root->getChildByID (Id::fonts) }; fontsSection != nullptr)
        {
            const juce::Identifier leaf { juce::String { styleId } };

            if (fontsSection->contains (leaf))
                return *fontsSection->get<juce::String> (leaf);
        }

        return {};
    }

    /**
     * @brief Checks whether the fonts section carries a given style leaf.
     *
     * @param styleId The leaf name to check on the fonts section.
     * @return True if the leaf is present, false otherwise.
     */
    bool hasFontsStyle (juce::StringRef styleId) const noexcept
    {
        if (auto* fontsSection { style.root->getChildByID (Id::fonts) }; fontsSection != nullptr)
            return fontsSection->contains (juce::Identifier { juce::String { styleId } });

        return false;
    }

    /**
     * @brief Reads a numeric style leaf from the fonts section.
     *
     * @tparam NumberType The numeric type to return (integer or floating-point).
     * @param styleId The leaf name looked up on the fonts section.
     * @return The leaf value cast to NumberType, or a default-constructed NumberType if unregistered.
     * @note Asserts (jassertfalse) when styleId is not present on the fonts section.
     */
    template <typename NumberType = int>
    NumberType getFontsStyle (juce::StringRef styleId) const noexcept
    {
        if (auto* fontsSection { style.root->getChildByID (Id::fonts) }; fontsSection != nullptr)
        {
            const juce::Identifier leaf { juce::String { styleId } };

            if (fontsSection->contains (leaf))
                return static_cast<NumberType> (*fontsSection->get<double> (leaf));
        }

        jassertfalse;
        return {};
    }

    //==============================================================================

#if JUCE_DEBUG
    /**
     * @brief Prints the list of registered fonts to standard output.
     *
     * This debug method outputs the names of all registered fonts to
     * standard output for verification purposes.
     */
    void printRegisteredFonts() const;

    /**
     * @brief Prints the list of registered colors to standard output.
     *
     * This debug method outputs the names of all registered colors to
     * standard output for verification purposes.
     */
    void printRegisteredColours() const;
#endif// JUCE_DEBUG

private:
    //==============================================================================
    /**
     * @brief Registers every font record of the fonts section into fonts, pushing each resolved
     *        typeface into the engine's typeface cache.
     *
     * For each font record, tries fetchers in order until one resolves the record's file to a
     * typeface; on the first success, registers the typeface into fonts (see registerFont()) and
     * pushes the same fetcher's raw binary onto VulkanEngine::getInstance()'s typeface cache via
     * registerTypeface().
     */
    void registerFonts (const Document::Element& fontsSection);

    /** @brief Registers one resolved typeface under fontAlias, applying height and kerning;
     *  overwrites any existing registration under the same alias. */
    void registerFont (const juce::String& fontAlias,
                       juce::Typeface::Ptr typeface,
                       float height,
                       float kerningFactor);

    /** @brief Registers every entry of document's colours section into colours,
     *  overwriting any existing registration under the same alias. */
    void registerColours (const Document& document);

    /** @brief Registers every entry of document's metrics section into metrics,
     *  overwriting any existing registration under the same alias. */
    void registerMetrics (const Document& document);

    /** @brief Registers one resolved colour under colourAlias; overwrites any existing
     *  registration under the same alias. */
    void registerColour (const juce::Identifier& colourAlias, juce::Colour colour);

    /** @brief Registers document's fonts, colours, and appearance aliases into fonts,
     *  colours, and appearance -- overwriting any existing registration under the same key. */
    void registerStyle (const Document& document);

    /** @brief Applies document's light/dark appearance section, then its general appearance
     *  section, to laf. */
    void applyDocumentAppearance (const Document& document,
                                  juce::LookAndFeel& laf,
                                  juce::StringRef lightOrDark);

    /** @brief Applies document's per-widget-style colour section, identified by styleId, to
     *  lookAndFeel. */
    void applyDocumentStyle (const Document& document,
                             juce::StringRef styleId,
                             juce::LookAndFeel* lookAndFeel);

    /** @brief Finds the source font filename registered under fontAlias. */
    juce::String getFontFilename (const juce::String& fontAlias) const;

    /**
     * @brief Looks up the JUCE colour ID integer registered under a css custom-property word.
     *
     * @param colourId The css custom-property word (as parsed into the CSS colours section).
     * @return The matching juce::LookAndFeel colour ID, or -1 if unregistered.
     */
    int getColourId (const juce::Identifier& colourId) const;

    //==============================================================================
    const Document& style;
    ResourceFetchers fetchers;
    jam::HashMap<juce::String, juce::FontOptions> fonts;
    jam::HashMap<juce::Identifier, juce::Colour> colours;
    jam::HashMap<juce::String, double> metrics;

    /** @brief Maps a lexicon colour-id string to the colour alias active under the current appearance. */
    jam::HashMap<juce::String, juce::String> appearance;

    jam::HashMap<juce::Identifier, int> colourScheme;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleManager)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */

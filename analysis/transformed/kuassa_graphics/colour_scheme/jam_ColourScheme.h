namespace jam
{
/*____________________________________________________________________________*/

struct ColourScheme : public AnyMap<>
{
    static ColourScheme fromValueTree (const juce::ValueTree& tree) noexcept
    {
        ColourScheme map;

        for (const auto& child : tree)
            map.add<ColourScheme> (child.getType(), fromValueTree (child));

        return map;
    }

    ColourScheme* getChildWithName (const juce::Identifier& name) noexcept
    {
        for (auto& [key, ptr] : entries)
        {
            if (isType<ColourScheme> (key))
            {
                auto* child { static_cast<ColourScheme*> (ptr.get()) };

                if (key == name)
                    return child;

                if (auto* found { child->getChildWithName (name) })
                    return found;
            }
        }

        return nullptr;
    }

    const ColourScheme* getChildWithName (const juce::Identifier& name) const noexcept
    {
        return const_cast<ColourScheme*> (this)->getChildWithName (name);
    }

    void
    addColourId (const juce::Identifier& treeType, const juce::Identifier& property, int colourId)
    {
        ColourScheme* tree { getChildWithName (treeType) };
        assert (tree != nullptr
                and "addColourId: treeType not found in colour scheme — call fromValueTree first");
        tree->add<int> (property, colourId);
    }

    void applyColours (juce::LookAndFeel& laf, const juce::ValueTree& root)
    {
        applyColours (laf, root, *this);
    }

    bool contains (const juce::ValueTree& tree, const juce::Identifier& property) const
    {
        const ColourScheme* colours { getChildWithName (tree.getType()) };
        return colours != nullptr and colours->isType<int> (property);
    }

    /** @brief Extracts a uint32 from a juce::var (int64 → uint32 cast).
     *  @param v  var holding an int64 value.
     *  @return   Truncated uint32.
     */
    static juce::uint32 toInt (const juce::var& v) noexcept
    {
        return static_cast<juce::uint32> (static_cast<juce::int64> (v));
    }

    /** @brief Extracts a juce::Colour from a juce::var (int64 → ARGB uint32 → Colour).
     *  @param v  var holding an ARGB colour as int64.
     *  @return   juce::Colour.
     */
    static juce::Colour toColour (const juce::var& v) noexcept
    {
        return juce::Colour { toInt (v) };
    }

private:
    void applyColours (juce::LookAndFeel& laf, const juce::ValueTree& tree, ColourScheme& colours)
    {
        for (int i { 0 }; i < tree.getNumProperties(); ++i)
        {
            const auto property { tree.getPropertyName (i) };

            if (colours.isType<int> (property))
                laf.setColour (*colours.get<int> (property),
                                toColour (tree.getProperty (property)));
        }

        for (const auto& child : tree)
        {
            if (colours.isType<ColourScheme> (child.getType()))
                applyColours (laf, child, *colours.get<ColourScheme> (child.getType()));
        }
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam

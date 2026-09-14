/**
 * @file jam_ParameterLayout.h
 * @brief Builds the plugin's juce::AudioProcessorValueTreeState parameter
 *        layout from parameters.md's `parameter` table.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ParameterLayout
 * @brief Static builder for the automatable parameter layout. parameters.md's
 *        `parameter` table is the single source of truth: row order is host
 *        enumeration order, and the `type` cell (float/choice/bool) selects
 *        the AudioParameter type and dispatch arm for each row.
 */
struct ParameterLayout
{
    /**
     * @class Group
     * @brief Owns one juce::AudioProcessorParameterGroup per distinct
     *        `group` cell in parameters.md, keyed by group name.
     */
    struct Group : public jam::HashMap<juce::String, std::unique_ptr<juce::AudioProcessorParameterGroup>>
    {
        /**
         * @brief Creates one parameter group per distinct `group` cell in
         *        the `parameter` table.
         * @param parameters  Parsed parameters.md document.
         */
        explicit Group (const MarkdownDocument& parameters)
        {
            for (auto* row : parameters.getTableRows (Id::parameter))
            {
                const auto group { parameters.getTableValue (*row, Id::group) };
                auto [entry, inserted] { try_emplace (group) };
                auto& [name, parameterGroup] { *entry };

                if (inserted)
                    parameterGroup = std::make_unique<juce::AudioProcessorParameterGroup> (Format::toScreamingSnakeCase (group), Format::toTitleCase (group), juce::String::charToString (Chars::pipe));
            }
        }
    };

    /** @return The row's `choices` cell, tokenised on commas and trimmed. */
    static juce::StringArray getChoices (const MarkdownDocument& parameters, Document::Element& row)
    {
        juce::StringArray choices;
        choices.addTokens (parameters.getTableValue (row, Id::choices), ",", "");
        choices.trim();
        return choices;
    }

    /**
     * @brief Builds the parameter's display name: the title-cased `name`
     *        cell alone for the master group, otherwise prefixed with its
     *        title-cased `group` cell.
     * @param parameters  Parsed parameters.md document.
     * @param row         `parameter` table row.
     * @return The parameter's display name.
     */
    static juce::String getParameterName (const MarkdownDocument& parameters, Document::Element& row)
    {
        juce::String param { Format::toTitleCase (parameters.getTableValue (row, Id::name)) };
        juce::String group { Format::toTitleCase (parameters.getTableValue (row, Id::group)) };
        if (group.equalsIgnoreCase (Id::toTag (Id::master))) return Format::correctKeywordCase (param);
        return Format::correctKeywordCase (group + " " + param);
    }

    /** @return The row's `id` cell, normalised to a SCREAMING_SNAKE_CASE parameter ID. */
    static juce::String getParameterID (const MarkdownDocument& parameters, Document::Element& row)
    {
        return Format::toScreamingSnakeCase (parameters.getTableValue (row, Id::id));
    }

    /** @return Attributes for an AudioParameterFloat built from the row, with its stringFromValue() function. */
    static juce::AudioParameterFloatAttributes getFloatAttributes (const MarkdownDocument& parameters, Document::Element& row)
    {
        return juce::AudioParameterFloatAttributes()
            .withCategory (juce::AudioProcessorParameter::genericParameter)
            .withStringFromValueFunction (stringFromValue<float> (parameters, row, map::AudioParameter::floatingPoint));
    }

    /** @return Attributes for an AudioParameterInt built from the row, with its stringFromValue() function. */
    static juce::AudioParameterIntAttributes getIntAttributes (const MarkdownDocument& parameters, Document::Element& row)
    {
        return juce::AudioParameterIntAttributes()
            .withCategory (juce::AudioProcessorParameter::genericParameter)
            .withStringFromValueFunction (stringFromValue<int> (parameters, row, map::AudioParameter::integer));
    }

    /** @return Attributes for an AudioParameterChoice built from the row, with its stringFromValue() function. */
    static juce::AudioParameterChoiceAttributes getChoiceAttributes (const MarkdownDocument& parameters, Document::Element& row)
    {
        return juce::AudioParameterChoiceAttributes()
            .withCategory (juce::AudioProcessorParameter::genericParameter)
            .withStringFromValueFunction (stringFromValue<int> (parameters, row, map::AudioParameter::choice));
    }

    /** @return Attributes for an AudioParameterBool built from the row, with its stringFromValue() function. */
    static juce::AudioParameterBoolAttributes getBoolAttributes (const MarkdownDocument& parameters, Document::Element& row)
    {
        return juce::AudioParameterBoolAttributes()
            .withCategory (juce::AudioProcessorParameter::genericParameter)
            .withStringFromValueFunction (stringFromValue<bool> (parameters, row, map::AudioParameter::boolean));
    }

    /**
     * @brief Builds a parameter-ID to value map from one `parameter` table
     *        column, converting each row's cell to `ValueType`.
     * @tparam ValueType    Value type to convert each cell to (int, double, juce::String, or bool).
     * @param parameters    Parsed parameters.md document.
     * @param propertyName  Column to read (e.g. `Id::defaultValue`, `Id::unit`).
     * @return The parameter ID to value map.
     */
    template <typename ValueType>
    static ValueMap getValueMap (const MarkdownDocument& parameters, const juce::Identifier& propertyName) noexcept
    {
        ValueMap d;

        auto addValue = [&d, &parameters, &propertyName] (Document::Element& row)
        {
            auto text { parameters.getTableValue (row, propertyName) };
            juce::var value;

            if constexpr (std::is_same_v<int, ValueType>) value = text.getIntValue();
            else if constexpr (std::is_same_v<double, ValueType>) value = text.getDoubleValue();
            else if constexpr (std::is_same_v<juce::String, ValueType>) value = text;
            else if constexpr (std::is_same_v<bool, ValueType>) value = text.getIntValue() != 0;

            d.insert ({ getParameterID (parameters, row), value });
        };

        for (auto* row : parameters.getTableRows (Id::parameter))
            addValue (*row);

        return d;
    }

    /** @return The parameter ID to default-value map, read from the `defaultValue` column. */
    static ValueMap getDefaultValueMap (const MarkdownDocument& parameters) noexcept { return getValueMap<double> (parameters, Id::defaultValue); }
    /** @return The parameter ID to unit-string map, read from the `unit` column. */
    static ValueMap getUnitMap (const MarkdownDocument& parameters) noexcept { return getValueMap<juce::String> (parameters, Id::unit); }

    /**
     * @brief Builds the value-to-display-string function for one parameter,
     *        using its `minLabel`/`maxLabel` cells at the range extremes and
     *        falling back to the on/off labels, a numeric value plus unit,
     *        or the matching choice, by parameter type.
     * @tparam ValueType  Parameter's underlying value type (float, int, or bool).
     * @param parameters  Parsed parameters.md document.
     * @param row         `parameter` table row.
     * @param type        Parameter type, one of the map::AudioParameter constants.
     * @return The value-to-display-string function, or `nullptr` for an unsupported type.
     */
    template <typename ValueType>
    static std::function<juce::String (ValueType value, int maximumStringLength)>
        stringFromValue (const MarkdownDocument& parameters, Document::Element& row, int type)
    {
        auto minLabel { parameters.getTableValue (row, Id::minLabel) };
        auto maxLabel { parameters.getTableValue (row, Id::maxLabel) };

        if constexpr (std::is_same_v<ValueType, bool>)
        {
            static const juce::StringArray booleanLabels { Id::off.toString().toUpperCase(), Id::on.toString().toUpperCase() };

            return [minLabel, maxLabel] (ValueType value, int maximumStringLength)
            {
                if (value) { if (maxLabel.isNotEmpty()) return maxLabel; }
                else       { if (minLabel.isNotEmpty()) return minLabel; }
                return booleanLabels[toInt (value)];
            };
        }
        else
        {
            auto min { static_cast<ValueType> (parameters.getTableValue (row, Id::min).getDoubleValue()) };
            auto max { static_cast<ValueType> (parameters.getTableValue (row, Id::max).getDoubleValue()) };
            auto unit { Format::toUnit (parameters.getTableValue (row, Id::unit)) };

            switch (type)
            {
                case map::AudioParameter::floatingPoint:
                case map::AudioParameter::integer:
                    return [minLabel, maxLabel, min, max, unit] (ValueType value, int maximumStringLength)
                    {
                        if (minLabel.isNotEmpty() and value == min) return minLabel;
                        if (maxLabel.isNotEmpty() and value == max) return maxLabel;
                        juce::String text { value };
                        return text + " " + unit;
                    };
                case map::AudioParameter::choice:
                {
                    const auto choices { getChoices (parameters, row) };
                    return [minLabel, maxLabel, min, max, unit, choices] (ValueType index, int maximumStringLength)
                    {
                        if (minLabel.isNotEmpty() and index == min) return minLabel;
                        if (maxLabel.isNotEmpty() and index == max) return maxLabel;
                        return choices[index] + " " + unit;
                    };
                }
                default: break;
            }
        }
        return nullptr;
    }

    /**
     * @brief Adds a newly-constructed AudioProcessorParameter to the group
     *        matching `belongToGroup` by name.
     * @tparam AudioProcessorParameterType  Concrete AudioProcessorParameter type to construct.
     * @tparam Args                         Constructor argument types, forwarded to the parameter's constructor.
     * @param parameterGroup  Group table to search.
     * @param belongToGroup   Group name the new parameter is added to.
     * @param args            Constructor arguments for the new parameter.
     */
    template <typename AudioProcessorParameterType, typename... Args>
    static void addParameterToGroup (Group& parameterGroup, const juce::String& belongToGroup, Args&&... args)
    {
        for (auto& [name, group] : parameterGroup)
            if (name.equalsIgnoreCase (belongToGroup))
                group->addChild (std::make_unique<AudioProcessorParameterType> (std::forward<Args> (args)...));
    }

    /**
     * @brief Adds a newly-constructed AudioParameter either to its declared
     *        group or, when it declares none, directly to the layout.
     * @tparam AudioParameterType  Concrete AudioParameter type to construct.
     * @tparam Args                Constructor argument types, forwarded to the parameter's constructor.
     * @param layout          Parameter layout the parameter is added to when it declares no group.
     * @param parameterGroup  Group table the parameter is added to when it declares one.
     * @param group           Parameter's declared group name, or empty for none.
     * @param args            Constructor arguments for the new parameter.
     */
    template <typename AudioParameterType, typename... Args>
    static void addParameter (juce::AudioProcessorValueTreeState::ParameterLayout& layout, Group& parameterGroup, const juce::String& group, Args&&... args)
    {
        if (group.isNotEmpty()) addParameterToGroup<AudioParameterType> (parameterGroup, group, std::forward<Args> (args)...);
        else layout.add (std::make_unique<AudioParameterType> (std::forward<Args> (args)...));
    }

    /**
     * @brief Builds the complete parameter layout: one AudioParameter per
     *        `parameter` table row, dispatched by its `type` cell
     *        (float/choice/bool) to the matching constructor arm, added to
     *        its declared group or to the layout directly, then every
     *        populated group is added to the layout.
     * @param parameters  Parsed parameters.md document.
     * @return The built parameter layout.
     */
    static juce::AudioProcessorValueTreeState::ParameterLayout get (const MarkdownDocument& parameters) noexcept
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        Group parameterGroup { parameters };

        static const jam::Function::Map<juce::Identifier, void> dispatch {
            [] {
                jam::Function::Map<juce::Identifier, void> table;

                table.add<juce::AudioProcessorValueTreeState::ParameterLayout&, Group&, const MarkdownDocument&, Document::Element&>
                    (Id::floatingPoint,
                     [] (juce::AudioProcessorValueTreeState::ParameterLayout& layout, Group& parameterGroup, const MarkdownDocument& parameters, Document::Element& row)
                     {
                         juce::ParameterID paramID { getParameterID (parameters, row), ProjectInfo::versionHint };
                         auto group { parameters.getTableValue (row, Id::group) };

                         juce::NormalisableRange<float> range;
                         auto min { static_cast<float> (parameters.getTableValue (row, Id::min).getDoubleValue()) };
                         auto max { static_cast<float> (parameters.getTableValue (row, Id::max).getDoubleValue()) };
                         auto defaultValue { static_cast<float> (parameters.getTableValue (row, Id::defaultValue).getDoubleValue()) };
                         auto* taperMap { map::TaperMap::getInstance() };
                         auto taper { taperMap->get (parameters.getTableValue (row, Id::taper)) };

                         switch (taper)
                         {
                             case map::TaperMap::linear: range = juce::NormalisableRange<float> (min, max); break;
                             case map::TaperMap::skew:
                                 range = juce::NormalisableRange<float> (min, max);
                                 range.setSkewForCentre (static_cast<float> (parameters.getTableValue (row, Id::centre).getDoubleValue()));
                                 break;
                             default:
                             {
                                 bool isUsingDecibelTaper { parameters.getTableValue (row, Id::dB).getIntValue() != 0 };
                                 if (Taper::isTaperAntiLog (taper))
                                     range = isUsingDecibelTaper ? Taper::getDecibelAntiLogNormalisableRange (min, max, taper) : Taper::getAntiLogNormalisableRange (min, max, taper);
                                 else
                                     range = isUsingDecibelTaper ? Taper::getDecibelNormalisableRange (min, max, taper) : Taper::getNormalisableRange (min, max, taper);
                                 break;
                             }
                         }

                         range.interval = static_cast<float> (parameters.getTableValue (row, Id::interval).getDoubleValue());
                         addParameter<juce::AudioParameterFloat> (layout, parameterGroup, group, paramID, getParameterName (parameters, row), range, defaultValue, getFloatAttributes (parameters, row));
                     });

                table.add<juce::AudioProcessorValueTreeState::ParameterLayout&, Group&, const MarkdownDocument&, Document::Element&>
                    (Id::choice,
                     [] (juce::AudioProcessorValueTreeState::ParameterLayout& layout, Group& parameterGroup, const MarkdownDocument& parameters, Document::Element& row)
                     {
                         juce::ParameterID paramID { getParameterID (parameters, row), ProjectInfo::versionHint };
                         auto group { parameters.getTableValue (row, Id::group) };
                         auto defaultValue { parameters.getTableValue (row, Id::defaultValue).getIntValue() };
                         addParameter<juce::AudioParameterChoice> (layout, parameterGroup, group, paramID, getParameterName (parameters, row), getChoices (parameters, row), defaultValue, getChoiceAttributes (parameters, row));
                     });

                table.add<juce::AudioProcessorValueTreeState::ParameterLayout&, Group&, const MarkdownDocument&, Document::Element&>
                    (Id::boolean,
                     [] (juce::AudioProcessorValueTreeState::ParameterLayout& layout, Group& parameterGroup, const MarkdownDocument& parameters, Document::Element& row)
                     {
                         juce::ParameterID paramID { getParameterID (parameters, row), ProjectInfo::versionHint };
                         auto group { parameters.getTableValue (row, Id::group) };
                         auto defaultValue { parameters.getTableValue (row, Id::defaultValue).getIntValue() != 0 };
                         addParameter<juce::AudioParameterBool> (layout, parameterGroup, group, paramID, getParameterName (parameters, row), defaultValue, getBoolAttributes (parameters, row));
                     });

                return table;
            }()
        };

        for (auto* row : parameters.getTableRows (Id::parameter))
            dispatch.get (juce::Identifier { parameters.getTableValue (*row, Id::type) }, layout, parameterGroup, parameters, *row);

        for (auto& [name, group] : parameterGroup)
            layout.add (std::move (group));

        return layout;
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */

namespace jam
{
/*____________________________________________________________________________*/

SettingsModel::SettingsModel (const juce::Identifier& newTreeID)
{
    state = std::make_unique<juce::ValueTree> (newTreeID);
}

SettingsModel::SettingsModel()
{
    state = std::make_unique<juce::ValueTree> (Format::toScreamingSnakeCase (ProjectInfo::projectName));
}

SettingsModel::~SettingsModel()
{
}

//==============================================================================
juce::ValueTree& SettingsModel::get() const noexcept
{
    return *state;
}

void SettingsModel::replaceState (const juce::ValueTree& newState)
{
    state->copyPropertiesAndChildrenFrom (newState, nullptr);
}

bool SettingsModel::writeToXml (juce::File& destinationFile)
{
    constexpr int indentWidth { 2 };

    const juce::juce_wchar quoteCharacter { Chars::doubleQuote };

    std::function<juce::String (const juce::ValueTree&, int)> serialize;

    serialize = [&serialize, quoteCharacter] (const juce::ValueTree& node, int depth) -> juce::String
    {
        const juce::String indent { juce::String::repeatedString (juce::String::charToString (Chars::space), depth * indentWidth) };
        const auto tag { node.getType().toString() };

        juce::String attributes;

        for (int index { 0 }; index < node.getNumProperties(); ++index)
        {
            const auto propertyName { node.getPropertyName (index) };
            attributes << Chars::space << propertyName.toString()
                       << Chars::equals << quoteCharacter
                       << node.getProperty (propertyName).toString() << quoteCharacter;
        }

        juce::String xml;

        if (node.getNumChildren() == 0)
        {
            xml << indent << Chars::lessThan << tag << attributes << Chars::selfCloseTag << Chars::newline;
            return xml;
        }

        xml << indent << Chars::lessThan << tag << attributes << Chars::greaterThan << Chars::newline;

        for (const auto& child : node)
            xml << serialize (child, depth + 1);

        xml << indent << Chars::endTagOpen << tag << Chars::greaterThan << Chars::newline;

        return xml;
    };

    juce::String text;
    text << Chars::processingOpen << Chars::xml << Chars::space << Id::version.toString()
         << Chars::equals << quoteCharacter << Chars::one << Chars::dot << Chars::zero << quoteCharacter
         << Chars::space << Chars::encoding << Chars::equals
         << quoteCharacter << Chars::utf8 << quoteCharacter
         << Chars::processingClose << Chars::newline << Chars::newline;
    text << serialize (*state, 0);

    return destinationFile.replaceWithText (text);
}

//==============================================================================

void SettingsModel::valueChanged (juce::Value& value)
{
    if (onValueChanged != nullptr)
        onValueChanged();
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam

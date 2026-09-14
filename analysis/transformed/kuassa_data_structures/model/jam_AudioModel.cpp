/**
 * @file jam_AudioModel.cpp
 * @brief Implementation of the AudioModel class for managing plugin parameters and state.
 *
 * This file contains the implementation of the AudioModel class, which handles user interface
 * and parameter management for JUCE-based audio plugins. The AudioModel integrates functionality
 * for value tree properties, dark mode settings, preset handling, and license evaluation.
 */

namespace jam
{
/*____________________________________________________________________________*/
AudioModel::AudioModel (jam::ParameterManager& manager,
                        juce::AudioProcessor& processorToConnectTo,
                        juce::AudioProcessorValueTreeState::ParameterLayout parameterLayout,
                        juce::UndoManager* undoManagerToUse)
    : juce::AudioProcessorValueTreeState (processorToConnectTo,
                                          undoManagerToUse,
                                          Format::toValidID (ProjectInfo::projectName, true),
                                          std::move (parameterLayout))
    , manager (manager)
{
    /** Plugin version */
    state.setProperty (Id::version, ProjectInfo::versionString, nullptr);

    populateParameterPageTree();

    /** since macOS has its own retina scaling, setScaleFactor was never called,
        hence the following serves as fallback to calculate state.getUIScaleFactor() */
    setDesktopScale (1.0f);
}

AudioModel::~AudioModel() { removeListener(); }

//==============================================================================
void AudioModel::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                           const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged.getType() == Id::toType (Id::user))
        if (onUserTreeChanged != nullptr)
            onUserTreeChanged();

    if (treeWhosePropertyHasChanged.getType() == Id::toType (Id::param))
    {
        const auto parameterID { treeWhosePropertyHasChanged.getProperty (Id::id) };
        refreshCurrentPageValue (parameterID);

        markPresetDirty (parameterID);
    }
}

void AudioModel::attach (juce::Value& value,
                         const juce::Identifier& parameterId,
                         const juce::Identifier& propertyId)
{
    if (getParameter (parameterId.toString()) != nullptr)
        Model::attach (state, value, parameterId, propertyId);
    else
    {
        auto child { state.getChildWithName (Id::toTag (Id::parameter)) };
        jassert (child.isValid());
        Model::attach (child, value, parameterId, propertyId);
    }
}

juce::Value AudioModel::getValue (const juce::Identifier& parameterId,
                                  const juce::Identifier& propertyId) const noexcept
{
    return Model::getValueFromChildWithID (state, parameterId, propertyId);
}

juce::Value AudioModel::getPresetValue (const juce::Identifier& propertyId) const noexcept
{
    return Model::getValueFromChildWithID (state, Id::toTag (Id::preset), propertyId);
}

juce::Value AudioModel::getBypassStateValue() const noexcept
{
    return Model::getValueFromChildWithID (state, Id::toTag (Id::bypass), Id::value);
}

//==============================================================================
void AudioModel::setUIScale (const juce::String& newScale)
{
    getUIScaleValue().setValue (newScale);
}

void AudioModel::setDesktopScale (float newScale)
{
    if (auto param { Model::getChildWithID (state, Id::toTag (Id::UIScale)) }; param.isValid())
        param.setProperty (Id::desktop, newScale, nullptr);
}

float AudioModel::getDesktopScale() const noexcept
{
    if (auto param { Model::getChildWithID (state, Id::toTag (Id::UIScale)) }; param.isValid())
        if (param.hasProperty (Id::desktop))
            return param.getPropertyAsValue (Id::desktop, nullptr).getValue();

    return 1.0f;
}

int AudioModel::getUIScale() const noexcept
{
    const auto& map { *map::UIScaleMap::getInstance() };
    return map.get (getUIScaleValue().getValue().toString());
}

juce::Value AudioModel::getUIScaleValue() const noexcept
{
    return Model::getValueFromChildWithID (state, Id::toTag (Id::UIScale));
}

int AudioModel::getUIPanelHeight() const noexcept
{
    return juce::roundToInt (manager.getPanelHeight() * getHostScale());
}

//==============================================================================
juce::String AudioModel::getAppearance() const noexcept
{
    return Model::getValueFromChildWithID (state, Id::toTag (Id::appearance)).toString();
}

juce::Value AudioModel::getAppearanceValue() const noexcept
{
    return Model::getValueFromChildWithID (state, Id::toTag (Id::appearance));
}

void AudioModel::setAppearance (const juce::String& appearance)
{
    getAppearanceValue().setValue (appearance);
}

juce::Value AudioModel::getOrientationValue() const noexcept
{
    return Model::getValueFromChildWithID (state, Id::toTag (Id::orientation));
}

juce::String AudioModel::getOrientation() const noexcept
{
    return getOrientationValue().toString();
}

//==============================================================================
#if JAM_USING_OVERSAMPLING
juce::Value AudioModel::getOversamplingValue() const noexcept
{
    return Model::getValueFromChildWithID (state, Id::toTag (Id::oversampling));
}

int AudioModel::getOversamplingFactor() const noexcept
{
    auto* os { map::Oversampling::getInstance() };

    if (auto value { getOversamplingValue().getValue().toString() }; value.isNotEmpty())
        return os->get (value) - 1;

    return os->get (manager.getDefaultOversampling()) - 1;
}

bool AudioModel::isOversampled() const noexcept
{
    return static_cast<bool> (getOversamplingFactor());
}
#endif// JAM_USING_OVERSAMPLING
//==============================================================================
// FROM DAW
void AudioModel::setState (const juce::ValueTree& stateFromDAW)
{
    if (stateFromDAW.isValid())
    {
        state.copyPropertiesAndChildrenFrom (stateFromDAW, nullptr);

        if (onSetState != nullptr)
            onSetState();

#if JAM_USING_AQUATIC_PRIME
        refreshEvaluationStatus (true);
#endif// JAM_USING_AQUATIC_PRIME
    }
}

std::unique_ptr<juce::XmlElement> AudioModel::getPresetXml() const noexcept
{
    if (auto xml = state.createXml())
    {
        /** only write parameters (excluding BYPASS and OVERSAMPLING) to preset file */
        // REMOVE USER INFO TREE
        xml->removeChildElement (xml->getChildByName (Id::toTag (Id::user)), true);

        // REMOVE BYPASS PARAMETER
        xml->removeChildElement (xml->getChildByAttribute (Id::id, Id::toTag (Id::bypass)), true);

        // REMOVE A-B TREE
        xml->deleteAllChildElementsWithTagName (Id::toTag (Id::page));

        // REMOVE SETTINGS
        xml->deleteAllChildElementsWithTagName (Id::toTag (Id::settings));

        // REMOVE NON-AUTO PARAMETERS
        xml->removeChildElement (xml->getChildByName (Id::toTag (Id::parameter)), true);

        return xml;
    }

    return nullptr;
}

bool AudioModel::createDefaultInitPreset() const noexcept
{
    if (auto init { manager.getDefaultInitPreset() }; not init.existsAsFile())
    {
        if (auto xml = getPresetXml())
        {
            for (auto* e : xml->getChildWithTagNameIterator (Id::toType (Id::param)))
            {
                auto value { manager.getDefaultValueMap().at (e->getStringAttribute (Id::id)) };

                e->setAttribute (Id::value, value.toString());
            }

            return xml->writeTo (init);
        }
    }

    return true;
}

juce::File AudioModel::getCurrentPresetFile() const noexcept
{
    return { Model::getValueFromChildWithID (state, Id::toTag (Id::preset), Id::file)
                 .getValue()
                 .toString() };
}

juce::String AudioModel::getCurrentPresetName() const noexcept
{
    return { Model::getValueFromChildWithID (state, Id::toTag (Id::preset), Id::value)
                 .getValue()
                 .toString() };
}

juce::String AudioModel::getCurrentPresetFileName() const noexcept
{
    return { Format::toFileName (getCurrentPresetName(), manager.getPresetExtension()) };
}

void AudioModel::loadPreset (const juce::File& file)
{
    if (not isPresetDirty())
    {
        if (file.existsAsFile())
        {
            if (auto xml { juce::parseXML (file) })
            {
                getValue (Id::toTag (Id::preset), Id::file).setValue (file.getFullPathName());
                getValue (Id::toTag (Id::preset), Id::isDirty).setValue (false);
                loadPreset (xml.get());
                populateParameterPageTree();
            }
        }
    }
}

void AudioModel::loadPreset (juce::XmlElement* presetXml)
{
    if (presetXml != nullptr)
    {
        for (auto&& p : presetXml->getChildWithTagNameIterator (Id::toType (Id::param)))
        {
            auto parameterID { p->getStringAttribute (Id::id) };

            if (auto* param { getParameter (parameterID) })
            {
                double value { (p->getDoubleAttribute (Id::value)) };
                param->beginChangeGesture();
                param->setValueNotifyingHost (param->convertTo0to1 (value));

                if (p->hasAttribute (Id::onState))
                {
                    auto parameter { Model::getChildWithID (state, parameterID) };
                    parameter.setProperty (Id::onState, p->getIntAttribute (Id::onState), nullptr);
                }

                param->endChangeGesture();
            }
        }
    }
}

bool AudioModel::savePreset (const juce::File& file) const noexcept
{
    if (auto xml { getPresetXml() })
        return xml->writeTo (file);

    return false;
}

bool AudioModel::saveCurrentPreset() const noexcept { return savePreset (getCurrentPresetFile()); }

bool AudioModel::isPresetDirty() const noexcept
{
    return { getPresetValue (Id::isDirty).getValue() };
}

#if not JAM_USING_AQUATIC_PRIME
bool AudioModel::isEvaluating() const noexcept { return false; }
#endif// not JAM_USING_AQUATIC_PRIME

//==============================================================================
void AudioModel::populateParameterPageTree()
{
    if (auto* pageContext { map::ParameterPage::getInstance() })
    {
        const auto numPages { static_cast<int> (pageContext->get().size()) };

        juce::ValueTree collectedParams { Id::toTag (Id::parameter) };

        for (auto&& child : state)
        {
            if (child.hasType (Id::toType (Id::param)))
                collectedParams.appendChild (child.createCopy(), nullptr);
        }

        juce::ValueTree generatedPages { Id::toTag (Id::page) };

        for (int i { 0 }; i < numPages; ++i)
        {
            auto pageTree { collectedParams.createCopy() };
            pageTree.setProperty (Id::id, pageContext->get (i), nullptr);
            generatedPages.appendChild (pageTree, nullptr);
        }

        auto statePage { state.getOrCreateChildWithName (Id::toTag (Id::page), nullptr) };
        statePage.removeAllChildren (nullptr);

        for (auto&& page : generatedPages)
            statePage.appendChild (page.createCopy(), nullptr);
    }

    for (auto&& p : state.getChildWithName (Id::toTag (Id::page)))
        copyParameterValuesToPage (p.getProperty (Id::id).toString());
}

//==============================================================================
juce::Value AudioModel::getCurrentPageValue() const noexcept
{
    return Model::getValueFromChildWithID (state, Id::toTag (Id::page));
}

std::unique_ptr<juce::XmlElement>
AudioModel::getPagePreset (const juce::Identifier& type) const noexcept
{
    return state.getChildWithName (Id::toTag (Id::page))
        .getChildWithProperty (Id::id, type.toString())
        .createXml();
}

void AudioModel::refreshCurrentPageValue (const juce::String& parameterID)
{
    if (auto parameter { Model::getChildWithID (state, getCurrentPageValue().toString()) };
        parameter.isValid())
    {
        if (auto param { Model::getChildWithID (parameter, parameterID) }; param.isValid())
        {
            if (auto* p { getParameter (parameterID) })
                param.setProperty (Id::value, p->convertFrom0to1 (p->getValue()), nullptr);
        }
    }
}

void AudioModel::copyParameterValuesToPage (const juce::String& pageDestination)
{
    if (auto parameter { Model::getChildWithID (state, pageDestination) }; parameter.isValid())
    {
        for (auto&& param : parameter)
        {
            auto parameterID { param.getPropertyAsValue (Id::id, nullptr).toString() };

            param.setProperty (Id::value, getRawParameterValue (parameterID)->load(), nullptr);
        }
    }
}

//==============================================================================
juce::String AudioModel::getHostDescription() const noexcept
{
    return juce::PluginHostType().getHostDescription();
}

juce::String AudioModel::getPluginFormatName() const noexcept
{
    auto description { processor.getWrapperTypeDescription (processor.wrapperType) };

    return description;
}

juce::AudioProcessor::WrapperType AudioModel::getWrapperType() const noexcept
{
    return processor.wrapperType;
}
//==============================================================================
bool AudioModel::isValidParameterID (const juce::String& parameterID) const noexcept
{
    return getParameter (parameterID) != nullptr;
}

void AudioModel::markPresetDirty (const juce::String& parameterID)
{
    if (not isPresetDirty())
    {
        if (getCurrentPresetFile().getFileNameWithoutExtension().compare (getCurrentPresetName())
            == 0)
        {
            if (auto file { juce::File (getPresetValue (Id::file).toString()) };
                file.existsAsFile())
            {
                if (auto xml { juce::parseXML (file) })
                {
                    if (auto* param { Xml::getChildByID (xml, parameterID) })
                    {
                        const auto& getNormalisedValue = [this, parameterID] (float value)
                        {
                            auto parameter { getParameter (parameterID) };
                            return parameter->convertTo0to1 (value);
                        };

                        float presetValue { getNormalisedValue (
                            param->getDoubleAttribute (Id::value)) };
                        float parameterValue { getNormalisedValue (
                            getParameterAsValue (parameterID).getValue()) };

                        const bool isDirty { not juce::approximatelyEqual (parameterValue, presetValue) };

                        if (isDirty)
                            getValue (Id::toTag (Id::preset), Id::isDirty).setValue (isDirty);
                    }
                }
            }
        }
    }
}

void AudioModel::addListener (juce::AudioProcessorValueTreeState::Listener& processorChainAsListener)
{
    listener = &processorChainAsListener;

    for (auto& param : processor.getParameters())
    {
        if (auto* p { dynamic_cast<juce::AudioProcessorParameterWithID*> (param) })
        {
            addParameterListener (p->getParameterID(), listener);
        }
    }
}

void AudioModel::removeListener()
{
    if (listener != nullptr)
    {
        for (auto& param : processor.getParameters())
        {
            if (auto* p { dynamic_cast<juce::AudioProcessorParameterWithID*> (param) })
            {
                removeParameterListener (p->getParameterID(), listener);
            }
        }
    }
}

//==============================================================================
juce::ValueTree AudioModel::getOrCreateComponentState (const juce::String& componentId)
{
    auto interfaceTree { state.getOrCreateChildWithName (Id::toType (Id::view), nullptr) };
    auto componentNode { interfaceTree.getChildWithProperty (Id::id, componentId) };

    if (not componentNode.isValid())
    {
        componentNode = juce::ValueTree (juce::Identifier (Id::toTag (Id::component)));
        componentNode.setProperty (Id::id, componentId, nullptr);
        interfaceTree.appendChild (componentNode, nullptr);
    }

    return componentNode;
}

juce::Value AudioModel::getComponentPropertyValue (const juce::String& componentId,
                                                   const juce::Identifier& propertyId)
{
    auto componentNode { getOrCreateComponentState (componentId) };
    return componentNode.getPropertyAsValue (propertyId, nullptr);
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam

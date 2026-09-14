#if JucePlugin_Build_Standalone

namespace jam
{
/*____________________________________________________________________________*/
// AudioSettingsWindow
/*____________________________________________________________________________*/

AudioStandaloneApp::AudioSettingsWindow::AudioSettingsWindow (juce::AudioDeviceManager& deviceManagerToUse,
                                                               int maxNumInputs,
                                                               int maxNumOutputs,
                                                               bool processorProducesMidi)
    : jam::Window (createContent (deviceManagerToUse, maxNumInputs, maxNumOutputs, processorProducesMidi),
                      "Audio/MIDI Settings",
                      false,
                      true)
{
    setVisible (true);
}

void AudioStandaloneApp::AudioSettingsWindow::closeButtonPressed() { setVisible (false); }

juce::Component* AudioStandaloneApp::AudioSettingsWindow::createContent (juce::AudioDeviceManager& deviceManagerToUse,
                                                                          int maxNumInputs,
                                                                          int maxNumOutputs,
                                                                          bool processorProducesMidi)
{
    auto selector { std::make_unique<juce::AudioDeviceSelectorComponent> (deviceManagerToUse,
                                                                          0, maxNumInputs,
                                                                          0, maxNumOutputs,
                                                                          true,
                                                                          processorProducesMidi,
                                                                          true,
                                                                          false) };
    selector->setSize (500, 550);
    return selector.release();
}

/*____________________________________________________________________________*/
// MainWindow
/*____________________________________________________________________________*/

AudioStandaloneApp::MainWindow::MainWindow (const juce::String& name,
                                            std::unique_ptr<juce::StandalonePluginHolder> pluginHolderIn)
    : jam::Window (createEditorComponent (*pluginHolderIn), name, false, true)
    , pluginHolder (std::move (pluginHolderIn))
{
    setVisible (true);
}

AudioStandaloneApp::MainWindow::~MainWindow()
{
    if (auto* editor { getEditor() })
        pluginHolder->processor->editorBeingDeleted (editor);

    clearContentComponent();
}

void AudioStandaloneApp::MainWindow::closeButtonPressed()
{
    pluginHolder->savePluginState();
    juce::JUCEApplicationBase::quit();
}

void AudioStandaloneApp::MainWindow::showAudioSettingsDialog()
{
    auto* processor { pluginHolder->processor.get() };
    jassert (processor != nullptr);

    auto maxNumInputs { 0 };
    auto maxNumOutputs { 0 };

    if (auto* bus { processor->getBus (true, 0) })
        maxNumInputs = juce::jmax (0, bus->getDefaultLayout().size());

    if (auto* bus { processor->getBus (false, 0) })
        maxNumOutputs = juce::jmax (0, bus->getDefaultLayout().size());

    audioSettingsWindow = std::make_unique<AudioSettingsWindow> (pluginHolder->deviceManager,
                                                                  maxNumInputs,
                                                                  maxNumOutputs,
                                                                  processor->producesMidi());
}

juce::AudioProcessorEditor* AudioStandaloneApp::MainWindow::getEditor() const noexcept
{
    return dynamic_cast<juce::AudioProcessorEditor*> (getContentComponent());
}

juce::Component* AudioStandaloneApp::MainWindow::createEditorComponent (juce::StandalonePluginHolder& holder)
{
    auto* processor { holder.processor.get() };
    jassert (processor != nullptr);

    if (processor->hasEditor()) return processor->createEditorAndMakeActive();

    auto editor { std::make_unique<juce::GenericAudioProcessorEditor> (*processor) };
    return editor.release();
}

/*____________________________________________________________________________*/
// AudioStandaloneApp
/*____________________________________________________________________________*/

AudioStandaloneApp::AudioStandaloneApp()
{
    juce::PropertiesFile::Options options;

    options.applicationName     = juce::CharPointer_UTF8 (JucePlugin_Name);
    options.filenameSuffix      = ".settings";
    options.osxLibrarySubFolder = "Application Support";
#if JUCE_LINUX or JUCE_BSD
    options.folderName          = "~/.config";
#else
    options.folderName          = "";
#endif

    appProperties.setStorageParameters (options);
}

const juce::String AudioStandaloneApp::getApplicationName()    { return juce::CharPointer_UTF8 (JucePlugin_Name); }
const juce::String AudioStandaloneApp::getApplicationVersion() { return JucePlugin_VersionString; }
bool AudioStandaloneApp::moreThanOneInstanceAllowed()           { return true; }

void AudioStandaloneApp::initialise (const juce::String&)
{
    jassert (not juce::Desktop::getInstance().getDisplays().displays.isEmpty());

    mainWindow = std::make_unique<MainWindow> (getApplicationName(), createPluginHolder());
}

void AudioStandaloneApp::shutdown()
{
    mainWindow = nullptr;
    appProperties.saveIfNeeded();
}

void AudioStandaloneApp::systemRequestedQuit()
{
    if (mainWindow != nullptr)
        mainWindow->pluginHolder->savePluginState();

    if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
    {
        juce::Timer::callAfterDelay (100, []
        {
            if (auto* app { juce::JUCEApplicationBase::getInstance() })
                app->systemRequestedQuit();
        });
    }
    else
    {
        quit();
    }
}

std::unique_ptr<juce::StandalonePluginHolder> AudioStandaloneApp::createPluginHolder()
{
    constexpr auto autoOpenMidiDevices =
#if (JUCE_ANDROID or JUCE_IOS) and not JUCE_DONT_AUTO_OPEN_MIDI_DEVICES_ON_MOBILE
        true;
#else
        false;
#endif

#ifdef JucePlugin_PreferredChannelConfigurations
    constexpr juce::StandalonePluginHolder::PluginInOuts channels[] { JucePlugin_PreferredChannelConfigurations };
    const juce::Array<juce::StandalonePluginHolder::PluginInOuts> channelConfig (channels, juce::numElementsInArray (channels));
#else
    const juce::Array<juce::StandalonePluginHolder::PluginInOuts> channelConfig;
#endif

    return std::make_unique<juce::StandalonePluginHolder> (appProperties.getUserSettings(),
                                                            false,
                                                            juce::String{},
                                                            nullptr,
                                                            channelConfig,
                                                            autoOpenMidiDevices);
}

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam

juce::JUCEApplicationBase* juce_CreateApplication() { return new jam::AudioStandaloneApp(); }

#endif

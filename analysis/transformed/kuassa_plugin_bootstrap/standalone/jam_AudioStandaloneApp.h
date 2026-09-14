#pragma once

#if JucePlugin_Build_Standalone

namespace jam
{
/*____________________________________________________________________________*/

class AudioStandaloneApp : public juce::JUCEApplication
{
public:
    AudioStandaloneApp();

    const juce::String getApplicationName() override;
    const juce::String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed() override;

    void initialise (const juce::String& commandLineParameters) override;
    void shutdown() override;
    void systemRequestedQuit() override;

protected:
    virtual std::unique_ptr<juce::StandalonePluginHolder> createPluginHolder();

private:
    class AudioSettingsWindow : public jam::Window
    {
    public:
        AudioSettingsWindow (juce::AudioDeviceManager& deviceManagerToUse,
                             int maxNumInputs,
                             int maxNumOutputs,
                             bool processorProducesMidi);

        void closeButtonPressed() override;

    private:
        static juce::Component* createContent (juce::AudioDeviceManager& deviceManagerToUse,
                                               int maxNumInputs,
                                               int maxNumOutputs,
                                               bool processorProducesMidi);

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioSettingsWindow)
    };

    class MainWindow : public jam::Window
    {
    public:
        MainWindow (const juce::String& name, std::unique_ptr<juce::StandalonePluginHolder> pluginHolderIn);
        ~MainWindow() override;

        void closeButtonPressed() override;

        void showAudioSettingsDialog();

        juce::AudioProcessorEditor* getEditor() const noexcept;

        std::unique_ptr<juce::StandalonePluginHolder> pluginHolder;

    private:
        static juce::Component* createEditorComponent (juce::StandalonePluginHolder& holder);

        std::unique_ptr<AudioSettingsWindow> audioSettingsWindow;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    juce::ApplicationProperties appProperties;
    std::unique_ptr<MainWindow> mainWindow;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioStandaloneApp)
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam

extern juce::JUCEApplicationBase* juce_CreateApplication();

#endif

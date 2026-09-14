// Windows defines MessageBoxA/MessageBoxW macros that conflict with jam::MessageBox methods
#ifdef _WIN32
  #undef MessageBoxA
  #undef MessageBoxW
  #undef MessageBox
#endif

namespace jam
{
/*____________________________________________________________________________*/

void MessageBox::showAlert (const juce::String& title,
                            const juce::String& message,
                            juce::Component* associatedComponent,
                            std::function<void (int)> callback,
                            juce::MessageBoxIconType iconType)
{
    juce::NativeMessageBox::showAsync (juce::MessageBoxOptions()
                                           .withTitle (title)
                                           .withIconType (iconType)
                                           .withMessage (message)
                                           .withButton (text::English::buttonOk)
                                           .withAssociatedComponent (associatedComponent),
                                       callback);
}

void MessageBox::showYesNoBox (const juce::String& title,
                               const juce::String& message,
                               juce::Component* associatedComponent,
                               std::function<void (int)> callback)
{
    juce::NativeMessageBox::showAsync (juce::MessageBoxOptions()
                                           .withTitle (title)
                                           .withIconType (juce::AlertWindow::QuestionIcon)
                                           .withMessage (message)
                                           .withButton (text::English::buttonYes)
                                           .withButton (text::English::buttonNo)
                                           .withAssociatedComponent (associatedComponent),
                                       callback);
}

void MessageBox::newerVersionPreset (const juce::String& presetFileName,
                                     juce::Component* associatedComponent,
                                     std::function<void (int)> callback)
{
    showAlert (presetFileName, Format::getAlertNewerVersionPreset (ProjectInfo::projectName), associatedComponent, callback);
}

void MessageBox::askReplacePreset (juce::StringRef presetFileName,
                                   juce::Component* associatedComponent,
                                   std::function<void (int)> callback)
{
    showYesNoBox (Format::getPresetAlreadyExists (presetFileName), Format::getAskReplace(), associatedComponent, callback);
}

void MessageBox::askReplaceFile (juce::StringRef fileName,
                                 juce::Component* associatedComponent,
                                 std::function<void (int)> callback)
{
    showYesNoBox (Format::getFileAlreadyExists (fileName),
                  Format::getAskReplace(),
                  associatedComponent,
                  callback);
}

void MessageBox::presetSaved (const juce::String& presetFileName,
                              juce::Component* associatedComponent)
{
    showAlert (presetFileName, text::English::presetSaved, associatedComponent);
}

void MessageBox::fileSaved (const juce::File& file,
                            juce::Component* associatedComponent)
{
    juce::String message;
    message << text::English::fileSavedPrefix << Chars::space << file.getFullPathName();
    showAlert (file.getFileNameWithoutExtension(), message, associatedComponent);
}

void MessageBox::manualNotFound (juce::Component* associatedComponent)
{
    showAlert (ProjectInfo::projectName, Format::getAlertUserManualNotFound(), associatedComponent);
}

void MessageBox::impulsePathTooLong (juce::Component* associatedComponent)
{
    showAlert (Format::getAlertIRNameTooLong(),
               Format::getChooseShorterNameOrLocation(),
               associatedComponent);
}

void MessageBox::impulseNotRecognised (juce::Component* associatedComponent)
{
    showAlert (Format::getAlertIRNotRecognized(),
               Format::getChooseAnotherFile(),
               associatedComponent);
}

void MessageBox::noCigar (const juce::String& message, juce::Component* associatedComponent)
{
    showAlert (Format::getNoCigar(), message, associatedComponent);
}

void MessageBox::tryAgain (const juce::String& message, juce::Component* associatedComponent)
{
    showAlert (Format::getPleaseTryAgain(), message, associatedComponent);
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam

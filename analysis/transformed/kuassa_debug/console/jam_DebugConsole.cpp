/*
 MIT License

 Copyright (c) 2018 Janos Buttgereit

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 _____________________________________________________________________________*/

#if JUCE_WINDOWS
#include <jam_core/utilities/jam_Platform.h>
#include <io.h>
#include <fcntl.h>
#define fileno _fileno
#define dup _dup
#define dup2 _dup2
#define read _read
#define close _close
#else
#include <unistd.h>
#endif

namespace jam
{
/*____________________________________________________________________________*/

DebugConsole::Log::Log (bool captureStdErrImmediately, bool captureStdOutImmediately)
{
#if JUCE_WINDOWS
    if (AllocConsole())
    {
        freopen ("CONOUT$", "w", stdout);
        freopen ("CONOUT$", "w", stderr);
        ShowWindow (FindWindowA ("ConsoleWindowClass", NULL), false);
    }
#endif

    addAndMakeVisible (consoleOutputEditor);
    consoleOutputEditor.setMultiLine (true, true);
    consoleOutputEditor.setReadOnly (true);
    consoleOutputEditor.setLineSpacing (1.5f);

    consoleOutputEditor.setFont (juce::FontOptions { juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain });
    consoleOutputEditor.setColour (juce::TextEditor::focusedOutlineColourId, backgroundColour);

    if (originalStdout == -3)
        originalStdout = dup (fileno (stdout));
    if (originalStderr == -4)
        originalStderr = dup (fileno (stderr));
    std::ios::sync_with_stdio();

    if (captureStdErrImmediately)
        captureStdErr();

    if (captureStdOutImmediately)
        captureStdOut();
}

DebugConsole::Log::~Log()
{
    releaseStdOut();
    releaseStdErr();

    if (juce::Logger::getCurrentLogger() == this)
        juce::Logger::setCurrentLogger (nullptr);
}

void DebugConsole::Log::clear()
{
    const juce::ScopedLock scopedLock (linesLock);

    consoleOutputEditor.clear();
    lines.clear();
    lineColours.clearQuick();
    numLinesStored = 0;
    numNewLinesSinceUpdate = 0;
}

void DebugConsole::Log::logMessage (const juce::String& message)
{
    {
        const juce::ScopedLock scopedLock (linesLock);
        lines.add (message + "\n");
        lineColours.add (logMessageColour);

        ++numNewLinesSinceUpdate;
        ++numLinesStored;
    }

    triggerAsyncUpdate();
}

bool DebugConsole::Log::captureStdOut()
{
    if (currentStdOutTarget == this)
        return true;

    if (createAndAssignPipe (logStdOutputPipe, stdout) == false)
        return false;

    currentStdOutTarget = this;

    if (stdOutReaderThread == nullptr)
        stdOutReaderThread.reset (new std::thread (Log::readStdOut));

    return true;
}

bool DebugConsole::Log::captureStdErr()
{
    if (currentStdErrTarget == this)
        return true;

    if (createAndAssignPipe (logErrOutputPipe, stderr) == false)
        return false;

    currentStdErrTarget = this;

    if (stdErrReaderThread == nullptr)
        stdErrReaderThread.reset (new std::thread (Log::readStdErr));

    return true;
}

void DebugConsole::Log::releaseStdOut (bool printRestoreMessage)
{
    if (currentStdOutTarget != this)
        return;

    currentStdOutTarget = nullptr;
    deletePipeAndEndThread (originalStdout, stdout, stdOutReaderThread);

    if (printRestoreMessage)
        std::cout << "Log restored stdout" << std::endl;
}

void DebugConsole::Log::releaseStdErr (bool printRestoreMessage)
{
    if (currentStdErrTarget != this)
        return;

    currentStdErrTarget = nullptr;
    deletePipeAndEndThread (originalStderr, stderr, stdErrReaderThread);

    if (printRestoreMessage)
        std::cout << "Log restored stderr" << std::endl;
}

void DebugConsole::Log::resized()
{
    consoleOutputEditor.setBounds (getLocalBounds());
}

bool DebugConsole::Log::createAndAssignPipe (int* pipeIDs, FILE* stream)
{
    fflush (stream);

#if JUCE_WINDOWS
    int retValue = _pipe (pipeIDs, tmpBufLen, _O_TEXT);
#else
    int retValue = pipe (pipeIDs);
#endif
    if (retValue != 0)
        return false;

    dup2 (pipeIDs[1], fileno (stream));
    close (pipeIDs[1]);

    std::ios::sync_with_stdio();

    setvbuf (stream, NULL, _IONBF, 0);

    return true;
}

void DebugConsole::Log::deletePipeAndEndThread (int original, FILE* stream, std::unique_ptr<std::thread>& thread)
{
    fprintf (stream, " ");
    fflush (stream);
    thread->join();

    dup2 (original, fileno (stream));

    thread.reset (nullptr);
}

void DebugConsole::Log::readStdOut()
{
    char tmpStdOutBuf[tmpBufLen];

    while (true)
    {
        fflush (stdout);
        size_t numCharsRead = read (logStdOutputPipe[0], tmpStdOutBuf, tmpBufLen - 1);
        if (currentStdOutTarget == nullptr)
            break;
        tmpStdOutBuf[numCharsRead] = '\0';
        currentStdOutTarget->addFromStd (tmpStdOutBuf, numCharsRead, currentStdOutTarget->stdOutColour);
    }
}

void DebugConsole::Log::readStdErr()
{
    char tmpStdErrBuf[tmpBufLen];

    while (true)
    {
        fflush (stderr);
        size_t numCharsRead = read (logErrOutputPipe[0], tmpStdErrBuf, tmpBufLen - 1);
        if (currentStdErrTarget == nullptr)
            break;
        tmpStdErrBuf[numCharsRead] = '\0';
        currentStdErrTarget->addFromStd (tmpStdErrBuf, numCharsRead, currentStdErrTarget->stdErrColour);
    }
}

void DebugConsole::Log::addFromStd (char* stringBufferToAdd, size_t bufferSize, juce::Colour colourOfString)
{
    linesLock.enter();
    int numNewLines = lines.addTokens (stringBufferToAdd, "\n", "");
    for (int i = 0; i < numNewLines; i++)
    {
        lineColours.add (colourOfString);
    }

    numNewLinesSinceUpdate += numNewLines;
    numLinesStored += numNewLines;

    if (stringBufferToAdd[bufferSize - 1] == '\n')
    {
        lines.getReference (numLinesStored - 1) += "\n";
    }
    for (int i = numLinesStored - numNewLines; i < numLinesStored - 1; i++)
    {
        lines.getReference (i) += "\n";
    }

    linesLock.exit();
    triggerAsyncUpdate();
}

void DebugConsole::Log::handleAsyncUpdate()
{
    const juce::ScopedLock scopedLock (linesLock);
    if (numLinesStored > numLinesToStore)
    {
        int numLinesToRemove = numLinesStored - numLinesToStore;
        if (numLinesToRemove < numLinesToRemoveWhenFull)
            numLinesToRemove = numLinesToRemoveWhenFull;
        lines.removeRange (0, numLinesToRemove);
        lineColours.removeRange (0, numLinesToRemove);
        numLinesStored = lines.size();

        consoleOutputEditor.clear();
        numNewLinesSinceUpdate = numLinesStored;
    }

    juce::Colour lastColour = juce::Colours::black;
    consoleOutputEditor.moveCaretToEnd();
    consoleOutputEditor.setColour (juce::TextEditor::ColourIds::textColourId, lastColour);

    for (int i = numLinesStored - numNewLinesSinceUpdate; i < numLinesStored; i++)
    {
        if (lineColours[i] != lastColour)
        {
            lastColour = lineColours[i];
            consoleOutputEditor.setColour (juce::TextEditor::ColourIds::textColourId, lastColour);
        }
        consoleOutputEditor.insertTextAtCaret (lines[i]);
    }

    numNewLinesSinceUpdate = 0;
}

int DebugConsole::Log::originalStdout { -3 };
int DebugConsole::Log::originalStderr { -4 };
int DebugConsole::Log::logStdOutputPipe[2];
int DebugConsole::Log::logErrOutputPipe[2];
std::unique_ptr<std::thread> DebugConsole::Log::stdOutReaderThread;
std::unique_ptr<std::thread> DebugConsole::Log::stdErrReaderThread;

DebugConsole::Log* DebugConsole::Log::currentStdOutTarget { nullptr };
DebugConsole::Log* DebugConsole::Log::currentStdErrTarget { nullptr };

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam

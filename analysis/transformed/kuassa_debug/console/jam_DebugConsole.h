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

 forked from:  https://git.fh-muenster.de/NTLab/CodeReUse/Log4JUCE
 _____________________________________________________________________________*/

/**
 * @file jam_DebugConsole.h
 * @brief DebugConsole — resizable stdout/stderr-capturing log window.
 */

#pragma once
#include <thread>

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class DebugConsole
 * @brief A resizable window that acts as a console for logging and displaying messages.
 *
 * @details The DebugConsole class inherits from jam::Window and provides an interface
 *          for displaying logged information in a customizable, resizable UI component.
 *          The class supports logging messages, clearing the log, and capturing stdout and stderr.
 */
class DebugConsole : public jam::Window
{
public:
    /**
     * @brief Default constructor for DebugConsole.
     *
     * @details Initializes the console window with default dimensions based on the user's primary display.
     *          Sets up logging and window properties.
     */
    DebugConsole()
        : jam::Window (
              []
              {
                  auto desktop { juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userBounds.toNearestInt() };
                  auto log { std::make_unique<Log>() };
                  log->setSize (desktop.getWidth() / 3, desktop.getHeight());
                  return log.release();
              }(),
              "Console",
              true,
              false)
    {
        auto desktop { juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userBounds.toNearestInt() };
        setTopLeftPosition (desktop.getWidth() - getWidth(), 0);
        windowInit();
    }

    /**
     * @brief Parameterized constructor for DebugConsole.
     *
     * @param windowTitle The title of the console window.
     * @param width Width of the console window.
     * @param height Height of the console window.
     *
     * @details Initializes the console window with the provided dimensions and title.
     */
    DebugConsole (const juce::String& windowTitle, const int width, const int height)
        : jam::Window (
              [width, height]
              {
                  auto log { std::make_unique<Log>() };
                  log->setSize (width, height);
                  return log.release();
              }(),
              windowTitle,
              true,
              false)
    {
        windowInit();
    }

    /**
     * @brief Destructor for DebugConsole.
     */
    ~DebugConsole() = default;

    /**
     * @brief Clears all log messages from the console.
     */
    void clear() { log().clear(); }

    //==============================================================================

    /**
     * @brief Writes a variable and its value to the console using iostream.
     *
     * @tparam Type The type of the variable to print.
     * @param variable The name of the variable.
     * @param value The value of the variable.
     * @param precision Precision for floating-point values (default: 15).
     */
    template<typename Type>
    static void print (const char* variable, const Type& value, int precision = 15)
    {
        std::cout << variable << std::setprecision (precision) << " = " << value << std::endl;
    }

    /**
     * @brief Writes a message to the console using iostream.
     *
     * @tparam Type The type of the message to print.
     * @param messageToPrint The message to display in the console.
     */
    template<typename Type>
    static void print (const Type& messageToPrint)
    {
        std::cout << messageToPrint << std::endl;
    }

private:
    /**
     * @brief Initializes the console window properties.
     *
     * @details Sets up resizing, visibility, and LookAndFeel.
     */
    void windowInit()
    {
        setResizable (true, false);
        juce::Logger::setCurrentLogger (&log());
        setLookAndFeel (&jam::StyleDebug::getShared());
        setVisible (true);
    }

    //==============================================================================
    /**
     * @class Log
     * @brief A custom component that captures and displays stdout and stderr messages.
     *
     * @details The Log class is responsible for redirecting standard output streams
     *          and displaying the captured messages within a text editor component.
     *          It supports clearing the log and restoring original output states.
     */
    class Log
        : public juce::Component
        , public juce::Logger
        , private juce::AsyncUpdater
    {
    public:
        /**
         * @brief Constructor for Log.
         *
         * @param captureStdErrImmediately Whether to capture stderr immediately upon construction.
         * @param captureStdOutImmediately Whether to capture stdout immediately upon construction.
         */
        Log (bool captureStdErrImmediately = true, bool captureStdOutImmediately = true);

        /**
         * @brief Destructor for Log.
         */
        ~Log() override;

        /**
         * @brief Clears all messages stored in the log.
         */
        void clear();

        /**
         * @brief Posts a message directly to the log.
         *
         * @param message The message to log.
         *
         * @details Called automatically if this is set as the current JUCE logger.
         */
        void logMessage (const juce::String& message) override;

        /** @brief Redirects stdout to this component. */
        bool captureStdOut();

        /** @brief Redirects stderr to this component. */
        bool captureStdErr();

        /** @brief Restores the original stdout. */
        void releaseStdOut (bool printRestoreMessage = true);

        /** @brief Restores the original stderr. */
        void releaseStdErr (bool printRestoreMessage = true);

        /**
         * @brief Handles resizing of the log component.
         */
        void resized() override;

    private:
        // File descriptors to restore standard console output streams
        static int originalStdout, originalStderr;

        // Pipes to redirect streams to this component
        static int logStdOutputPipe[2];
        static int logErrOutputPipe[2];

        static std::unique_ptr<std::thread> stdOutReaderThread;
        static std::unique_ptr<std::thread> stdErrReaderThread;

        // Indicates if this is the current stdout or stderr target
        static Log* currentStdOutTarget;
        static Log* currentStdErrTarget;

        static const int tmpBufLen = 512;
        juce::TextEditor consoleOutputEditor;
        juce::Colour stdOutColour { 0xff4e8c93 };
        juce::Colour stdErrColour { 0xfffc704c };
        juce::Colour logMessageColour { 0xff01c2d2 };
        juce::Colour backgroundColour { 0xff090a0b };

        // Text storage for the log component
        int numLinesToStore = 200;
        int numLinesToRemoveWhenFull = 20;
        int numLinesStored = 0;
        int numNewLinesSinceUpdate = 0;
        juce::StringArray lines;
        juce::Array<juce::Colour> lineColours;
        juce::CriticalSection linesLock;

        static bool createAndAssignPipe (int* pipeIDs, FILE* stream);
        static void deletePipeAndEndThread (int original, FILE* stream, std::unique_ptr<std::thread>& thread);

        static void readStdOut();
        static void readStdErr();

        void addFromStd (char* stringBufferToAdd, size_t bufferSize, juce::Colour colourOfString);

        void handleAsyncUpdate() override;
    };

    Log& log() { return *dynamic_cast<Log*> (getContentComponent()); }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DebugConsole)
};

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam

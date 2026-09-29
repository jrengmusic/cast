#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"

/**
 * @struct Toolchain
 * @brief Selects @c ## toolchain and @c ## pack rows by host and by CLI
 *        argument, and runs child processes for them.
 *
 * Toolchain keeps no state. Its selection functions read only the model
 * and the compile-time host.
 */
struct Toolchain
{
    /**
     * @brief Returns the host this binary was built for.
     *
     * @returns @c Id::mac, @c Id::win, or @c Id::linux.
     */
    static const juce::Identifier& getHost() noexcept
    {
       #if JUCE_MAC
        return Id::mac;
       #elif JUCE_WINDOWS
        return Id::win;
       #else
        return Id::linux;
       #endif
    }

    /**
     * @brief Answers whether @p row runs on this host.
     *
     * @param model The model @p row belongs to.
     * @param row   The @c ## toolchain or @c ## pack row.
     * @returns @c true when the @c host cell is empty or names the host
     *          that getHost() returns.
     */
    static bool isHostSelected (const Model& model, const Model::Element& row)
    {
        const auto host { model.getColumnValue (row, Id::host) };

        return host.isEmpty() or juce::Identifier (host) == getHost();
    }

    /**
     * @brief Answers whether @p row is selected by both the CLI argument
     *        and the host.
     *
     * @param model             The model @p row belongs to.
     * @param row               The @c ## toolchain or @c ## pack row.
     * @param toolchainArgument The CLI-selected toolchain group. Empty
     *                          selects the rows with a blank @c argument
     *                          cell.
     * @returns @c true when the @c argument cell equals
     *          @p toolchainArgument and isHostSelected() holds.
     */
    static bool isRowSelected (const Model& model, const Model::Element& row, const juce::String& toolchainArgument)
    {
        return model.getColumnValue (row, Id::argument).compare (toolchainArgument) == 0 and isHostSelected (model, row);
    }

    /**
     * @brief Resolves @p path against the current working directory.
     *
     * @param path A relative or absolute path.
     * @returns The file @p path names, relative to the current working
     *          directory when @p path is relative.
     */
    static juce::File getWorkingFile (const juce::String& path)
    {
        return juce::File::getCurrentWorkingDirectory().getChildFile (path);
    }

    /**
     * @brief Tokenizes @p command and @p flag into one argv array.
     *
     * @param command The toolchain row's own @c command, placed at
     *                argument index zero.
     * @param flag    The toolchain row's own @c flag, tokenized after
     *                @p command.
     * @returns @p command followed by @p flag's own whitespace-tokenized
     *          arguments.
     */
    static juce::StringArray getToolchainArguments (const juce::String& command, const juce::String& flag)
    {
        juce::StringArray arguments { command };
        arguments.addTokens (flag, true);

        return arguments;
    }

    /**
     * @brief Runs @p arguments as a child process in the current working
     *        directory, streaming its output to stdout and blocking until
     *        it exits.
     *
     * @param arguments      The process argv, index zero the executable.
     * @param diagnosticLine The command line named in a failure, when the
     *                       process exits non-zero.
     * @returns juce::Result::ok() when the process exits zero, or a
     *          failure naming @p diagnosticLine.
     */
    static juce::Result runProcess (const juce::StringArray& arguments, const juce::String& diagnosticLine)
    {
        juce::WaitableEvent finished;
        auto exitCode { -1 };

        jam::Subprocess subprocess;
        subprocess.launch (arguments,
                           juce::File::getCurrentWorkingDirectory(),
                           [&finished, &exitCode] (int processExitCode, const std::string&)
                           {
                               exitCode = processExitCode;
                               finished.signal();
                           },
                           [] (std::string_view chunk, bool)
                           {
                               fwrite (chunk.data(), 1, chunk.size(), stdout);
                               fflush (stdout);
                           });

        finished.wait();

        if (exitCode != 0)
            return juce::Result::fail (
                diagnosticLine + Id::diagnosticSeparator + text::Diagnostics::failToolchain);

        return juce::Result::ok();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Toolchain)
};

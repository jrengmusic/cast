/**
 * @file jam_Log.h
 * @brief jam::debug::Log — reusable file-logger primitive.
 *
 * Lives in jam_core so that lower-level headers (e.g. DST diagnostics)
 * can emit log lines without creating a circular dependency on jam_debug.
 */

#pragma once

namespace jam::debug
{
/*____________________________________________________________________________*/

/**
 * @brief Reusable file-logger primitive.
 *
 * Wraps juce::FileLogger behind a static API. The primitive itself is not
 * gated on JUCE_DEBUG; call sites guard diagnostics with \#if JUCE_DEBUG.
 */
struct Log
{
    /**
     * @brief RAII owner for the active diagnostic file logger.
     *
     * Construct one instance (typically as a member of JUCEApplication) to
     * bring the logger online.  The destructor resets the logger before
     * JUCE's static leak detector runs, eliminating the shutdown leak
     * complaint.
     *
     * Non-copyable and non-movable — exactly one Scope may exist at a time.
     */
    struct Scope
    {
        /** @brief Opens (or creates) the log file and activates the logger.
         *  @param logPath  Absolute path to the log file; created if absent. */
        explicit Scope (const juce::File& logPath)
        {
            logFile = logPath;
            logger = std::make_unique<juce::FileLogger> (logPath, juce::String(), 0);
        }

        /** @brief Deactivates the logger and clears the configured path,
         *  leaving both statics in their pre-Scope state. */
        ~Scope()
        {
            logger = nullptr;
            logFile = juce::File();
        }

        Scope (const Scope&) = delete;
        Scope& operator= (const Scope&) = delete;
    };

    /** @brief RAII elapsed-time logger — captures a tag at construction and
     *  writes "[tag]: [elapsed] ms" (3 decimal places) to the diagnostic
     *  log on destruction. Non-copyable and non-movable.
     */
    struct Timer
    {
        /** @brief Starts the timer, capturing @p tag for the destruction log line.
         *  @param tag  Label prepended to the elapsed-ms entry. */
        explicit Timer (const juce::String& tag)
            : label { tag }
            , start { juce::Time::getHighResolutionTicks() }
        {}

        ~Timer()
        {
            const double elapsed { juce::Time::highResolutionTicksToSeconds (
                juce::Time::getHighResolutionTicks() - start) * 1000.0 };
            write (label + ": " + juce::String (elapsed, 3) + " ms");
        }

        Timer (const Timer&) = delete;
        Timer& operator= (const Timer&) = delete;
        Timer (Timer&&) = delete;
        Timer& operator= (Timer&&) = delete;

    private:
        juce::String label;
        juce::int64 start;
    };

    /** @brief Appends one line to the active diagnostic log.
     *  @param message  Line content; newline appended automatically.
     *  @note No-op if no Log::Scope is currently alive.
     *  @note Non-ASCII bytes in @p message are replaced with '?' by sanitize()
     *        before the line is written. String literals whose non-ASCII bytes
     *        are introduced before juce::String construction (e.g. an em-dash
     *        literal) will still assert inside juce::String's own ctor at
     *        juce_String.cpp:327 before write() is ever entered -- use ASCII
     *        alternatives (-- instead of em-dash) in any literal that feeds
     *        juce::String. Runtime strings from external sources are safe: their
     *        non-ASCII bytes pass through juce::String construction unchanged and
     *        are then replaced by sanitize() here. */
    static void write (const juce::String& message)
    {
        if (logger != nullptr)
            logger->logMessage (sanitize (message));
    }

    /** @brief Stringifies two or more arguments, joins them with single spaces,
     *  and delegates to the single-String write() overload.
     *  @param args  Arguments convertible by stringify() -- arithmetic types,
     *               juce::String, const char*, pointers, or types exposing
     *               toString(). Requires at least 2 arguments; single-argument
     *               callers use the juce::String overload directly.
     */
    template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) >= 2)>>
    static void write (Args&&... args)
    {
        juce::StringArray parts;
        (parts.add (stringify (std::forward<Args> (args))), ...);
        write (parts.joinIntoString (" "));
    }

    /** @brief Returns the configured log file path, or an empty File if no
     *  Log::Scope is currently alive.
     *  @note Reusable by external tooling/agents to locate the log. */
    static juce::File getPath() noexcept { return logFile; }

private:
    /** @brief Trait: true when Argument exposes a toString() member returning a juce::String. */
    template <typename Argument, typename = void>
    struct HasToString : std::false_type {};

    /** @brief Specialisation that resolves to true_type when toString() is well-formed on Argument. */
    template <typename Argument>
    struct HasToString<Argument, std::void_t<decltype (std::declval<Argument>().toString())>> : std::true_type {};

    /** @brief Converts a single argument to juce::String for write() assembly.
     *  Dispatches on type: juce::String (identity), const char\* and char\* (ctor),
     *  std::string (ctor), arithmetic (juce::String ctor), pointer (hex address),
     *  toString() member, or "\<unprintable\>" fallback. */
    template <typename Argument>
    static juce::String stringify (Argument&& val)
    {
        using DecayedArgument = std::decay_t<Argument>;
        if constexpr (std::is_same_v<DecayedArgument, juce::String>)
            return std::forward<Argument> (val);
        else if constexpr (std::is_same_v<DecayedArgument, const char*> or std::is_same_v<DecayedArgument, char*>)
            return juce::String (val);
        else if constexpr (std::is_same_v<DecayedArgument, std::string>)
            return juce::String (val);
        else if constexpr (std::is_arithmetic_v<DecayedArgument>)
            return juce::String (val);
        else if constexpr (std::is_pointer_v<DecayedArgument>)
            return juce::String::toHexString (reinterpret_cast<juce::pointer_sized_int> (val));
        else if constexpr (HasToString<DecayedArgument>::value)
            return val.toString();
        else
            return "<unprintable>";
    }

    /** @brief Replaces every byte above 127 in @p msg with '?' and returns
     *  the result as a juce::String safe for FileLogger::logMessage(). */
    static juce::String sanitize (const juce::String& msg)
    {
        std::string raw { msg.toStdString() };
        for (auto& ch : raw)
            if (static_cast<unsigned char> (ch) > 127)
                ch = '?';
        return juce::String { raw };
    }

    static inline std::unique_ptr<juce::FileLogger> logger;
    static inline juce::File logFile;
};

/*____________________________________________________________________________*/
} /** namespace jam::debug */

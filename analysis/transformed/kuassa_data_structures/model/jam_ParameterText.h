/**
 * @file jam_ParameterText.h
 * @brief Self-owned double-buffered text parameter — cross-thread string transport.
 *
 * Owns two juce::HeapBlock<char> pre-allocated at construction. Producer thread writes
 * to the inactive buffer via setValue() then release-stores the active index. Consumer
 * thread acquire-loads the index in getValue(). Lock-free SPSC — different indices on
 * each side, no CAS needed. setValue() notifies all registered Listeners with the new value.
 *
 * Self-contained ownership: ParameterText owns its buffers internally and accepts
 * a copy at the API boundary.
 *
 * @see jam::ValueTreeState
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/
/**
 * @struct ParameterText
 * @brief Self-owned double-buffered text parameter. Cross-thread string transport with zero hot-path allocation.
 *
 * Owns two juce::HeapBlock<char> pre-allocated at construction. Producer thread writes
 * to the inactive buffer via setValue() then release-stores the active index. Consumer
 * thread acquire-loads the index in getValue(). Lock-free SPSC — different indices on
 * each side, no CAS needed.
 *
 * setValue() notifies all registered Listeners with the new value (juce::var wrapping
 * the just-written string). Atomic value holder with Listener notification — no flush,
 * no ValueTree, no adapter concerns.
 */
struct ParameterText final : ParameterBase
{
    /**
     * @brief Constructs a ParameterText with self-identifying id, property key, max buffer size, and default value.
     * @param parameterId  Self-identifying parameter identifier.
     * @param newPropertyId Property identifier for this text parameter. Stored as base class propertyId.
     * @param maxlen       Maximum string length in bytes (each buffer is allocated to this size).
     * @param defaultText  Initial value seeded into both buffers.
     */
    ParameterText (const juce::Identifier& parameterId,
                   const juce::Identifier& newPropertyId,
                   int maxlen,
                   const juce::String& defaultText) noexcept
        : ParameterBase { parameterId, newPropertyId }
        , buffers { juce::HeapBlock<char> { static_cast<size_t> (maxlen) },
                    juce::HeapBlock<char> { static_cast<size_t> (maxlen) } }
        , active { 0 }
        , bufferSize { maxlen }
    {
        const int len { juce::jmin (static_cast<int> (defaultText.getNumBytesAsUTF8()), bufferSize - 1) };
        const char* src { defaultText.toRawUTF8() };

        std::memcpy (buffers[0].getData(), src, static_cast<size_t> (len));
        buffers[0][len] = '\0';
        std::memcpy (buffers[1].getData(), src, static_cast<size_t> (len));
        buffers[1][len] = '\0';
    }

    /**
     * @brief Lock-free producer-side store. Copies src into the inactive buffer, null-terminates,
     *        release-stores the new active index, and notifies all listeners. Any thread.
     * @param src     Source bytes (need not be null-terminated).
     * @param length  Number of bytes to copy (truncated to maxlen-1 if longer).
     */
    void setValue (const char* src, int length) noexcept
    {
        const int next { 1 - active.load (std::memory_order_relaxed) };
        const int len  { juce::jmin (length, bufferSize - 1) };
        std::memcpy (buffers[next].getData(), src, static_cast<size_t> (len));
        buffers[next][len] = '\0';
        active.store (next, std::memory_order_release);
        sendValueChangedMessageToListeners (juce::var { getValue() });
    }

    /**
     * @brief Convenience overload for juce::String.
     */
    void setValue (const juce::String& text) noexcept
    {
        setValue (text.toRawUTF8(), static_cast<int> (text.getNumBytesAsUTF8()));
    }

    /**
     * @brief Acquire-loads the active buffer and returns it as a juce::String. Any thread.
     * @return The current text value. Empty string if nothing has been stored yet.
     */
    juce::String getValue() const noexcept
    {
        const int idx { active.load (std::memory_order_acquire) };
        return juce::String::fromUTF8 (buffers[idx].getData());
    }

    void setValueFromVar (const juce::var& v) noexcept override { setValue (v.toString()); }
    juce::var getValueAsVar() const noexcept override { return juce::var { getValue() }; }

private:
    juce::HeapBlock<char> buffers[2];
    std::atomic<int> active { 0 };
    int bufferSize;
};
/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam

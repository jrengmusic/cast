/**
 * @file jam_ParameterBase.h
 * @brief Typed parameter value holders with Listener-based change notification.
 *
 * Parameter wraps a std::atomic for lock-free cross-thread read/write and
 * owns a Listener interface for value-change notification. Atomic value holder
 * with Listener notification — no flush, no ValueTree, no adapter concerns.
 * JUCE analog: AudioProcessorParameter (value + listener only).
 *
 * Three specializations:
 * - Parameter<int>: discrete int/bool parameter transport.
 * - Parameter<int64_t>: 64-bit integer transport. Used for packed composite
 *   parameters (e.g. TerminalWinsize, Cell::Rectangle).
 * - Parameter<float>: continuous float parameter transport.
 *
 * Primary template:
 * - Parameter<T>: packed-domain-type transport for any trivially-copyable T
 *   with a 4- or 8-byte backing (jam::UUID, jam::Bounds, jam::Size<T>).
 *   Storage is always std::atomic<int64_t> — a 4-byte T widens through
 *   uint32_t on write and narrows back on read via jam::bit_cast.
 *
 * - ParameterText: cross-thread text transport. Self-owned double buffer with
 *   lock-free active-index swap. (See jam_parameter_text.h.)
 *
 * ParameterAdapter (internal to Model) bridges Parameter to ValueTree.
 *
 * @see jam::ValueTreeState
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/
/**
 * @struct ParameterBase
 * @brief Base class for typed parameter values.
 *
 * Owns the parameter identifier and a Listener interface for value-change
 * notification. JUCE analog: AudioProcessorParameter.
 *
 * Each parameter carries its own identifier (following the JUCE
 * AudioProcessorParameter pattern where each parameter self-identifies).
 */
struct ParameterBase
{
    /** @brief Parameter value change listener.
     *  JUCE analog: AudioProcessorParameter::Listener.
     */
    struct Listener
    {
        virtual ~Listener() = default;

        /** @brief Called when the parameter's value changes.
         *  @param id        Parameter identifier.
         *  @param newValue  The new value (type-erased).
         *  @note Fires on the calling thread — may be any thread.
         */
        virtual void parameterValueChanged (const juce::Identifier& id,
                                            const juce::var& newValue) = 0;
    };

    /** @brief Constructs a ParameterBase with self-identifying id and VT property binding.
     *  @param parameterId   Self-identifying parameter identifier.
     *  @param newPropertyId VT property name. Default Id::value (PARAM pattern).
     *                       Override when the value lives on a SECTION node's property
     *                       rather than a PARAM child. */
    ParameterBase (const juce::Identifier& parameterId,
                   const juce::Identifier& newPropertyId = Id::value) noexcept
        : id { parameterId }
        , propertyId { newPropertyId }
    {
    }

    virtual ~ParameterBase() = default;

    /** @brief Registers a parameter value listener.
     *  @note Any thread (CriticalSection-guarded). */
    void addListener (Listener* listener) noexcept
    {
        const juce::CriticalSection::ScopedLockType lock (listenerLock);
        listeners.add (listener);
    }

    /** @brief Removes a parameter value listener.
     *  @note Any thread (CriticalSection-guarded). */
    void removeListener (Listener* listener) noexcept
    {
        const juce::CriticalSection::ScopedLockType lock (listenerLock);
        listeners.removeFirstMatchingValue (listener);
    }

    /** @brief Broadcasts the new value to all registered listeners.
     *  @param newValue  The new value (type-erased).
     *  @note Fires on the calling thread.
     */
    void sendValueChangedMessageToListeners (const juce::var& newValue) noexcept
    {
        const juce::CriticalSection::ScopedLockType lock (listenerLock);

        for (int i { listeners.size() }; --i >= 0;)
            listeners.getUnchecked (i)->parameterValueChanged (id, newValue);
    }

    /** @brief Type-erased value write. Specializations cast from var.
     *  JUCE analog: AudioProcessorParameter::setValue(float).
     *  @note Any thread. */
    virtual void setValueFromVar (const juce::var& newValue) noexcept = 0;

    /** @brief Type-erased value read. Specializations wrap to var.
     *  JUCE analog: AudioProcessorParameter::getValue().
     *  @note Any thread. */
    virtual juce::var getValueAsVar() const noexcept = 0;

    /** @brief Self-identifying parameter identifier (mirrors RangedAudioParameter::paramID). */
    const juce::Identifier id;

    /** @brief VT property name. Default Id::value (PARAM pattern). Override when the
     *  value lives on a SECTION node's property (e.g. a config section's primitive-typed
     *  attributes bound to atomic transport). */
    juce::Identifier propertyId;

private:
    juce::CriticalSection listenerLock;
    juce::Array<Listener*> listeners;
};

template <typename T>
struct Parameter;

/**
 * @struct Parameter<int>
 * @brief Atomic int value holder with Listener notification.
 *
 * setValue()/getValue() are lock-free relaxed-order operations.
 * setValue() notifies all registered Listeners with the new value.
 *
 * Bool parameters are stored as int (0/1) — no separate bool specialization.
 */
template <>
struct Parameter<int> final : ParameterBase
{
    /**
     * @brief Constructs a Parameter<int> with self-identifying id, initial value, and property binding.
     * @param parameterId   Self-identifying parameter identifier.
     * @param defaultValue  Initial atomic value.
     * @param newPropertyId VT property name. Default Id::value (PARAM pattern).
     *                      Override to bind a SECTION node's primitive property.
     */
    Parameter (const juce::Identifier& parameterId, int defaultValue,
               const juce::Identifier& newPropertyId = Id::value) noexcept
        : ParameterBase { parameterId, newPropertyId }
        , value { defaultValue }
    {
    }

    /** @brief Lock-free relaxed load. Any thread. */
    int getValue() const noexcept { return value.load (std::memory_order_relaxed); }

    /** @brief Lock-free relaxed store. Notifies all listeners. Any thread. */
    void setValue (int v) noexcept
    {
        value.store (v, std::memory_order_relaxed);
        sendValueChangedMessageToListeners (juce::var { v });
    }

    /** @brief Direct access to the underlying atomic for explicit memory-order operations. */
    std::atomic<int>& getRawValue() noexcept { return value; }

    void setValueFromVar (const juce::var& v) noexcept override { setValue (static_cast<int> (v)); }
    juce::var getValueAsVar() const noexcept override { return juce::var { getValue() }; }

private:
    std::atomic<int> value;
};

/**
 * @struct Parameter<int64_t>
 * @brief Atomic int64 value holder with Listener notification.
 *
 * Identical to Parameter<int> but for 64-bit values. Used for packed
 * composite parameters (e.g. TerminalWinsize, Cell::Rectangle).
 * setValue()/getValue() are lock-free relaxed-order operations.
 * std::atomic<int64_t> is lock-free on x86-64 and ARM64.
 */
template <>
struct Parameter<int64_t> final : ParameterBase
{
    /**
     * @brief Constructs a Parameter<int64_t> with self-identifying id, initial value, and property binding.
     * @param parameterId   Self-identifying parameter identifier.
     * @param defaultValue  Initial atomic value.
     * @param newPropertyId VT property name. Default Id::value (PARAM pattern).
     *                      Override to bind a SECTION node's primitive property.
     */
    Parameter (const juce::Identifier& parameterId, int64_t defaultValue,
               const juce::Identifier& newPropertyId = Id::value) noexcept
        : ParameterBase { parameterId, newPropertyId }
        , value { defaultValue }
    {
    }

    /** @brief Lock-free relaxed load. Any thread. */
    int64_t getValue() const noexcept { return value.load (std::memory_order_relaxed); }

    /** @brief Lock-free relaxed store. Notifies all listeners. Any thread. */
    void setValue (int64_t v) noexcept
    {
        value.store (v, std::memory_order_relaxed);
        sendValueChangedMessageToListeners (juce::var { v });
    }

    /** @brief Direct access to the underlying atomic for explicit memory-order operations. */
    std::atomic<int64_t>& getRawValue() noexcept { return value; }

    void setValueFromVar (const juce::var& v) noexcept override { setValue (static_cast<int64_t> (v)); }
    juce::var getValueAsVar() const noexcept override { return juce::var { getValue() }; }

private:
    std::atomic<int64_t> value;

    static_assert (std::atomic<int64_t>::is_always_lock_free,
                   "Parameter<int64_t> requires lock-free std::atomic<int64_t>");
};

/**
 * @struct Parameter<float>
 * @brief Atomic float value holder with Listener notification.
 *
 * setValue()/getValue() are lock-free relaxed-order operations.
 * setValue() notifies all registered Listeners with the new value.
 */
template <>
struct Parameter<float> final : ParameterBase
{
    /**
     * @brief Constructs a Parameter<float> with self-identifying id, initial value, and property binding.
     * @param parameterId   Self-identifying parameter identifier.
     * @param defaultValue  Initial atomic value.
     * @param newPropertyId VT property name. Default Id::value (PARAM pattern).
     *                      Override to bind a SECTION node's primitive property.
     */
    Parameter (const juce::Identifier& parameterId, float defaultValue,
               const juce::Identifier& newPropertyId = Id::value) noexcept
        : ParameterBase { parameterId, newPropertyId }
        , value { defaultValue }
    {
    }

    /** @brief Lock-free relaxed load. Any thread. */
    float getValue() const noexcept { return value.load (std::memory_order_relaxed); }

    /** @brief Lock-free relaxed store. Notifies all listeners. Any thread. */
    void setValue (float v) noexcept
    {
        value.store (v, std::memory_order_relaxed);
        sendValueChangedMessageToListeners (juce::var { static_cast<double> (v) });
    }

    /** @brief Direct access to the underlying atomic for explicit memory-order operations. */
    std::atomic<float>& getRawValue() noexcept { return value; }

    void setValueFromVar (const juce::var& v) noexcept override { setValue (static_cast<float> (v)); }
    juce::var getValueAsVar() const noexcept override { return juce::var { static_cast<double> (getValue()) }; }

private:
    std::atomic<float> value;
};

/**
 * @struct Parameter
 * @brief Atomic packed-domain-type value holder with Listener notification (primary template).
 *
 * Packed composite transport for trivially-copyable domain types whose backing
 * fits a 32- or 64-bit word — jam::UUID, jam::Bounds, jam::Size<...>,
 * or any future jam::Union-based packed quad. Storage is always
 * std::atomic<int64_t>: an 8-byte T bit-casts directly to/from the atomic; a
 * 4-byte T widens through uint32_t on write and narrows back through uint32_t
 * on read. setValue()/getValue() are lock-free relaxed-order operations.
 * setValue() notifies all registered Listeners with the packed int64 var.
 *
 * @tparam T  Packed domain type. Must be trivially copyable with sizeof(T)
 *            equal to 4 or 8 bytes.
 *
 * @code
 * Parameter<jam::UUID> owner { Id::owner, jam::UUID {} };
 * auto uuid { owner.getValue() };
 *
 * Parameter<jam::Bounds> bounds { Id::bounds, jam::Bounds { rect } };
 * auto [x, y, width, height] { bounds.getValue() };
 * @endcode
 */
template <typename T>
struct Parameter final : ParameterBase
{
    static_assert (std::is_trivially_copyable_v<T> and (sizeof (T) == 4 or sizeof (T) == 8),
                   "Parameter<T> packed domain type: trivially copyable, 32/64-bit backing");

    /**
     * @brief Constructs a Parameter<T> with self-identifying id, initial value, and property binding.
     * @param parameterId   Self-identifying parameter identifier.
     * @param defaultValue  Initial value, packed into the atomic on construction.
     * @param newPropertyId VT property name. Default Id::value (PARAM pattern).
     *                      Override to bind a SECTION node's primitive property.
     */
    Parameter (const juce::Identifier& parameterId, T defaultValue,
               const juce::Identifier& newPropertyId = Id::value) noexcept
        : ParameterBase { parameterId, newPropertyId }
        , value { pack (defaultValue) }
    {
    }

    /** @brief Lock-free relaxed load, unpacked from the atomic backing. Any thread. */
    T getValue() const noexcept { return unpack (value.load (std::memory_order_relaxed)); }

    /** @brief Lock-free relaxed store. Notifies all listeners with the packed int64 var. Any thread. */
    void setValue (T v) noexcept
    {
        const auto raw { pack (v) };
        value.store (raw, std::memory_order_relaxed);
        sendValueChangedMessageToListeners (juce::var { raw });
    }

    /** @brief Direct access to the underlying packed atomic for explicit memory-order operations. */
    std::atomic<int64_t>& getRawValue() noexcept { return value; }

    void setValueFromVar (const juce::var& v) noexcept override
    {
        setValue (unpack (static_cast<int64_t> (v)));
    }

    juce::var getValueAsVar() const noexcept override
    {
        return juce::var { value.load (std::memory_order_relaxed) };
    }

private:
    /** @brief Packs T into the int64 atomic backing via jam::bit_cast.
     *  8-byte T bit-casts directly; 4-byte T bit-casts through uint32_t then widens.
     */
    static int64_t pack (T v) noexcept
    {
        if constexpr (sizeof (T) == 8)
            return jam::bit_cast<int64_t> (v);
        else
            return static_cast<int64_t> (jam::bit_cast<uint32_t> (v));
    }

    /** @brief Unpacks T from the int64 atomic backing via jam::bit_cast.
     *  8-byte T bit-casts directly; 4-byte T narrows through uint32_t first.
     */
    static T unpack (int64_t raw) noexcept
    {
        if constexpr (sizeof (T) == 8)
            return jam::bit_cast<T> (raw);
        else
            return jam::bit_cast<T> (static_cast<uint32_t> (raw));
    }

    std::atomic<int64_t> value;
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam

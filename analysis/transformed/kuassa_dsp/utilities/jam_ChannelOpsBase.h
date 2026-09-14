namespace jam::dsp
{
/*____________________________________________________________________________*/

//==============================================================================
/**
 * @file jam_ChannelOpsBase.h
 * @brief CRTP-based channel wrappers for stereo/M/S/L/R processing
 *
 * DESIGN PRINCIPLES:
 *
 * 1. FAIL FAST (Architectural Pillar):
 *    - All array access uses std::array::at() for bounds checking
 *    - Prevents undefined behavior from invalid channel indices
 *    - Provides immediate, debuggable errors (exception with stack trace)
 *    - Performance cost: negligible (~1 branch, perfect prediction for NumChannels ≤ 16)
 *
 * 2. TRANSPARENT WRAPPER:
 *    Exposes DSP operations in three categories:
 *
 *    a) UNIVERSAL (all DSP classes have these):
 *       - setSampleRate(), setBypassed(), reset()
 *       Usage: filter.setSampleRate(48000.0);
 *
 *    b) COMMON (most filter classes have these):
 *       - setFrequency(), setGain(), setQ(), setBandwidth(), setSlope(), setType(), setInverted()
 *       Usage: filter.setFrequency(1000.0);
 *
 *    c) SPECIFIC (class-specific operations):
 *       - Access via: filter.channels.at(i).specificMethod()
 *       Usage: filter.channels.at(0).setCustomParameter(value);
 *
 * 3. TRIVIALLY COPYABLE (DSP Contract):
 *    - Uses std::array (trivially copyable when ProcessorType is)
 *    - No virtual functions, no dynamic allocation
 *    - Enables lock-free state snapshots for SmoothStateTransition
 *    - Verified by static_assert at end of file
 *
 * USAGE EXAMPLES:
 *
 * @code
 * using ButterworthMSED = ChannelProcessorMSED<Butterworth<StateVariable>, 2>;
 * ButterworthMSED filter;
 *
 * // Common operations - use wrapper methods
 * filter.setFrequency(1000.0);
 * filter.setSampleRate(48000.0);
 * filter.setSlope(24);
 *
 * // Specific operations - direct access with bounds checking
 * filter.channels.at(0).setBandwidth(2.0);
 *
 * // Apply to all channels - use range-for
 * for (auto& ch : filter.channels)
 *     ch.reset();
 *
 * // Or indexed loop with .at()
 * for (size_t i = 0; i < filter.channels.size(); ++i)
 *     filter.channels.at(i).setSlope(24);
 * @endcode
 */

/**
 * @brief CRTP base class providing common channel operations (DRY principle).
 *
 * Provides shared operations for all channel processors without virtual functions.
 * Uses CRTP (Curiously Recurring Template Pattern) for static polymorphism.
 *
 * @tparam Derived The derived class type (CRTP pattern)
 * @tparam ProcessorType The type of processor to wrap (must be trivially copyable)
 * @tparam NumChannels Number of channels (typically 2 for stereo)
 *
 * @note This class follows the DSP Architectural Contract:
 *       - Trivially copyable (no virtual functions, no dynamic allocation)
 *       - Rule of Zero (compiler-generated special members are correct)
 *       - Bounds-checked access (Fail Fast principle via std::array::at())
 */
template <typename Derived, typename ProcessorType, size_t NumChannels>
struct ChannelOpsBase
{
    std::array<ProcessorType, NumChannels> channels;// Bounds-checked array (trivially copyable!)

    /** @brief Default constructor for the ChannelOpsBase class. */
    ChannelOpsBase() = default;

    /**
     * @brief Constructs every channel by forwarding the same arguments to each ProcessorType.
     * @tparam Args Constructor argument types forwarded to each channel's ProcessorType.
     * @param args Arguments forwarded to construct each channel.
     */
    template <typename... Args>
    explicit ChannelOpsBase(Args&&... args)
        : channels{ {ProcessorType(std::forward<Args>(args)...),
                    ProcessorType(std::forward<Args>(args)...)} } // Initialize each channel with forwarded args
    {
        static_assert(NumChannels > 0, "Number of channels must be greater than 0");
    }

    /**
     * @brief Returns this object statically cast to its CRTP-derived type.
     * @return A reference to the derived instance.
     */
    Derived& derived() noexcept
    {
        return static_cast<Derived&> (*this);
    }

    /**
     * @brief Returns this object statically cast to its CRTP-derived type.
     * @return A const reference to the derived instance.
     */
    const Derived& derived() const noexcept
    {
        return static_cast<const Derived&> (*this);
    }

    // Shared operations via CRTP (DRY principle)
    // All use range-for (no index needed, inherently bounds-safe)
    /**
     * @brief Sets the bypass state on every channel.
     * @param shouldBeBypassed True to bypass every channel, false to process normally.
     */
    void setBypassed (bool shouldBeBypassed) noexcept
    {
        for (auto& channel : channels)
            channel.setBypassed (shouldBeBypassed);
    }

    /**
     * @brief Sets the sample rate on every channel.
     * @param sampleRate The new sample rate in Hz.
     */
    void setSampleRate (double sampleRate) noexcept
    {
        for (auto& channel : channels)
            channel.setSampleRate (sampleRate);
    }

    /**
     * @brief Prepares every channel with the given sample rate and maximum block size.
     * @param sampleRate The sample rate in Hz.
     * @param maxBlockSize The maximum number of samples per processing block.
     */
    void prepare (double sampleRate, int maxBlockSize) noexcept
    {
        for (auto& channel : channels)
            channel.prepare (sampleRate, maxBlockSize);
    }

    /**
     * @brief Sets the frequency on every channel.
     * @param frequency The new frequency in Hz.
     */
    void setFrequency (double frequency) noexcept
    {
        for (auto& channel : channels)
            channel.setFrequency (frequency);
    }

    /**
     * @brief Sets the gain on every channel.
     * @param gain The new gain value.
     */
    void setGain (double gain) noexcept
    {
        for (auto& channel : channels)
            channel.setGain (gain);
    }

    /**
     * @brief Sets the value range on every channel.
     * @param minValue The minimum value of the range.
     * @param maxValue The maximum value of the range.
     */
    void setRange (double minValue, double maxValue) noexcept
    {
        for (auto& channel : channels)
            channel.setRange (minValue, maxValue);
    }

    /**
     * @brief Sets the Q factor on every channel.
     * @param q The new Q factor.
     */
    void setQ (double q) noexcept
    {
        for (auto& channel : channels)
            channel.setQ (q);
    }

    /**
     * @brief Sets the bandwidth on every channel.
     * @param bandwidth The new bandwidth value.
     */
    void setBandwidth (double bandwidth) noexcept
    {
        for (auto& channel : channels)
            channel.setBandwidth (bandwidth);
    }

    /**
     * @brief Sets the slope on every channel.
     * @param slope The new slope value in dB per octave.
     */
    void setSlope (double slope) noexcept
    {
        for (auto& channel : channels)
            channel.setSlope (slope);
    }

    /**
     * @brief Sets the filter type on every channel.
     * @param type The new filter type.
     */
    void setType (Filter::Type type) noexcept
    {
        for (auto& channel : channels)
            channel.setType (type);
    }

    /**
     * @brief Sets the inverted state on every channel.
     * @param shouldBeInverted True to invert every channel's filter type, false otherwise.
     */
    void setInverted (bool shouldBeInverted) noexcept
    {
        for (auto& channel : channels)
            channel.setInverted (shouldBeInverted);
    }

    /**
     * @brief Resets every channel's internal state.
     */
    void reset() noexcept
    {
        for (auto& channel : channels)
            channel.reset();
    }

    /**
     * @brief Retrieves the filter type from the first channel.
     * @return The filter type of channel 0, or a default-constructed Filter::Type if NumChannels is 0.
     */
    Filter::Type getType() const noexcept
    {
        if constexpr (NumChannels > 0)
            return channels.at (0).getType();
        return {};// Should not happen
    }
};

//==============================================================================
/**
 * @brief Simple stereo processor without M/S routing.
 *
 * Processes two channels independently (left and right).
 * Trivially copyable for use with SmoothStateTransition.
 *
 * @tparam ProcessorType The DSP processor type (must be trivially copyable)
 * @tparam NumChannels Number of channels (default: 2 for stereo)
 *
 * @par Example:
 * @code
 * using Orfanidis = jam::dsp::Orfanidis;
 * using StereoOrfanidis = jam::dsp::ChannelProcessor<Orfanidis, 2>;
 *
 * StereoOrfanidis filter;
 * filter.setSampleRate(48000.0);
 * filter.setFrequency(1000.0);
 * filter.process(leftSample, rightSample);  // Processes both channels
 * @endcode
 */

template <typename ProcessorType, size_t NumChannels = 2>
struct ChannelProcessor
    : ChannelOpsBase<ChannelProcessor<ProcessorType, NumChannels>, ProcessorType, NumChannels>
{
    using Base = ChannelOpsBase<ChannelProcessor, ProcessorType, NumChannels>;
    using Base::channels;

    /** @brief Default constructor for the ChannelProcessor class. */
    ChannelProcessor() = default;

    /**
     * @brief Constructs every channel by forwarding the same arguments to each ProcessorType.
     * @tparam Args Constructor argument types forwarded to each channel's ProcessorType.
     * @param args Arguments forwarded to construct each channel.
     */
    template <typename... Args>
    explicit ChannelProcessor(Args&&... args)
        : Base(std::forward<Args>(args)...)
    {
    }

    /**
     * @brief Processes a single sample through the specified channel.
     * @tparam SampleType The sample's numeric type.
     * @param channel The channel index to process through.
     * @param sample The sample to process in place.
     */
    template <typename SampleType>
    void process (int channel, SampleType& sample) noexcept
    {
        channels.at (channel).process (sample);
    }

    /**
     * @brief Processes a stereo sample pair, each channel independently.
     * @tparam SampleType The sample's numeric type.
     * @param leftSample The left sample to process in place.
     * @param rightSample The right sample to process in place.
     */
    template <typename SampleType>
    void process (SampleType& leftSample, SampleType& rightSample) noexcept
    {
        enum
        {
            left = 0,
            right = 1
        };

        channels.at (left).process (leftSample);
        channels.at (right).process (rightSample);
    }
};

//==============================================================================
/**
 * @brief Stereo processor with M/S/L/R routing support.
 *
 * Extends ChannelProcessor with mid/side encoding and channel routing.
 * Trivially copyable for use with SmoothStateTransition.
 *
 * @tparam ProcessorType The DSP processor type (must be trivially copyable)
 * @tparam NumChannels Number of channels (default: 2 for stereo)
 *
 * @par Routing Modes:
 * - stereo: Process both channels independently
 * - mid: Process mid (sum) signal only
 * - side: Process side (difference) signal only
 * - left: Process left channel only
 * - right: Process right channel only
 *
 * @par Example:
 * @code
 * using Butterworth = jam::dsp::Butterworth<jam::dsp::StateVariable>;
 * using ButterworthMSED = jam::dsp::ChannelProcessorMSED<Butterworth, 2>;
 *
 * ButterworthMSED filter;
 * filter.setChannelProcessing(jam::dsp::Channel::mid);  // Process mid signal
 * filter.process(leftSample, rightSample);
 * @endcode
 */
template <typename ProcessorType, size_t NumChannels = 2>
struct ChannelProcessorMSED
    : ChannelOpsBase<ChannelProcessorMSED<ProcessorType, NumChannels>, ProcessorType, NumChannels>
{
    using Base = ChannelOpsBase<ChannelProcessorMSED, ProcessorType, NumChannels>;
    using Base::channels;

    /** @brief Default constructor for the ChannelProcessorMSED class. */
    ChannelProcessorMSED() = default;

    /**
     * @brief Constructs every channel by forwarding the same arguments to each ProcessorType.
     * @tparam Args Constructor argument types forwarded to each channel's ProcessorType.
     * @param args Arguments forwarded to construct each channel.
     */
    template <typename... Args>
    explicit ChannelProcessorMSED(Args&&... args)
        : Base(std::forward<Args>(args)...)
    {
    }

    /** @brief Active channel routing mode (stereo, mid, side, left, or right). */
    Channel::processing channelMode { Channel::off };

    /**
     * @brief Sets the channel routing mode used by the stereo process() overload.
     * @param mode The routing mode to apply.
     */
    void setChannelProcessing (Channel::processing mode) noexcept
    {
        channelMode = mode;
    }

    /**
     * @brief Processes a single sample through the specified channel.
     * @tparam SampleType The sample's numeric type.
     * @param channel The channel index to process through.
     * @param sample The sample to process in place.
     */
    template <typename SampleType>
    void process (int channel, SampleType& sample) noexcept
    {
        channels.at (channel).process (sample);
    }

    /**
     * @brief Processes a stereo sample pair according to the current channel routing mode.
     * @tparam SampleType The sample's numeric type.
     * @param leftSample The left sample to process in place.
     * @param rightSample The right sample to process in place.
     */
    template <typename SampleType>
    void process (SampleType& leftSample, SampleType& rightSample) noexcept
    {
        enum
        {
            left = 0,
            right = 1
        };

        switch (channelMode)
        {
            case Channel::stereo:
                channels.at (left).process (leftSample);
                channels.at (right).process (rightSample);
                break;

            case Channel::mid:
            {
                auto processLeft = [&] (SampleType& sample)
                {
                    channels.at (left).process (sample);
                };
                Channel::processMid (leftSample, rightSample, processLeft);
            }
            break;

            case Channel::side:
            {
                auto processRight = [&] (SampleType& sample)
                {
                    channels.at (right).process (sample);
                };
                Channel::processSide (leftSample, rightSample, processRight);
            }
            break;

            case Channel::left:
                channels.at (left).process (leftSample);
                break;

            case Channel::right:
                channels.at (right).process (rightSample);
                break;

            default:
                break;
        }
    }
};

//==============================================================================
// Verify DSP contract: trivially copyable (enables lock-free state snapshots)
static_assert (std::is_trivially_copyable_v<ChannelProcessor<int, 2>>,
               "ChannelProcessor must be trivially copyable per DSP contract");
static_assert (std::is_trivially_copyable_v<ChannelProcessorMSED<int, 2>>,
               "ChannelProcessorMSED must be trivially copyable per DSP contract");

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp

/**
 * @file jam_SpectrumFIFO.h
 * @brief Lock-free audio-thread-to-analysis-thread ring buffer for spectrum analysis.
 */

#pragma once

namespace jam
{

/**
 * @class SpectrumFIFO
 * @brief Lock-free per-channel ring buffer bridging audio-thread writes to a background analysis thread.
 *
 * Accepts float or double audio blocks on the audio thread via process(),
 * storing samples in per-channel juce::AbstractFifo-backed ring buffers of
 * both sample types. A consumer thread pulls fixed-size windows via
 * pullFloat()/pullDouble(), advancing the read pointer by half the
 * requested window so consecutive pulls overlap by 50%.
 */
class SpectrumFIFO
{
public:
    /**
     * @brief Default constructor for the SpectrumFIFO class.
     */
    SpectrumFIFO() = default;

    /**
     * @brief Allocates per-channel ring buffers sized for the given FFT size and channel count.
     *
     * Clears and resizes both the float and double AbstractFifo instances and
     * their backing buffers, sized to four times fftSize per channel.
     *
     * @param fftSize The FFT size the ring buffers are sized against.
     * @param numChannels The number of channels to allocate buffers for (clamped to at least 1).
     */
    void prepare (int fftSize, int numChannels)
    {
        maxChannels = juce::jmax (1, numChannels);
        const int bufferSize { fftSize * 4 };

        abstractFifoFloat.clear();
        abstractFifoDouble.clear();
        bufferFloat.clear();
        bufferDouble.clear();

        abstractFifoFloat.clear();
        abstractFifoDouble.clear();
        bufferFloat.clear();
        bufferDouble.clear();

        bufferFloat.resize (static_cast<size_t> (maxChannels));
        bufferDouble.resize (static_cast<size_t> (maxChannels));

        for (int ch { 0 }; ch < maxChannels; ++ch)
        {
            const auto sz { static_cast<size_t> (ch) };
            abstractFifoFloat.add (std::make_unique<juce::AbstractFifo> (bufferSize));
            abstractFifoDouble.add (std::make_unique<juce::AbstractFifo> (bufferSize));
            bufferFloat.at (sz).allocate (bufferSize, true);
            bufferDouble.at (sz).allocate (bufferSize, true);
        }
    }

    /**
     * @brief Pushes an audio block's samples into the per-channel ring buffers. Audio-thread safe.
     *
     * Dispatches to the double or float ring buffer per channel based on
     * SampleType via if constexpr. No-op if the block has no samples or channels.
     *
     * @tparam SampleType The sample type of the block (float or double).
     * @param block The audio block whose channels are pushed into the FIFO.
     */
    template <typename SampleType>
    void process (const juce::dsp::AudioBlock<SampleType>& block) noexcept
    {
        const int numSamples { static_cast<int> (block.getNumSamples()) };
        const int numCh { juce::jmin (static_cast<int> (block.getNumChannels()), maxChannels) };

        if (numSamples <= 0 or numCh <= 0)
            return;

        for (int ch { 0 }; ch < numCh; ++ch)
        {
            if constexpr (std::is_same_v<SampleType, double>)
            {
                pushChannel (*abstractFifoDouble.at (static_cast<size_t> (ch)),
                             bufferDouble.at (static_cast<size_t> (ch)),
                             block.getChannelPointer (static_cast<size_t> (ch)),
                             numSamples);
            }
            else
            {
                pushChannel (*abstractFifoFloat.at (static_cast<size_t> (ch)),
                             bufferFloat.at (static_cast<size_t> (ch)),
                             block.getChannelPointer (static_cast<size_t> (ch)),
                             numSamples);
            }
        }
    }

    /**
     * @brief Resets every channel's float and double ring buffers to empty.
     */
    void reset() noexcept
    {
        const int ready { static_cast<int> (abstractFifoFloat.size()) };

        for (int ch { 0 }; ch < ready; ++ch)
        {
            abstractFifoFloat.at (static_cast<size_t> (ch))->reset();
            abstractFifoDouble.at (static_cast<size_t> (ch))->reset();
        }
    }

    /**
     * @brief Retrieves the number of samples ready to read from a channel's float ring buffer.
     * @param channel The channel index to query.
     * @return The number of samples ready to read.
     */
    int getNumReadyFloat (int channel) const noexcept
    {
        return abstractFifoFloat.at (static_cast<size_t> (channel))->getNumReady();
    }

    /**
     * @brief Retrieves the number of samples ready to read from a channel's double ring buffer.
     * @param channel The channel index to query.
     * @return The number of samples ready to read.
     */
    int getNumReadyDouble (int channel) const noexcept
    {
        return abstractFifoDouble.at (static_cast<size_t> (channel))->getNumReady();
    }

    /**
     * @brief Retrieves the number of channels allocated by the last prepare() call.
     * @return The allocated channel count.
     */
    int getMaxChannels() const noexcept { return maxChannels; }

    /**
     * @brief Pulls up to numSamples samples from a channel's float ring buffer, advancing the read pointer by half the window.
     *
     * @param channel The channel index to pull from.
     * @param destination The buffer to copy samples into; must hold at least numSamples samples.
     * @param numSamples The number of samples requested.
     * @return The number of samples actually copied, which may be less than numSamples if fewer are available.
     */
    int pullFloat (int channel, float* destination, int numSamples) noexcept
    {
        return pull (*abstractFifoFloat.at (static_cast<size_t> (channel)),
                     bufferFloat.at (static_cast<size_t> (channel)),
                     destination,
                     numSamples);
    }

    /**
     * @brief Pulls up to numSamples samples from a channel's double ring buffer, advancing the read pointer by half the window.
     *
     * @param channel The channel index to pull from.
     * @param destination The buffer to copy samples into; must hold at least numSamples samples.
     * @param numSamples The number of samples requested.
     * @return The number of samples actually copied, which may be less than numSamples if fewer are available.
     */
    int pullDouble (int channel, double* destination, int numSamples) noexcept
    {
        return pull (*abstractFifoDouble.at (static_cast<size_t> (channel)),
                     bufferDouble.at (static_cast<size_t> (channel)),
                     destination,
                     numSamples);
    }

private:
    template <typename T, typename SampleType>
    void pushChannel (juce::AbstractFifo& fifo,
                      juce::HeapBlock<T>& buffer,
                      const SampleType* src,
                      int numSamples) noexcept
    {
        int start1 { 0 }, size1 { 0 }, start2 { 0 }, size2 { 0 };
        fifo.prepareToWrite (numSamples, start1, size1, start2, size2);

        for (int i { 0 }; i < size1; ++i)
            buffer.getData()[start1 + i] = static_cast<T> (src[i]);

        for (int i { 0 }; i < size2; ++i)
            buffer.getData()[start2 + i] = static_cast<T> (src[size1 + i]);

        fifo.finishedWrite (size1 + size2);
    }

    template <typename T>
    int pull (juce::AbstractFifo& fifo, juce::HeapBlock<T>& buffer, T* destination, int numSamples) noexcept
    {
        const int available { fifo.getNumReady() };
        const int toRead { juce::jmin (numSamples, available) };

        int start1 { 0 }, size1 { 0 }, start2 { 0 }, size2 { 0 };
        fifo.prepareToRead (toRead, start1, size1, start2, size2);

        if (size1 > 0)
            std::memcpy (destination, buffer.getData() + start1, static_cast<size_t> (size1) * sizeof (T));

        if (size2 > 0)
            std::memcpy (destination + size1, buffer.getData() + start2, static_cast<size_t> (size2) * sizeof (T));

        const int overlap { numSamples / 2 };
        fifo.finishedRead (juce::jmax (0, toRead - overlap));

        return toRead;
    }

    int maxChannels { 1 };
    jam::Owner<juce::AbstractFifo> abstractFifoFloat;
    jam::Owner<juce::AbstractFifo> abstractFifoDouble;
    std::vector<juce::HeapBlock<float>> bufferFloat;
    std::vector<juce::HeapBlock<double>> bufferDouble;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumFIFO)
};

} // namespace jam

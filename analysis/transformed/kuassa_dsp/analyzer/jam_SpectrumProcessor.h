/**
 * @file jam_SpectrumProcessor.h
 * @brief Background-thread FFT spectrum analyzer fed by SpectrumFIFO from the audio thread.
 */

#pragma once

namespace jam
{

/**
 * @class SpectrumProcessor
 * @brief Background-thread FFT spectrum analyzer fed by SpectrumFIFO from the audio thread.
 *
 * Audio-thread calls to process() push samples into an internal
 * SpectrumFIFO. A dedicated low-priority juce::Thread pulls fixed-size,
 * windowed blocks, performs a forward FFT, applies exponential smoothing
 * across frames, and converts the smoothed magnitudes to decibels for
 * lock-free readout via getSmoothedBins().
 */
class SpectrumProcessor : public juce::Thread
{
public:
    /**
     * @brief Constructs a SpectrumProcessor with its background analysis thread named "SpectrumProcessor".
     */
    SpectrumProcessor()
        : juce::Thread ("SpectrumProcessor")
    {
    }

    /**
     * @brief Destructor for the SpectrumProcessor class.
     *
     * Stops the background analysis thread, waiting up to 2000ms.
     */
    ~SpectrumProcessor() override
    {
        stopThread (2000);
    }

    //==============================================================================
    /**
     * @brief Allocates FFT and analysis buffers for the given process spec and (re)starts the analysis thread.
     *
     * Stops any running thread first, computes the FFT size from the current
     * fftOrder, allocates the FFT plan, window table, per-channel smoothing
     * and output buffers, builds a Hann window, prepares the internal
     * SpectrumFIFO, and starts the analysis thread at low priority.
     *
     * @param spec The process spec supplying sample rate and channel count.
     */
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        if (isThreadRunning())
            stopThread (2000);

        sampleRate = spec.sampleRate;
        fftSize = 1 << fftOrder;
        numChannels = juce::jmax (1, static_cast<int> (spec.numChannels));
        const int numBins { fftSize / 2 };

        fft = std::make_unique<juce::dsp::FFT> (fftOrder);

        fftBuffer.allocate (fftSize * 2, true);
        windowTable.allocate (fftSize, true);

        smoothedBins.resize (static_cast<size_t> (numChannels));
        outputDB.resize (static_cast<size_t> (numChannels));

        for (int ch { 0 }; ch < numChannels; ++ch)
        {
            smoothedBins.at (static_cast<size_t> (ch)).allocate (numBins, true);
            outputDB.at (static_cast<size_t> (ch)).allocate (numBins, true);
        }

        pullScratchDouble.allocate (fftSize, true);
        pullScratchFloat.allocate (fftSize, true);

        const float pi { juce::MathConstants<float>::pi };
        float* const win { windowTable.getData() };

        for (int i { 0 }; i < fftSize; ++i)
            win[i] = 0.5f * (1.0f - std::cos (2.0f * pi * static_cast<float> (i)
                                                / static_cast<float> (fftSize - 1)));

        fifo.prepare (fftSize, numChannels);

        startThread (juce::Thread::Priority::low);
    }

    /**
     * @brief Pushes an audio block's samples into the internal SpectrumFIFO. Audio-thread safe.
     * @tparam SampleType The sample type of the block (float or double).
     * @param block The audio block whose channels are pushed into the FIFO.
     */
    template <typename SampleType>
    void process (const juce::dsp::AudioBlock<SampleType>& block) noexcept
    {
        fifo.process (block);
    }

    /**
     * @brief Resets the internal SpectrumFIFO's ring buffers to empty.
     */
    void reset() noexcept
    {
        fifo.reset();
    }

    /**
     * @brief Stops the background analysis thread, waiting up to 2000ms.
     */
    void releaseResources()
    {
        stopThread (2000);
    }

    //==============================================================================
    /**
     * @brief Sets the exponential smoothing factor applied to magnitude values between frames.
     * @param factor The smoothing factor, clamped to [0.0, 0.9999].
     */
    void setSmoothingFactor (float factor) noexcept
    {
        smoothingFactor = juce::jlimit (0.0f, 0.9999f, factor);
    }

    /**
     * @brief Sets the FFT order used on the next prepare() call.
     * @param newOrder The FFT order (FFT size is `1 << newOrder`).
     */
    void setFFTOrder (int newOrder)
    {
        fftOrder = newOrder;
    }

    /**
     * @brief Sets the channel summing mode used by getSmoothedBins().
     * @param mode The channel processing mode.
     */
    void setChannels (dsp::Channel::processing mode) noexcept
    {
        channelMode = mode;
    }

    //==============================================================================
    /**
     * @brief Copies the smoothed, decibel-converted magnitude bins into destination. Lock-free-safe readout.
     *
     * Packs all channels into destination as [ch0_bin0, ch0_bin1, ..., ch1_bin0, ...]
     * when channelMode is not dsp::Channel::mid, in which case numBins must
     * equal (fftSize / 2) * numChannels. When channelMode is
     * dsp::Channel::mid, averages all channels into a single set of bins, in
     * which case numBins must equal fftSize / 2.
     *
     * @param destination The buffer to copy decibel magnitude bins into.
     * @param numBins The number of bins destination can hold.
     */
    void getSmoothedBins (float* destination, int numBins) const noexcept
    {
        const int binsPerChannel { fftSize / 2 };
        const int clampedChannels { juce::jmin (numChannels, static_cast<int> (outputDB.size())) };
        juce::CriticalSection::ScopedLockType lock (outputLock);

        if (channelMode == dsp::Channel::mid)
        {
            const int toCopy { juce::jmin (binsPerChannel, numBins) };
            juce::FloatVectorOperations::clear (destination, toCopy);

            for (int ch { 0 }; ch < clampedChannels; ++ch)
                juce::FloatVectorOperations::add (destination,
                                                  outputDB.at (static_cast<size_t> (ch)).getData(),
                                                  toCopy);

            juce::FloatVectorOperations::multiply (destination,
                                                   1.0f / static_cast<float> (clampedChannels),
                                                   toCopy);
            return;
        }

        for (int ch { 0 }; ch < clampedChannels; ++ch)
        {
            const int offset { ch * binsPerChannel };
            const int toCopy { juce::jmin (binsPerChannel, numBins - offset) };

            if (toCopy <= 0)
                break;

            juce::FloatVectorOperations::copy (destination + offset,
                                               outputDB.at (static_cast<size_t> (ch)).getData(),
                                               toCopy);
        }
    }

    /**
     * @brief Retrieves the number of frequency bins produced per channel.
     * @return The bin count (`fftSize / 2`).
     */
    int getNumBins() const noexcept { return fftSize / 2; }

    //==============================================================================
    /**
     * @brief Background thread entry point: pulls ready windows from the FIFO, performs FFT analysis, and updates the smoothed decibel output.
     */
    void run() override
    {
        while (not threadShouldExit())
        {
            wait (10);

            if (fft == nullptr)
                continue;

            const int numBins { fftSize / 2 };
            const float normFactor { 1.0f / static_cast<float> (fftSize) };

            for (int ch { 0 }; ch < numChannels; ++ch)
            {
                bool processed { false };

                if (fifo.getNumReadyDouble (ch) >= fftSize)
                {
                    fifo.pullDouble (ch, pullScratchDouble.getData(), fftSize);

                    float* const fftData { fftBuffer.getData() };

                    for (int i { 0 }; i < fftSize; ++i)
                        fftData[i] = static_cast<float> (pullScratchDouble.getData()[i]);

                    processed = true;
                }
                else if (fifo.getNumReadyFloat (ch) >= fftSize)
                {
                    fifo.pullFloat (ch, pullScratchFloat.getData(), fftSize);

                    float* const fftData { fftBuffer.getData() };

                    for (int i { 0 }; i < fftSize; ++i)
                        fftData[i] = pullScratchFloat.getData()[i];

                    processed = true;
                }

                if (processed)
                {
                    float* const fftData { fftBuffer.getData() };
                    const float* const window { windowTable.getData() };
                    float* const smoothed { smoothedBins.at (static_cast<size_t> (ch)).getData() };

                    for (int i { 0 }; i < fftSize; ++i)
                        fftData[i] *= window[i];

                    juce::FloatVectorOperations::clear (fftData + fftSize, fftSize);

                    fft->performFrequencyOnlyForwardTransform (fftData);

                    for (int i { 0 }; i < numBins; ++i)
                    {
                        const float newMag { fftData[i] * normFactor };
                        smoothed[i] = smoothed[i] * smoothingFactor + newMag * (1.0f - smoothingFactor);
                    }

                    {
                        juce::CriticalSection::ScopedLockType lock (outputLock);
                        float* const out { outputDB.at (static_cast<size_t> (ch)).getData() };

                        for (int i { 0 }; i < numBins; ++i)
                            out[i] = juce::Decibels::gainToDecibels (smoothed[i], -100.0f);
                    }
                }
                else
                {
                    float* const smoothed { smoothedBins.at (static_cast<size_t> (ch)).getData() };

                    for (int i { 0 }; i < numBins; ++i)
                        smoothed[i] *= smoothingFactor;

                    {
                        juce::CriticalSection::ScopedLockType lock (outputLock);
                        float* const out { outputDB.at (static_cast<size_t> (ch)).getData() };

                        for (int i { 0 }; i < numBins; ++i)
                            out[i] = juce::Decibels::gainToDecibels (smoothed[i], -100.0f);
                    }
                }
            }
        }
    }

private:
    //==============================================================================
    SpectrumFIFO fifo;
    std::unique_ptr<juce::dsp::FFT> fft;
    int fftSize { 4096 };
    int fftOrder { 12 };
    int numChannels { 1 };
    double sampleRate { 48000.0 };

    juce::HeapBlock<float> fftBuffer;
    juce::HeapBlock<float> windowTable;
    std::vector<juce::HeapBlock<float>> smoothedBins { 1 };
    std::vector<juce::HeapBlock<float>> outputDB { 1 };
    juce::HeapBlock<double> pullScratchDouble;
    juce::HeapBlock<float> pullScratchFloat;

    juce::CriticalSection outputLock;

    float smoothingFactor { 0.8f };
    dsp::Channel::processing channelMode { dsp::Channel::stereo };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumProcessor)
};

} // namespace jam

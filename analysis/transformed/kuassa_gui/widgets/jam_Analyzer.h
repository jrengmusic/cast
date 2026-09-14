/**
 * @file jam_Analyzer.h
 * @brief Real-time spectrum analyzer — polygon or banded-bar rendering with peak hold/decay.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Frequency-to-screen-x mapping used when distributing bars within a band. */
enum class BarSpacing
{
    logarithmic,
    linear
};

/**
 * @class Analyzer
 * @brief Polls a shared spectrum data map on a 60 Hz timer and renders it either
 * as a smooth Catmull-Rom polygon or as per-band bars with peak-hold indicators.
 *
 * Spectrum bins are read from an externally-owned nested data map, keyed by
 * component type then parameter, bound via bindToProcessor(). Frequency bands are divided
 * into a fixed set of ranges (bandRanges), each split into barsPerBand bars.
 */
class Analyzer
    : public juce::Component
    , private juce::Timer
{
public:
    /** @brief Colour identifiers for `jam::Analyzer`. */
    enum ColourIds
    {
        spectrumColourId     = map::ColourId::analyzerSpectrumColourId, ///< Polygon-mode stroke colour.
        peakColourId         = map::ColourId::analyzerPeakColourId, ///< Peak-hold indicator colour (bar mode).
        spectrumFillColourId = map::ColourId::analyzerSpectrumFillColourId, ///< Fill colour, both polygon and bar mode.
    };

    Analyzer() = default;

    ~Analyzer() noexcept override { stopTimer(); }

    //==============================================================================
    /**
     * @brief Binds this analyzer to a shared spectrum data map and starts polling
     * at `jam::Component::refreshRateHz`.
     * @param getters      Nested map keyed by component type then parameter; the Registry
     *                     passes its own key as @p type. Also queried directly with the
     *                     top-level keys `Id::sampleRate` and `Id::numChannels`.
     * @param type         Key selecting the type bucket within @p getters, providing
     *                     the spectrum bins entry at @p parameterID.
     * @param parameterID  Key identifying this analyzer's spectrum entry within its type bucket.
     */
    void bindToProcessor (jam::HashMap<juce::Identifier, jam::Function::Map<juce::Identifier, void>>& getters, const juce::Identifier& type, const juce::Identifier& parameterID)
    {
        auto& analyzerMap { getters.at (type) };

        spectrumSource = [&analyzerMap, parameterID] (float* output, int count)
        {
            analyzerMap.get (parameterID, output, count);
        };

        double sampleRate { 0.0 };
        getters.at (Id::sampleRate).get (Id::sampleRate, sampleRate);
        setSampleRate (sampleRate);

        int channels { 1 };
        getters.at (Id::numChannels).get (Id::numChannels, channels);
        setNumChannels (channels);

        startTimerHz (jam::Component::refreshRateHz);
    }

    //==============================================================================
    /** Reallocates spectrum storage for the new size and recomputes bin/band geometry. */
    void resized() override
    {
        const int w { getWidth() };
        const int h { getHeight() };

        width = static_cast<float> (w);
        height = static_cast<float> (h);

        if (numBins > 0)
        {
            spectrumBins.allocate (numBins * numChannels, true);
        }

        calc();
    }

    /** Recomputes the per-bin frequency resolution and rebuilds bar band edges. */
    void calc() noexcept
    {
        binHz = static_cast<float> (sampleRate) / static_cast<float> (fftSize);
        rebuildBarEdges();
    }

    //==============================================================================
    /**
     * @brief Sets the rendering mode.
     * @param newMode  Polygon or bar rendering mode.
     */
    void setMode (map::AnalyzerMode::value newMode) noexcept { mode = newMode; }

    /**
     * @brief Sets the rendering mode and total bar count.
     * @param newMode    Polygon or bar rendering mode.
     * @param totalBars  Total number of bars across all bands.
     */
    void setMode (map::AnalyzerMode::value newMode, int totalBars) noexcept
    {
        mode = newMode;
        setNumBars (totalBars);
    }

    /**
     * @brief Sets the total number of bars, distributed evenly across bands.
     * @param totalBars  Total number of bars across all bands.
     */
    void setNumBars (int totalBars) noexcept
    {
        const int numBands { static_cast<int> (bandRanges.size()) };
        barsPerBand = (numBands > 0) ? totalBars / numBands : totalBars;
        rebuildBarEdges();
    }

    /** @return Total number of bars across all bands. */
    int getNumBars() const noexcept { return static_cast<int> (barEdges.size()); }

    /**
     * @brief Sets the displayed decibel range.
     * @param minDb  Minimum displayed level, in dB.
     * @param maxDb  Maximum displayed level, in dB.
     */
    void setDecibelRange (float minDb, float maxDb) noexcept
    {
        minDecibels = minDb;
        maxDecibels = maxDb;
    }

    /**
     * @brief Sets the displayed frequency range.
     * @param minFreq  Minimum displayed frequency, in Hz.
     * @param maxFreq  Maximum displayed frequency, in Hz.
     */
    void setFrequencyRange (float minFreq, float maxFreq) noexcept
    {
        minFrequency = minFreq;
        maxFrequency = maxFreq;
    }

    /**
     * @brief Sets the polygon mode stroke width.
     * @param newWidth  Stroke width, in pixels.
     */
    void setStrokeWidth (float newWidth) noexcept { strokeWidth = newWidth; }

    /**
     * @brief Sets the bar-mode frequency spacing and rebuilds band edges.
     * @param spacing  Logarithmic or linear bar spacing.
     */
    void setBarSpacing (BarSpacing spacing) noexcept
    {
        barSpacing = spacing;
        rebuildBarEdges();
    }

    /**
     * @brief Sets the sample rate used to derive per-bin frequency resolution.
     * @param sr  Sample rate, in Hz.
     */
    void setSampleRate (double sr) noexcept
    {
        sampleRate = sr;
        binHz = static_cast<float> (sampleRate) / static_cast<float> (fftSize);
    }

    /**
     * @brief Sets the FFT size backing the spectrum bins, reallocating bin storage.
     * @param size  FFT size; bin count is `size / 2`.
     */
    void setFFTSize (int size) noexcept
    {
        fftSize = size;
        numBins = fftSize / 2;
        binHz = static_cast<float> (sampleRate) / static_cast<float> (fftSize);
        spectrumBins.allocate (numBins * numChannels, true);
        resizePeakArrays();
    }

    /**
     * @brief Sets the number of channels rendered, reallocating per-channel storage.
     * @param channels  Channel count; clamped to at least 1.
     */
    void setNumChannels (int channels) noexcept
    {
        numChannels = juce::jmax (1, channels);
        spectrumBins.allocate (numBins * numChannels, true);

        currentPaths.resize (numChannels);

        resizePeakArrays();
    }

    /**
     * @brief Sets how many frames a peak is held before decaying.
     * @param frames  Hold duration, in timer frames.
     */
    void setPeakHoldFrames (int frames) noexcept { peakHoldFrames = frames; }

    /**
     * @brief Sets the peak decay smoothing factor.
     * @param rate  Decay rate, in the range [0, 1] (higher = slower decay).
     */
    void setPeakDecayRate (float rate) noexcept { peakDecayRate = rate; }

    //==============================================================================
    /** Repaints when the component's enabled state changes. */
    void enablementChanged() override { repaint(); }

    /** Draws the current polygon or bar/peak paths for each channel, when enabled. */
    void paint (juce::Graphics& g) override
    {
        if (isEnabled())
        {
            const juce::Colour spectrumColour { findColour (spectrumColourId) };
            const juce::Colour peakColour { findColour (peakColourId) };
            const juce::Colour fillColour { findColour (spectrumFillColourId) };

            for (int ch { 0 }; ch < numChannels; ++ch)
            {
                const auto sz { static_cast<size_t> (ch) };

                if (mode == map::AnalyzerMode::polygon)
                {
                    if (not currentPaths.fillPaths.at (sz).isEmpty())
                    {
                        g.setColour (fillColour);
                        g.fillPath (currentPaths.fillPaths.at (sz));
                    }

                    if (not currentPaths.strokePlots.at (sz).isEmpty())
                    {
                        g.setColour (spectrumColour.withAlpha (strokeAlpha));
                        g.strokePath (currentPaths.strokePlots.at (sz), juce::PathStrokeType (strokeWidth));
                    }
                }
                else
                {
                    if (not currentPaths.barPaths.at (sz).isEmpty())
                    {
                        g.setColour (fillColour);
                        g.fillPath (currentPaths.barPaths.at (sz));
                    }

                    if (not currentPaths.peakPaths.at (sz).isEmpty())
                    {
                        g.setColour (peakColour);
                        g.fillPath (currentPaths.peakPaths.at (sz));
                    }
                }
            }
        }
    }

    //==============================================================================
    // Private method implementations live in jam_AnalyzerImpl.h, included at the
    // bottom of this file.
private:
    /** @brief Reallocates peakValues/peakHoldCounters for numBins * numChannels, resetting to minDecibels/0. */
    void resizePeakArrays() noexcept;
    /** @brief Advances the peak-hold/decay state for one channel's bins. */
    void updatePeakState (int channel, const float* bins) noexcept;
    /** @brief Builds the polygon stroke and fill paths for one channel. */
    void buildPolygonPaths (int channel) noexcept;
    /** @brief Builds the bar and peak-line paths for one channel. */
    void buildBarsPaths (int channel) noexcept;
    /** @return One screen-space point per pixel column, log-frequency-mapped and dB-interpolated from bins. */
    std::vector<juce::Point<float>> mapToScreen (const float* bins) const noexcept;
    /** @return input with leading/trailing floor-level (silent) runs trimmed to one boundary point each. */
    std::vector<juce::Point<float>>
    trimFloor (const std::vector<juce::Point<float>>& input, float epsilon = 1.0f) const noexcept;
    /** @brief Builds a smooth Catmull-Rom spline through points into path. */
    void buildCatmullRomPath (const std::vector<juce::Point<float>>& points, juce::Path& path) noexcept;
    /** @brief Builds per-segment fill rectangles from points down to the floor into path. */
    void buildFillStrips (const std::vector<juce::Point<float>>& points, juce::Path& path) noexcept;
    /** @brief Averages bins into one dB value per bar edge, into values. */
    void accumulateBars (const float* bins, std::vector<float>& values) noexcept;
    /** @brief Builds one filled rectangle per bar from values into path. */
    void buildBarRects (const std::vector<float>& values, juce::Path& path) noexcept;
    /** @brief Builds one thin peak-indicator rectangle per bar from values into path. */
    void buildPeakBarLines (const std::vector<float>& values, juce::Path& path) noexcept;
    /** @brief Recomputes barEdges from bandRanges/barsPerBand/barSpacing, and resizes barValues/peakBarValues. */
    void rebuildBarEdges() noexcept;
    /** @return count frequencies log-spaced between fmin and fmax, inclusive. */
    static std::vector<float> makeLogFrequencies (float fmin, float fmax, int count) noexcept;
    /** @return count frequencies linearly spaced between fmin and fmax, inclusive. */
    static std::vector<float> makeLinearFrequencies (float fmin, float fmax, int count) noexcept;

    //==============================================================================
    void timerCallback() override
    {
        // MESSAGE THREAD
        if (spectrumSource != nullptr and numBins > 0)
        {
            spectrumSource (spectrumBins.getData(), numBins * numChannels);

            currentPaths.clearAll (numChannels);

            for (int channel { 0 }; channel < numChannels; ++channel)
            {
                if (mode == map::AnalyzerMode::polygon)
                    buildPolygonPaths (channel);
                else
                    buildBarsPaths (channel);
            }

            repaint();
        }
    }

    //==============================================================================
    std::function<void (float*, int)> spectrumSource;
    jam::Array<float> spectrumBins;
    int numBins { 2048 };
    int numChannels { 1 };

    map::AnalyzerMode::value mode { map::AnalyzerMode::polygon };
    BarSpacing barSpacing { BarSpacing::logarithmic };
    int barsPerBand { 9 };

    struct PathSet
    {
        std::vector<juce::Path> strokePlots;
        std::vector<juce::Path> fillPaths;
        std::vector<juce::Path> barPaths;
        std::vector<juce::Path> peakPaths;

        void resize (int channels)
        {
            const auto sz { static_cast<size_t> (channels) };
            strokePlots.resize (sz);
            fillPaths.resize (sz);
            barPaths.resize (sz);
            peakPaths.resize (sz);
        }

        void clearAll (int channels)
        {
            for (int ch { 0 }; ch < channels; ++ch)
            {
                const auto sz { static_cast<size_t> (ch) };
                strokePlots.at (sz).clear();
                fillPaths.at (sz).clear();
                barPaths.at (sz).clear();
                peakPaths.at (sz).clear();
            }
        }
    };

    PathSet currentPaths;

    jam::Array<float> peakValues;
    jam::Array<int> peakHoldCounters;

    int peakHoldFrames { defaultPeakHoldFrames };
    float peakDecayRate { defaultPeakDecayRate };

    float strokeWidth { 2.0f };
    float strokeAlpha { 1.0f };

    float minFrequency { 20.0f };
    float maxFrequency { 20000.0f };
    float minDecibels { -90.0f };
    float maxDecibels { 0.0f };
    float width { 0.0f };
    float height { 0.0f };
    double sampleRate { 48000.0 };
    int fftSize { 4096 };
    float binHz { static_cast<float> (sampleRate) / static_cast<float> (fftSize) };

    std::vector<std::pair<float, float>> bandRanges {
        { 20.0f,   200.0f   },
        { 200.0f,  2000.0f  },
        { 2000.0f, 20000.0f }
    };

    std::vector<float> barValues;
    std::vector<float> peakBarValues;
    std::vector<std::pair<float, float>> barEdges;

    static constexpr float barGap { 1.0f };
    static constexpr int defaultPeakHoldFrames { 30 };
    static constexpr float defaultPeakDecayRate { 0.5f };
    static constexpr float peakLineHeight { 2.0f };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Analyzer)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */

#include "jam_AnalyzerImpl.h"

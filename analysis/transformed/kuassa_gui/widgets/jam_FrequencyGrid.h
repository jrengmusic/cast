/**
 * @file jam_FrequencyGrid.h
 * @brief Static frequency-axis grid background for spectrum visualizers.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Draws the static frequency-axis grid (major/minor tick lines, band
 *        dividers, amplitude lines and outline) behind a spectrum visualizer.
 *
 * Geometry is rebuilt by calc() into cached juce::Path members and only
 * re-stroked on paint() — layout changes (resize, scale, bar count, font)
 * trigger calc(), colour/appearance changes do not.
 */
class FrequencyGrid : public juce::Component
{
public:
    /** @brief LookAndFeel colour identifiers used by FrequencyGrid. */
    enum ColourIds
    {
        lineColourId = map::ColourId::frequencyGridLineColourId,  ///< Base colour the grid lines are derived from.
    };

    /** @brief Frequency-axis layout mode. */
    enum class Scale
    {
        logarithmic,  ///< Ticks placed per-band using logToNormalise()/normaliseToLog().
        linear,       ///< Ticks placed at evenly-spaced kHz intervals.
        equalBars     ///< Ticks placed at equalBarCount evenly-spaced positions.
    };

    //==============================================================================
    /**
     * @brief Maps a frequency to a normalised [0, 1] position on a logarithmic axis.
     * @param frequency  The frequency to map, in Hz.
     * @param startRange The frequency at normalised position 0.
     * @param endRange   The frequency at normalised position 1.
     * @return The normalised position of @p frequency between @p startRange and @p endRange.
     */
    static float logToNormalise (float frequency, float startRange, float endRange) noexcept
    {
        const float logStart { std::log10 (startRange) };
        const float logEnd { std::log10 (endRange) };
        return (std::log10 (frequency) - logStart) / (logEnd - logStart);
    }

    /**
     * @brief Inverse of logToNormalise() — maps a normalised [0, 1] position back to a frequency.
     * @param normalised The normalised position to map.
     * @param startRange The frequency at normalised position 0.
     * @param endRange   The frequency at normalised position 1.
     * @return The frequency in Hz corresponding to @p normalised.
     */
    static float normaliseToLog (float normalised, float startRange, float endRange) noexcept
    {
        const float logStart { std::log10 (startRange) };
        const float logEnd { std::log10 (endRange) };
        return std::pow (10.0f, normalised * (logEnd - logStart) + logStart);
    }

    //==============================================================================
    /**
     * @brief Constructs a FrequencyGrid, optionally expanding the band ranges outward.
     * @param rangeExpansion Fraction in [0, 1] by which the outermost band edges
     *        are pushed further out before the grid is first calculated. 0
     *        leaves bandRanges unchanged.
     */
    explicit FrequencyGrid (float rangeExpansion = 0.0f)
        : expansion (rangeExpansion)
    {
        setBufferedToImage (true);
        expandRange (expansion);
    }

    ~FrequencyGrid() noexcept override = default;

    /** @brief Recalculates grid geometry to fit the new component bounds. */
    void resized() override { calc(); }

    //==============================================================================
    /**
     * @brief Switches the frequency-axis layout mode and recalculates geometry.
     * @param newMode The Scale mode to switch to.
     */
    void setFrequencyScale (Scale newMode) noexcept
    {
        if (scaleMode != newMode)
        {
            scaleMode = newMode;
            calc();
        }
    }

    /**
     * @brief Sets the number of bars for Scale::equalBars mode and recalculates geometry.
     * @param count The number of equally-spaced bars.
     */
    void setNumBars (int count) noexcept
    {
        if (equalBarCount != count)
        {
            equalBarCount = count;
            calc();
        }
    }

    /**
     * @brief Sets the font used for axis numbering and recalculates geometry.
     * @param newFont The font to use for numbering labels.
     */
    void setNumbersFont (const juce::FontOptions& newFont) noexcept
    {
        numbersFont = newFont;
        calc();
    }

    /**
     * @brief Adjusts the numbering font's kerning and recalculates geometry.
     * @param newValue Kerning value; scaled internally by 0.01.
     */
    void setNumbersFontKerning (float newValue) noexcept
    {
        numbersFont = numbersFont.withKerningFactor (newValue * 0.01f);
        calc();
    }

    /**
     * @brief Sets the font used for the axis title and recalculates geometry.
     * @param newFont The font to use for the title.
     */
    void setTitleFont (const juce::FontOptions& newFont) noexcept
    {
        titleFont = newFont;
        calc();
    }

    /**
     * @brief Adjusts the title font's kerning and recalculates geometry.
     * @param newValue Kerning value; scaled internally by 0.01.
     */
    void setTitleFontKerning (float newValue) noexcept
    {
        titleFont = titleFont.withKerningFactor (newValue * 0.01f);
        calc();
    }

    /**
     * @brief Enables or disables the axis title and recalculates geometry.
     * @param shouldUseTitle True to draw the title.
     */
    void setUsingTitle (bool shouldUseTitle) noexcept
    {
        isUsingTitle = shouldUseTitle;
        calc();
    }

    /** @brief Sets the fill colour painted behind the grid. Alpha 0 skips the fill.
     *  @param newColour The canvas background colour.
     */
    void setCanvasColour (juce::Colour newColour) noexcept { canvasColour = newColour; }

    /**
     * @brief Sets the stroke widths for each grid layer. A width of 0 skips that layer.
     * @param newMajorStroke     Stroke width for major tick lines.
     * @param newMinorStroke     Stroke width for minor tick lines.
     * @param newBandsStroke     Stroke width for band divider lines.
     * @param newAmplitudeStroke Stroke width for amplitude (dB) lines.
     * @param newOutlineStroke   Stroke width for the grid area outline.
     */
    void setGridStrokeWidth (float newMajorStroke,
                             float newMinorStroke,
                             float newBandsStroke,
                             float newAmplitudeStroke,
                             float newOutlineStroke) noexcept
    {
        majorStroke = newMajorStroke;
        minorStroke = newMinorStroke;
        bandsStroke = newBandsStroke;
        amplitudeStroke = newAmplitudeStroke;
        outlineStroke = newOutlineStroke;
    }

    //==============================================================================
    /** @brief Returns the inset, in pixels, between the component bounds and the grid area. */
    int getGridInset() const noexcept { return gridInset; }

    /** @brief Returns the grid's drawable area (component bounds reduced by getGridInset()). */
    juce::Rectangle<int> getArea() const noexcept { return area; }

    /** @brief Returns the current amplitude (dB) range as { min, max }. */
    std::pair<float, float> getDecibelRange() const noexcept { return { amplitudeMin, amplitudeMax }; }

    /** @brief Returns the overall frequency range as { lowest band start, highest band end }. */
    std::pair<float, float> getFrequencyRange() const noexcept
    {
        const auto& [startLo, endLo] { bandRanges.at (0) };
        const auto& [startHi, endHi] { bandRanges.at (bandRanges.size() - 1) };

        return { startLo, endHi };
    }

    /** @brief Returns a copy of the per-band { start, end } frequency ranges. */
    std::vector<std::pair<float, float>> getBandRanges() const noexcept { return bandRanges; }

    //==============================================================================
    /** @brief Rebuilds all grid paths (background, ticks, bands, amplitude lines, outline)
     *  from the current bounds, scale mode and stroke settings.
     */
    void calc() noexcept
    {
        background.clear();
        major.clear();
        minor.clear();
        bands.clear();
        amplitude.clear();
        outline.clear();

        background.addRectangle (getLocalBounds());

        area = getLocalBounds().reduced (gridInset);

        outline.addRectangle (area);

        for (float db { amplitudeMin }; db <= amplitudeMax; db += dBInterval)
        {
            float norm { juce::jmap (db, amplitudeMin, amplitudeMax, 1.0f, 0.0f) };
            int y { area.getY() + static_cast<int> (norm * area.getHeight()) };

            if (db != amplitudeMin and db != amplitudeMax)
            {
                amplitude.startNewSubPath (area.getX(), y);
                amplitude.lineTo (area.getRight(), y);
            }
        }

        if (scaleMode == Scale::equalBars)
            calcEqualBars();
        else if (scaleMode == Scale::linear)
            calcLinear();
        else
            calcLogarithmic();
    }

    //==============================================================================
    /** @brief Strokes the cached grid paths built by calc(), using lineColourId
     *  as the base colour for all layers.
     *  @param g The graphics context to paint into.
     */
    void paint (juce::Graphics& g) override
    {
        if (canvasColour.getAlpha() > 0)
        {
            g.setColour (canvasColour);
            g.fillPath (background);
        }

        const auto lineColour { findColour (lineColourId) };
        const auto minorLine { lineColour.withBrightness (0.3f) };
        const auto majorLine { lineColour.withBrightness (0.5f) };
        const auto bandsLine { lineColour };
        const auto amplitudeLine { lineColour.withBrightness (0.3f) };
        const auto outlineLine { juce::Colour() };

        if (minorStroke > 0.0f)
        {
            g.setColour (minorLine.withMultipliedAlpha (gridAlpha));
            g.strokePath (minor, juce::PathStrokeType (minorStroke));
        }

        if (majorStroke > 0.0f)
        {
            g.setColour (majorLine.withMultipliedAlpha (gridAlpha));
            g.strokePath (major, juce::PathStrokeType (majorStroke));
        }

        if (bandsStroke > 0.0f)
        {
            g.setColour (bandsLine.withMultipliedAlpha (gridAlpha));
            g.strokePath (bands, juce::PathStrokeType (bandsStroke));
        }

        if (amplitudeStroke > 0.0f)
        {
            g.setColour (amplitudeLine.withMultipliedAlpha (gridAlpha));
            g.strokePath (amplitude, juce::PathStrokeType (amplitudeStroke));
        }

        if (outlineStroke > 0.0f)
        {
            g.setColour (outlineLine.withMultipliedAlpha (gridAlpha));
            g.strokePath (outline, juce::PathStrokeType (outlineStroke));
        }
    }

private:
    const float expansion;
    float amplitudeMin { -30.0f };
    float amplitudeMax { 30.0f };
    float dBInterval { 6.0f };
    Scale scaleMode { Scale::logarithmic };
    /** @brief Default bar count for Scale::equalBars mode. */
    static constexpr int defaultEqualBarCount { 27 };
    int equalBarCount { defaultEqualBarCount };

    juce::Path background;
    juce::Path major;
    juce::Path minor;
    juce::Path bands;
    juce::Path amplitude;
    juce::Path outline;

    int gridInset { 0 };
    juce::Rectangle<int> area;

    juce::FontOptions numbersFont { 14.0f };
    juce::FontOptions titleFont { 24.0f };

    juce::Colour canvasColour { };

    /** @brief Default stroke width for major tick lines. */
    static constexpr float defaultMajorStroke { 1.618f };
    float majorStroke { defaultMajorStroke };
    float minorStroke { 1.0f };
    float bandsStroke { 2.0f };
    float amplitudeStroke { 1.0f };
    float outlineStroke { 0.0f };

    /** @brief Default alpha multiplier applied to all grid line colours. */
    static constexpr float defaultGridAlpha { 0.444f };
    float gridAlpha { defaultGridAlpha };

    bool isUsingTitle { false };

    //==============================================================================
    /** @brief Builds major tick lines at equalBarCount evenly-spaced positions across area. */
    void calcEqualBars() noexcept
    {
        const float areaWidth { static_cast<float> (area.getWidth()) };
        const float x0 { static_cast<float> (area.getX()) };
        const float barWidth { areaWidth / static_cast<float> (equalBarCount) };

        for (int i { 1 }; i < equalBarCount; ++i)
        {
            const float x { x0 + static_cast<float> (i) * barWidth };
            major.startNewSubPath (x, area.getY());
            major.lineTo (x, area.getBottom());
        }
    }

    /** @brief Builds major tick lines at each whole-kHz frequency across the overall band range. */
    void calcLinear() noexcept
    {
        const float startFreq { bandRanges.at (0).first };
        const float endFreq { bandRanges.at (bandRanges.size() - 1).second };

        float bandWidth { static_cast<float> (area.getWidth()) };
        float x0 { static_cast<float> (area.getX()) };

        auto addLine = [this, startFreq, endFreq, bandWidth, x0] (float freq)
        {
            float norm { juce::jlimit (0.0f, 1.0f, (freq - startFreq) / (endFreq - startFreq)) };
            float x { x0 + norm * bandWidth };

            if (freq != startFreq and freq != endFreq)
            {
                major.startNewSubPath (x, area.getY());
                major.lineTo (x, area.getBottom());
            }
        };

        int startKHz { static_cast<int> (startFreq / 1000.0f) };
        int endKHz { static_cast<int> (endFreq / 1000.0f) };

        for (int k { startKHz }; k <= endKHz; ++k)
            addLine (k * 1000.0f);
    }

    /** @brief Builds major/minor tick lines per band (log-spaced) and band divider lines between bands. */
    void calcLogarithmic() noexcept
    {
        const int bandCount { static_cast<int> (bandRanges.size()) };
        float bandWidth { static_cast<float> (area.getWidth()) / static_cast<float> (bandCount) };

        for (int i { 0 }; i < bandCount; ++i)
        {
            float x0 { static_cast<float> (area.getX()) + i * bandWidth };

            auto addLine = [this, x0, bandWidth] (juce::Path& path, float start, float end, float freq)
            {
                float norm { logToNormalise (freq, start, end) };

                if (norm >= 0.0f and norm <= 1.0f)
                {
                    float x { x0 + norm * bandWidth };

                    if (freq != start and freq != end)
                    {
                        path.startNewSubPath (x, area.getY());
                        path.lineTo (x, area.getBottom());
                    }
                }
            };

            float start { bandRanges.at (i).first };
            float end { bandRanges.at (i).second };

            for (float f : minorTicks.at (i))
                addLine (minor, start, end, f);

            for (float f : majorTicks.at (i))
                addLine (major, start, end, f);

            if (i > 0)
            {
                float x { static_cast<float> (area.getX()) + i * bandWidth };
                bands.startNewSubPath (x, area.getY());
                bands.lineTo (x, area.getBottom());
            }
        }
    }

    //==============================================================================
    /** @brief Pushes the outermost band edges (bandRanges front/back) further out by fraction e, in [0, 1]. */
    void expandRange (float e) noexcept
    {
        if (e > 0.0f)
        {
            auto& [startLo, endLo] { bandRanges.at (0) };
            auto& [startHi, endHi] { bandRanges.at (bandRanges.size() - 1) };

            if (scaleMode == Scale::logarithmic)
            {
                const float minLo { startLo * 0.5f };
                const float maxHi { endHi * 2.0f };

                const float loBaseNorm { logToNormalise (startLo, minLo, endHi) };
                const float hiBaseNorm { logToNormalise (endHi, startLo, maxHi) };

                const float loNorm { juce::jmap (e, loBaseNorm, 0.0f) };
                const float hiNorm { juce::jmap (e, hiBaseNorm, 1.0f) };

                startLo = normaliseToLog (loNorm, minLo, endHi);
                endHi = normaliseToLog (hiNorm, startLo, maxHi);
            }
            else
            {
                const float range { endHi - startLo };
                const float maxDelta { 0.5f * range };

                const float lowerExpanded { startLo - juce::jmap (e, 0.0f, maxDelta) };
                const float upperExpanded { endHi + juce::jmap (e, 0.0f, maxDelta) };

                startLo = juce::jlimit (0.0f, endHi, lowerExpanded);
                endHi = upperExpanded;
            }
        }
    }

    std::vector<std::pair<float, float>> bandRanges {
        { 20.0f,   200.0f   },
        { 200.0f,  2000.0f  },
        { 2000.0f, 20000.0f }
    };

    std::vector<std::vector<float>> majorTicks {
        { 20.0f, 50.0f, 100.0f, 200.0f },
        { 500.0f, 1000.0f, 2000.0f },
        { 2000.0f, 5000.0f, 10000.0f, 20000.0f }
    };

    const std::vector<std::vector<float>> minorTicks {
        { 30.0f,   40.0f,   60.0f,   70.0f,   80.0f,   90.0f   },
        { 300.0f,  400.0f,  600.0f,  700.0f,  800.0f,  900.0f  },
        { 3000.0f, 4000.0f, 6000.0f, 7000.0f, 8000.0f, 9000.0f }
    };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyGrid)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */

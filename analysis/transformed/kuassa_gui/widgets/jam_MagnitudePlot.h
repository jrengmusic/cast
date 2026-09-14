/**
 * @file jam_MagnitudePlot.h
 * @brief Polled, smoothed magnitude-response curve display.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Draws a smoothed magnitude-response curve, polled at 60Hz from a
 *        Function::Map data source and rendered as a Catmull-Rom spline.
 */
class MagnitudePlot
    : public juce::Component
    , private juce::Timer
{
public:
    /** @brief LookAndFeel colour identifiers used by MagnitudePlot. */
    enum ColourIds
    {
        curveColourId = map::ColourId::magnitudePlotCurveColourId, ///< Magnitude curve stroke colour.
    };

    MagnitudePlot() { setBufferedToImage (true); }
    ~MagnitudePlot() noexcept override { stopTimer(); }

    /**
     * @brief Binds this plot to a magnitude data source and starts polling it at
     * `jam::Component::refreshRateHz`.
     *
     * @param getters     Nested map keyed by component type then parameter; the Registry
     *                    passes its own key as @p type. The entry at @p parameterID within
     *                    the @p type bucket fills the magnitude buffer via
     *                    `(double* output, int count)`.
     * @param type        Key selecting the type bucket within @p getters.
     * @param parameterID Key identifying the magnitude entry within the type bucket.
     */
    void bindToProcessor (jam::HashMap<juce::Identifier, jam::Function::Map<juce::Identifier, void>>& getters, const juce::Identifier& type, const juce::Identifier& parameterID)
    {
        auto& magnitudeMap { getters.at (type) };

        magnitudeSource = [&magnitudeMap, parameterID] (double* output, int count)
        {
            magnitudeMap.get (parameterID, output, count);
        };

        startTimerHz (jam::Component::refreshRateHz);
    }

    //==============================================================================
    /** @brief Reallocates the magnitude buffer when width changes and
     *  caches the current bounds for screen-space mapping.
     */
    void resized() override
    {
        const int w { getWidth() };
        const int h { getHeight() };

        if (w != numPoints)
        {
            numPoints = w;
            magnitudes.allocate (numPoints, true);
        }

        width = static_cast<float> (w);
        height = static_cast<float> (h);
    }

    //==============================================================================
    /** @brief Sets the frequency axis range used to map magnitudes to x positions.
     *  @param minFreq The frequency, in Hz, mapped to x = 0.
     *  @param maxFreq The frequency, in Hz, mapped to x = width.
     */
    void setFrequencyRange (float minFreq, float maxFreq) noexcept
    {
        minFrequency = minFreq;
        maxFrequency = maxFreq;
    }

    /** @brief Sets the decibel range used to map magnitudes to y positions.
     *  @param minDb The decibel value mapped to y = height (bottom).
     *  @param maxDb The decibel value mapped to y = 0 (top).
     */
    void setDecibelRange (float minDb, float maxDb) noexcept
    {
        minDecibels = minDb;
        maxDecibels = maxDb;
    }

    /** @brief Sets the stroke width used to draw the magnitude curve.
     *  @param newWidth The stroke width in pixels.
     */
    void setStrokeWidth (float newWidth) noexcept { strokeWidth = newWidth; }

    //==============================================================================
    /** @brief Repaints when the component's enabled state changes, so paint()
     *  can apply the dimmed alpha for the disabled state.
     */
    void enablementChanged() override { repaint(); }

    /** @brief Strokes the cached magnitude curve, dimmed when disabled.
     *  @param g The graphics context to paint into.
     */
    void paint (juce::Graphics& g) override
    {
        if (not currentPath.isEmpty())
        {
            const auto alpha { isEnabled() ? 1.0f : dimFactor };
            g.setColour (findColour (curveColourId).withMultipliedAlpha (alpha));
            g.strokePath (currentPath, juce::PathStrokeType (strokeWidth));
        }
    }

    //==============================================================================
private:
    //==============================================================================
    void timerCallback() override
    {
        // MESSAGE THREAD
        if (magnitudeSource != nullptr and numPoints > 0)
        {
            magnitudeSource (magnitudes.getData(), static_cast<int> (numPoints));

            auto points { mapToScreen() };
            auto trimmed { trimFloorAndCeil (points) };

            buildCatmullRomPath (trimmed, currentPath);

            repaint();
        }
    }

    //==============================================================================
    /** @brief Builds a smooth Catmull-Rom spline through points into path, replacing its contents. */
    void buildCatmullRomPath (const std::vector<juce::Point<float>>& points, juce::Path& path) noexcept
    {
        path.clear();

        if (points.size() >= 2)
        {
            path.startNewSubPath (points.front());

            if (points.size() == 2)
            {
                path.lineTo (points.at (1));
            }
            else
            {
                const auto n { points.size() };

                for (size_t i { 0 }; i < n - 1; ++i)
                {
                    const auto& p1 { points.at (i) };
                    const auto& p2 { points.at (i + 1) };

                    const auto p0 { i > 0 ? points.at (i - 1) : p1 * 2.0f - p2 };
                    const auto p3 { i + 2 < n ? points.at (i + 2) : p2 * 2.0f - p1 };

                    const auto cp1 { p1 + (p2 - p0) * (1.0f / 6.0f) };
                    const auto cp2 { p2 - (p3 - p1) * (1.0f / 6.0f) };

                    path.cubicTo (cp1, cp2, p2);
                }
            }
        }
    }

    //==============================================================================
    /** @return One screen-space point per pixel column, log-frequency-mapped and dB-limited from magnitudes. */
    std::vector<juce::Point<float>> mapToScreen() const noexcept
    {
        std::vector<juce::Point<float>> points;
        points.reserve (static_cast<size_t> (numPoints));

        const double logMin { std::log10 (static_cast<double> (minFrequency)) };
        const double logMax { std::log10 (static_cast<double> (maxFrequency)) };

        for (int i { 0 }; i < numPoints; ++i)
        {
            const float mag { static_cast<float> (
                magnitudes.getData()[i]) };// raw pointer: caller-guaranteed bounds (numPoints == allocated size)
            const float magdB { juce::Decibels::gainToDecibels (mag) };
            const float dB { juce::jlimit (minDecibels, maxDecibels, magdB) };

            const double logFreq { logMin + (logMax - logMin) * i / (numPoints - 1) };
            const float freq { static_cast<float> (std::pow (10.0, logFreq)) };

            const float x { jam::FrequencyGrid::logToNormalise (freq, minFrequency, maxFrequency) * width };
            const float y { juce::jmap (dB, minDecibels, maxDecibels, height, 0.0f) };

            points.emplace_back (x, y);
        }

        return points;
    }

    /** @return input with leading/trailing floor- or ceiling-level runs trimmed to one boundary point each. */
    std::vector<juce::Point<float>>
    trimFloorAndCeil (const std::vector<juce::Point<float>>& input, float epsilon = 1.0f) const noexcept
    {
        std::vector<juce::Point<float>> out;

        if (not input.empty())
        {
            const float floorY { height };
            const float ceilY { 0.0f };

            auto isEdge = [floorY, ceilY, epsilon] (float y) noexcept
            {
                return std::abs (y - floorY) < epsilon or std::abs (y - ceilY) < epsilon;
            };

            size_t start { 0 };
            while (start < input.size() and isEdge (input.at (start).y))
                ++start;

            size_t end { input.size() };
            while (end > start and isEdge (input.at (end - 1).y))
                --end;

            if (start != end)
            {
                if (start > 0)
                    out.push_back (input.at (start - 1));

                out.insert (out.end(),
                            input.begin() + static_cast<ptrdiff_t> (start),
                            input.begin() + static_cast<ptrdiff_t> (end));

                if (end < input.size())
                    out.push_back (input.at (end));
            }
        }

        return out;
    }

    //==============================================================================
    std::function<void (double*, int)> magnitudeSource;
    jam::Array<double> magnitudes;
    int numPoints { 0 };
    juce::Path currentPath;
    float strokeWidth { 3.0f };
    static constexpr float dimFactor { 0.25f };
    float minFrequency { 20.0f };
    float maxFrequency { 20000.0f };
    float minDecibels { -23.0f };
    float maxDecibels { 23.0f };
    float width { 0.0f };
    float height { 0.0f };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagnitudePlot)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */

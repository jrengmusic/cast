#pragma once

// Private method implementations for jam::Analyzer.
// Included at the end of jam_Analyzer.h — not for direct use.

inline void jam::Analyzer::resizePeakArrays() noexcept
{
    const int totalBins { numBins * numChannels };
    peakValues.allocate (totalBins, false);
    peakHoldCounters.allocate (totalBins, false);

    for (int i { 0 }; i < totalBins; ++i)
    {
        peakValues.at (i) = minDecibels;
        peakHoldCounters.at (i) = 0;
    }
}

inline void jam::Analyzer::updatePeakState (int channel, const float* bins) noexcept
{
    float* peaks { peakValues.getData() + channel * numBins };
    int* counters { peakHoldCounters.getData() + channel * numBins };

    for (int b { 0 }; b < numBins; ++b)
    {
        const float live { bins[b] };

        if (live > peaks[b])
        {
            peaks[b] = live;
            counters[b] = peakHoldFrames;
        }
        else if (counters[b] > 0)
        {
            --counters[b];
        }
        else
        {
            float decayed { peaks[b] * peakDecayRate + live * (1.0f - peakDecayRate) };

            if (decayed < minDecibels)
                decayed = minDecibels;

            peaks[b] = decayed;
        }
    }
}

inline void jam::Analyzer::buildPolygonPaths (int channel) noexcept
{
    const auto sz { static_cast<size_t> (channel) };
    const float* channelBins { spectrumBins.getData() + channel * numBins };

    updatePeakState (channel, channelBins);

    auto points { mapToScreen (channelBins) };
    auto trimmed { trimFloor (points) };
    buildCatmullRomPath (trimmed, currentPaths.strokePlots.at (sz));
    buildFillStrips (trimmed, currentPaths.fillPaths.at (sz));
}

inline void jam::Analyzer::buildBarsPaths (int channel) noexcept
{
    const auto sz { static_cast<size_t> (channel) };
    const float* channelBins { spectrumBins.getData() + channel * numBins };

    updatePeakState (channel, channelBins);

    accumulateBars (channelBins, barValues);
    buildBarRects (barValues, currentPaths.barPaths.at (sz));

    accumulateBars (peakValues.getData() + channel * numBins, peakBarValues);
    buildPeakBarLines (peakBarValues, currentPaths.peakPaths.at (sz));
}

inline std::vector<juce::Point<float>>
jam::Analyzer::mapToScreen (const float* bins) const noexcept
{
    std::vector<juce::Point<float>> points;

    if (width > 0.0f and binHz > 0.0f)
    {
        const int numPoints { static_cast<int> (width) };
        points.reserve (static_cast<size_t> (numPoints));

        for (int i { 0 }; i < numPoints; ++i)
        {
            const float norm { static_cast<float> (i) / static_cast<float> (numPoints - 1) };
            const float freq { FrequencyGrid::normaliseToLog (norm, minFrequency, maxFrequency) };
            const float bin { freq / binHz };
            const int i0 { static_cast<int> (bin) };
            const int i1 { juce::jmin (i0 + 1, numBins - 1) };
            const float t { bin - static_cast<float> (i0) };

            const float db0 { i0 >= 0 and i0 < numBins ? bins[i0] : minDecibels };
            const float db1 { i1 >= 0 and i1 < numBins ? bins[i1] : minDecibels };
            const float dB { juce::jlimit (minDecibels, maxDecibels, db0 + t * (db1 - db0)) };

            const float x { FrequencyGrid::logToNormalise (freq, minFrequency, maxFrequency) * width };
            const float y { juce::jmap (dB, minDecibels, maxDecibels, height, 0.0f) };

            points.emplace_back (x, y);
        }
    }

    return points;
}

inline std::vector<juce::Point<float>>
jam::Analyzer::trimFloor (const std::vector<juce::Point<float>>& input, float epsilon) const noexcept
{
    std::vector<juce::Point<float>> out;

    if (not input.empty())
    {
        const float floorY { height };

        auto isFloor = [floorY, epsilon] (float y) noexcept
        {
            return std::abs (y - floorY) < epsilon;
        };

        size_t start { 0 };
        while (start < input.size() and isFloor (input.at (start).y))
            ++start;

        size_t end { input.size() };
        while (end > start and isFloor (input.at (end - 1).y))
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

inline void
jam::Analyzer::buildCatmullRomPath (const std::vector<juce::Point<float>>& points, juce::Path& path) noexcept
{
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

inline void
jam::Analyzer::buildFillStrips (const std::vector<juce::Point<float>>& points, juce::Path& path) noexcept
{
    if (points.size() >= 2)
    {
        for (size_t i { 0 }; i < points.size() - 1; ++i)
        {
            const auto& p0 { points.at (i) };
            const auto& p1 { points.at (i + 1) };

            path.addRectangle (p0.x, p0.y, p1.x - p0.x, height - p0.y);
        }
    }
}

inline void
jam::Analyzer::accumulateBars (const float* bins, std::vector<float>& values) noexcept
{
    const int totalBars { static_cast<int> (barEdges.size()) };
    values.resize (static_cast<size_t> (totalBars), minDecibels);

    for (int b { 0 }; b < totalBars; ++b)
    {
        const float fLow { barEdges.at (static_cast<size_t> (b)).first };
        const float fHigh { barEdges.at (static_cast<size_t> (b)).second };

        const int binLow { static_cast<int> (fLow / binHz) };
        const int binHigh { static_cast<int> (fHigh / binHz) };

        const int lo { juce::jlimit (0, numBins - 1, binLow) };
        const int hi { juce::jlimit (0, numBins - 1, binHigh) };

        if (lo <= hi)
        {
            float sum { 0.0f };
            int count { 0 };

            for (int k { lo }; k <= hi; ++k)
            {
                sum += bins[k];
                ++count;
            }

            values.at (static_cast<size_t> (b)) = count > 0 ? sum / static_cast<float> (count) : minDecibels;
        }
    }
}

inline void
jam::Analyzer::buildBarRects (const std::vector<float>& values, juce::Path& path) noexcept
{
    const int totalBars { static_cast<int> (barEdges.size()) };
    const float gap { barGap };
    const float barWidth { width / static_cast<float> (totalBars) - gap };

    for (int b { 0 }; b < totalBars; ++b)
    {
        const float xLeft { static_cast<float> (b) * (barWidth + gap) };

        if (barWidth > 0.0f)
        {
            const float dB { juce::jlimit (minDecibels, maxDecibels, values.at (static_cast<size_t> (b))) };
            const float y { juce::jmap (dB, minDecibels, maxDecibels, height, 0.0f) };

            path.addRectangle (xLeft, y, barWidth, height - y);
        }
    }
}

inline void
jam::Analyzer::buildPeakBarLines (const std::vector<float>& values, juce::Path& path) noexcept
{
    const int totalBars { static_cast<int> (barEdges.size()) };
    const float gap { barGap };
    const float barWidth { width / static_cast<float> (totalBars) - gap };

    for (int b { 0 }; b < totalBars; ++b)
    {
        const float xLeft { static_cast<float> (b) * (barWidth + gap) };

        if (barWidth > 0.0f)
        {
            const float dB { juce::jlimit (minDecibels, maxDecibels, values.at (static_cast<size_t> (b))) };
            const float y { juce::jmap (dB, minDecibels, maxDecibels, height, 0.0f) };

            path.addRectangle (xLeft, y, barWidth, peakLineHeight);
        }
    }
}

inline void jam::Analyzer::rebuildBarEdges() noexcept
{
    barEdges.clear();

    for (const auto& [fLow, fHigh] : bandRanges)
    {
        std::vector<float> centers;

        if (barSpacing == BarSpacing::logarithmic)
            centers = makeLogFrequencies (fLow, fHigh, barsPerBand);
        else
            centers = makeLinearFrequencies (fLow, fHigh, barsPerBand);

        for (int i { 0 }; i < barsPerBand; ++i)
        {
            const float edgeLow =
                (i == 0) ? fLow
                         : (centers.at (static_cast<size_t> (i - 1)) + centers.at (static_cast<size_t> (i))) * 0.5f;

            const float edgeHigh =
                (i == barsPerBand - 1)
                    ? fHigh
                    : (centers.at (static_cast<size_t> (i)) + centers.at (static_cast<size_t> (i + 1))) * 0.5f;

            barEdges.emplace_back (edgeLow, edgeHigh);
        }
    }

    barValues.resize (barEdges.size(), minDecibels);
    peakBarValues.resize (barEdges.size(), minDecibels);
}

inline std::vector<float>
jam::Analyzer::makeLogFrequencies (float fmin, float fmax, int count) noexcept
{
    std::vector<float> freqs;
    freqs.reserve (static_cast<size_t> (count));
    const float ratio { std::pow (fmax / fmin, 1.0f / static_cast<float> (count - 1)) };
    float f { fmin };

    for (int i { 0 }; i < count; ++i)
    {
        freqs.push_back (f);
        f *= ratio;
    }

    return freqs;
}

inline std::vector<float>
jam::Analyzer::makeLinearFrequencies (float fmin, float fmax, int count) noexcept
{
    std::vector<float> freqs;
    freqs.reserve (static_cast<size_t> (count));
    const float step { (fmax - fmin) / static_cast<float> (count - 1) };

    for (int i { 0 }; i < count; ++i)
        freqs.push_back (fmin + static_cast<float> (i) * step);

    return freqs;
}

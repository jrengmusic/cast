namespace jam::dsp
{
/*____________________________________________________________________________*/

// Coefficient Generation Strategy:
// Generates stage-specific halfband polyphase coefficients with different tap counts but
// identical Kaiser window β = 12.26 (Quality::Maximum) for -120dB per-stage stopband:
// - Stage 2x: 63 taps (Quality::High) - critical first stage
// - Stage 4x: 31 taps (Quality::Fast) - already band-limited
// - Stage 8x: 31 taps (Quality::Fast) - double band-limited
// Both interpolation and decimation use the same halfband coefficients (Noble identity).
void Oversampler::prepare (const juce::dsp::ProcessSpec& newSpec,
                           Resources& resources)
{
    spec = newSpec;

    // =========================================================
    // Generate stage-specific coefficients
    // All use Quality::Maximum (β=12.26, -120dB stopband)
    // Different tap counts for computational efficiency
    // =========================================================
    resources.coefficients.halfband.stage2x =
        HalfbandPolyphase<Quality::High>::generateCoefficients (FIR::Quality::Maximum);  // 63 taps
    resources.coefficients.halfband.stage4x =
        HalfbandPolyphase<Quality::Fast>::generateCoefficients (FIR::Quality::Maximum);  // 31 taps
    resources.coefficients.halfband.stage8x =
        HalfbandPolyphase<Quality::Fast>::generateCoefficients (FIR::Quality::Maximum);  // 31 taps

    // =========================================================
    // Wire coefficients to filter instances (zero-copy via pointers)
    // Filters are stateless algorithms; coefficients are shared across all channels
    // Decimators share the same halfband coefficients as interpolators (Noble identity)
    // Loop exists for future multi-channel template support
    // =========================================================
    for (juce::uint32 channel = 0; channel < spec.numChannels; ++channel)
    {
        interpolator.stage2x.coefficients = &resources.coefficients.halfband.stage2x;
        interpolator.stage4x.coefficients = &resources.coefficients.halfband.stage4x;
        interpolator.stage8x.coefficients = &resources.coefficients.halfband.stage8x;

        decimator.stage2x.coefficients = &resources.coefficients.halfband.stage2x;
        decimator.stage4x.coefficients = &resources.coefficients.halfband.stage4x;
        decimator.stage8x.coefficients = &resources.coefficients.halfband.stage8x;
    }

    // Wire buffers FIRST before calc() tries to use them
    buffers = &resources.buffers;

    calc();

    // =========================================================
    // Reset state
    // =========================================================
    reset();
}

void Oversampler::reset()
{
    for (auto& channelState : delayLines)
    {
        channelState.reset();
        fractionalDelay.reset (channelState.fractionalDelayState);
    }
}

void Oversampler::setFactor (int newOversamplingFactor)
{
    if (newOversamplingFactor != oversamplingFactor)
    {
        oversamplingFactor = newOversamplingFactor;
        calc();
        reset();
    }
}

void Oversampler::calc()
{
    auto factor { Math::pow2<int> (oversamplingFactor) };

    effectiveSampleRate = spec.sampleRate * factor;
    numUpsamples = spec.maximumBlockSize * factor;

    // Reallocate buffers if we have them
    if (buffers != nullptr)
    {
        buffers->setSize (spec.numChannels,
                          numUpsamples,
                          false,// Don't keep existing content
                          false,// DON'T clear - we're about to write to it anyway
                          true);// Avoid reallocation (real-time safe)
    }

    // Fractional latency compensation: ceil - exact = remainder that allpass must cover
    double uncompensated { getUncompensatedLatency() };
    double fractionalAmount { std::ceil (uncompensated) - uncompensated };
    fractionalDelay.calc (fractionalAmount);
}

double Oversampler::getEffectiveSampleRate() const noexcept
{
    return effectiveSampleRate;
}

int Oversampler::getNumUpsamples() const noexcept
{
    return numUpsamples;
}

double Oversampler::getUncompensatedLatency() const noexcept
{
    double latency { 0.0 };
    int order { 1 };

    if (oversamplingFactor >= 1)
    {
        order *= 2;
        latency += static_cast<double> (HalfbandPolyphase<Quality::High>::latency * 2) / order;
    }

    if (oversamplingFactor >= 2)
    {
        order *= 2;
        latency += static_cast<double> (HalfbandPolyphase<Quality::Fast>::latency * 2) / order;
    }

    if (oversamplingFactor >= 3)
    {
        order *= 2;
        latency += static_cast<double> (HalfbandPolyphase<Quality::Fast>::latency * 2) / order;
    }

    return latency;
}

int Oversampler::getLatencySamples() const noexcept
{
    return static_cast<int> (std::ceil (getUncompensatedLatency()));
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp

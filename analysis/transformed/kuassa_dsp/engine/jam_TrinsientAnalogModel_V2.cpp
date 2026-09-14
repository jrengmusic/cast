/*____________________________________________________________________________*/

namespace jam::dsp::Engine
{

//==============================================================================
TrinsientAnalogModel_V2::TrinsientAnalogModel_V2()
    : sampleRate (44100.0)
    , bypassed (false)
    , hysteresisBypassed (false)
    , crossover (4)  // LR4 default
    , crossoverOrder (4)
    , transientControls (3)      // Create 3 transient controllers
    , hysteresisSaturators (3)   // Create 3 Jiles-Atherton saturators
    , hysteresisStateSpace (3)   // Create 3 state-space processors
    , hysteresisPreisach (3)     // Create 3 Preisach processors
{
    // Initialize crossover frequencies
    crossoverFrequencies[lowMid] = 200.0;
    crossoverFrequencies[midHigh] = 2800.0;

    // Initialize per-band parameters
    for (int band = low; band <= high; ++band)
    {
        // Transient control parameters
        attack[band] = 0.0f;
        attackStrength[band] = 0.5f;
        decay[band] = 0.0f;
        decayStrength[band] = 0.5f;
        inputGain[band] = 0.0f;
        outputGain[band] = 0.0f;
        gainCompensation[band] = 0.0f;
        detectionMode[band] = 1;  // Default to squared
        bandBypassed[band] = false;

        // V2: Set TransientControl output to unity (1.0 / 0dB)
        // We apply output gain separately after hysteresis
        transientControls.at (band)->setOutput (0.0f);  // 0dB = unity gain

        // Hysteresis parameters (NEW in V2)
        hysteresisType[band] = stateSpace;  // Default to state-space (efficient)
        hysteresisDrive[band] = 0.0f;
        hysteresisSaturation[band] = 0.0f;
        hysteresisWidth[band] = 0.5f;
        hysteresisAsymmetry[band] = 0.0f;
        hysteresisEnabled[band] = false;  // Disabled by default

        // Computed values
        inputGainLinear[band] = 1.0;
        outputGainLinear[band] = 1.0;
    }

    reset();
}

TrinsientAnalogModel_V2::~TrinsientAnalogModel_V2()
{
}

//==============================================================================
void TrinsientAnalogModel_V2::reset()
{
    // Reset transient controls
//    for (auto& tc : transientControls)
//        tc->reset();

    // Reset hysteresis processors (NEW in V2)
    for (auto& hs : hysteresisSaturators)
        hs->reset();

    for (auto& hs : hysteresisStateSpace)
        hs->reset();

    for (auto& hp : hysteresisPreisach)
        hp->reset();

    // Reset crossover
    crossover.reset();
}

void TrinsientAnalogModel_V2::setSampleRate (double newSampleRate)
{
    if (newSampleRate != sampleRate && newSampleRate > 0.0)
    {
        sampleRate = newSampleRate;

        // Update transient controls
        for (auto& tc : transientControls)
            tc->setSampleRate (sampleRate);

        // Update hysteresis processors (NEW in V2)
        for (auto& hs : hysteresisSaturators)
            hs->setSampleRate (sampleRate);

        for (auto& hs : hysteresisStateSpace)
            hs->setSampleRate (sampleRate);

        for (auto& hp : hysteresisPreisach)
            hp->setSampleRate (sampleRate);

        // Update crossover
        crossover.setSampleRate (sampleRate);

        calc();
    }
}

//==============================================================================
// Crossover Control

void TrinsientAnalogModel_V2::setCrossoverFrequency (Crossover crossoverIndex, double frequencyHz)
{
    jassert (crossoverIndex >= lowMid && crossoverIndex <= midHigh);

    if (frequencyHz != crossoverFrequencies[crossoverIndex] && frequencyHz > 0.0)
    {
        crossoverFrequencies[crossoverIndex] = frequencyHz;
        calc();
    }
}

void TrinsientAnalogModel_V2::setCrossoverOrder (int order)
{
    if (order != crossoverOrder && order > 0 && order % 2 == 0)
    {
        crossoverOrder = order;
        crossover.setOrder (order);
        calc();
    }
}

//==============================================================================
// Per-Band Transient Control

void TrinsientAnalogModel_V2::setAttack (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != attack[band])
    {
        attack[band] = value;
        transientControls.at (band)->setAttack (value);
    }
}

void TrinsientAnalogModel_V2::setAttackStrength (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != attackStrength[band])
    {
        attackStrength[band] = juce::jlimit (0.0f, 1.0f, value);
        transientControls.at (band)->setAttackStrength (attackStrength[band]);
    }
}

void TrinsientAnalogModel_V2::setDecay (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != decay[band])
    {
        decay[band] = value;
        transientControls.at (band)->setDecay (value);
    }
}

void TrinsientAnalogModel_V2::setDecayStrength (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != decayStrength[band])
    {
        decayStrength[band] = juce::jlimit (0.0f, 1.0f, value);
        transientControls.at (band)->setDecayStrength (decayStrength[band]);
    }
}

void TrinsientAnalogModel_V2::setInputGain (Band band, float gainDB)
{
    jassert (band >= low && band <= high);

    if (gainDB != inputGain[band])
    {
        inputGain[band] = gainDB;
        calc();
    }
}

void TrinsientAnalogModel_V2::setOutputGain (Band band, float gainDB)
{
    jassert (band >= low && band <= high);

    if (gainDB != outputGain[band])
    {
        outputGain[band] = gainDB;

        // V2: Do NOT forward to TransientControl - we apply output gain
        // after hysteresis instead. TC output is kept at unity (1.0).

        calc();
    }
}

void TrinsientAnalogModel_V2::setGainCompensation (Band band, float gainDB)
{
    jassert (band >= low && band <= high);

    if (gainDB != gainCompensation[band])
    {
        gainCompensation[band] = gainDB;
        transientControls.at (band)->setGainCompensation (gainDB);
    }
}

void TrinsientAnalogModel_V2::setDetectionMode (Band band, float mode)
{
    jassert (band >= low && band <= high);

    int modeInt = static_cast<int> (mode);
    if (modeInt != detectionMode[band])
    {
        detectionMode[band] = modeInt;
        transientControls.at (band)->setDetectionMode (mode);
    }
}

//==============================================================================
// Per-Band Hysteresis Control (NEW in V2)

void TrinsientAnalogModel_V2::setHysteresisType (Band band, HysteresisType type)
{
    jassert (band >= low && band <= high);

    if (type != hysteresisType[band])
    {
        hysteresisType[band] = type;

        // Reset the selected hysteresis processor
        switch (type)
        {
            case saturator:
                hysteresisSaturators.at (band)->reset();
                break;
            case stateSpace:
                hysteresisStateSpace.at (band)->reset();
                break;
            case preisach:
                hysteresisPreisach.at (band)->reset();
                break;
        }
    }
}

void TrinsientAnalogModel_V2::setHysteresisDrive (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != hysteresisDrive[band])
    {
        hysteresisDrive[band] = juce::jlimit (0.0f, 1.0f, value);

        // Update all hysteresis types for this band
        hysteresisSaturators.at (band)->setDrive (hysteresisDrive[band]);
        hysteresisStateSpace.at (band)->setDrive (hysteresisDrive[band]);
        hysteresisPreisach.at (band)->setDrive (hysteresisDrive[band]);
    }
}

void TrinsientAnalogModel_V2::setHysteresisSaturation (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != hysteresisSaturation[band])
    {
        hysteresisSaturation[band] = juce::jlimit (0.0f, 1.0f, value);

        // Update all hysteresis types for this band
        hysteresisSaturators.at (band)->setSaturation (hysteresisSaturation[band]);
        hysteresisStateSpace.at (band)->setSaturation (hysteresisSaturation[band]);
        hysteresisPreisach.at (band)->setSaturation (hysteresisSaturation[band]);
    }
}

void TrinsientAnalogModel_V2::setHysteresisWidth (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != hysteresisWidth[band])
    {
        hysteresisWidth[band] = juce::jlimit (0.0f, 1.0f, value);

        // Update all hysteresis types for this band
        hysteresisSaturators.at (band)->setHysteresisWidth (hysteresisWidth[band]);
        hysteresisStateSpace.at (band)->setHysteresisWidth (hysteresisWidth[band]);
        hysteresisPreisach.at (band)->setHysteresisWidth (hysteresisWidth[band]);
    }
}

void TrinsientAnalogModel_V2::setHysteresisAsymmetry (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != hysteresisAsymmetry[band])
    {
        hysteresisAsymmetry[band] = juce::jlimit (-1.0f, 1.0f, value);

        // Update all hysteresis types for this band
        hysteresisSaturators.at (band)->setAsymmetry (hysteresisAsymmetry[band]);
        hysteresisStateSpace.at (band)->setAsymmetry (hysteresisAsymmetry[band]);
        hysteresisPreisach.at (band)->setAsymmetry (hysteresisAsymmetry[band]);
    }
}

void TrinsientAnalogModel_V2::setHysteresisEnabled (Band band, bool enabled)
{
    jassert (band >= low && band <= high);

    if (enabled != hysteresisEnabled[band])
    {
        hysteresisEnabled[band] = enabled;

        // Update bypass state for all hysteresis types
        double bypassValue = enabled ? 0.0 : 1.0;
        hysteresisSaturators.at (band)->setBypassed (bypassValue);
        hysteresisStateSpace.at (band)->setBypassed (bypassValue);
        hysteresisPreisach.at (band)->setBypassed (bypassValue);
    }
}

//==============================================================================
// Bypass Control

void TrinsientAnalogModel_V2::setBypassed (bool shouldBeBypassed)
{
    bypassed = shouldBeBypassed;
}

void TrinsientAnalogModel_V2::setBandBypassed (Band band, bool shouldBeBypassed)
{
    jassert (band >= low && band <= high);
    bandBypassed[band] = shouldBeBypassed;
}

void TrinsientAnalogModel_V2::setCrossoverBypassed (bool shouldBeBypassed)
{
    crossover.setBypassed (shouldBeBypassed);
}

void TrinsientAnalogModel_V2::setHysteresisBypassed (bool shouldBeBypassed)
{
    hysteresisBypassed = shouldBeBypassed;
}

//==============================================================================
// Private methods

void TrinsientAnalogModel_V2::calc()
{
    // Update crossover frequencies
    crossover.setLowCrossover (crossoverFrequencies[lowMid]);
    crossover.setHighCrossover (crossoverFrequencies[midHigh]);

    // Update gain linear values
    for (int band = low; band <= high; ++band)
    {
        inputGainLinear[band] = std::pow (10.0, inputGain[band] / 20.0);
        outputGainLinear[band] = std::pow (10.0, outputGain[band] / 20.0);
    }
}

} // namespace jam::dsp::Engine

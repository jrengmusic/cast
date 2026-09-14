/*____________________________________________________________________________*/

namespace jam::dsp::Engine
{

//==============================================================================
TrinsientAnalogModel_V1::TrinsientAnalogModel_V1()
    : sampleRate (44100.0)
    , bypassed (false)
    , crossover (4)// LR4 default
    , crossoverOrder (4)
    , transientControls (3)// Create 3 transient controllers
{
    // Initialize crossover frequencies
    crossoverFrequencies[lowMid] = 200.0;
    crossoverFrequencies[midHigh] = 2800.0;

    // Initialize per-band parameters
    for (int band = low; band <= high; ++band)
    {
        attack[band] = 0.0f;
        attackStrength[band] = 0.5f;
        decay[band] = 0.0f;
        decayStrength[band] = 0.5f;
        inputGain[band] = 0.0f;
        outputGain[band] = 0.0f;
        gainCompensation[band] = 0.0f;
        detectionMode[band] = 1;// Default to squared
        bandBypassed[band] = false;
        inputGainLinear[band] = 1.0;
    }

    reset();
}

TrinsientAnalogModel_V1::~TrinsientAnalogModel_V1()
{
}

//==============================================================================
void TrinsientAnalogModel_V1::reset()
{
//    for (auto& tc : transientControls)
//        tc->reset();

    crossover.reset();
}

void TrinsientAnalogModel_V1::setSampleRate (double newSampleRate)
{
    if (newSampleRate != sampleRate && newSampleRate > 0.0)
    {
        sampleRate = newSampleRate;

        for (auto& tc : transientControls)
            tc->setSampleRate (sampleRate);

        crossover.setSampleRate (sampleRate);

        calc();
    }
}

//==============================================================================
// Crossover Control

void TrinsientAnalogModel_V1::setCrossoverFrequency (Crossover crossoverIndex, double frequencyHz)
{
    jassert (crossoverIndex >= lowMid && crossoverIndex <= midHigh);

    if (frequencyHz != crossoverFrequencies[crossoverIndex] && frequencyHz > 0.0)
    {
        crossoverFrequencies[crossoverIndex] = frequencyHz;
        calc();
    }
}

void TrinsientAnalogModel_V1::setCrossoverOrder (int order)
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

void TrinsientAnalogModel_V1::setAttack (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != attack[band])
    {
        attack[band] = value;
        transientControls.at (band)->setAttack (value);
    }
}

void TrinsientAnalogModel_V1::setAttackStrength (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != attackStrength[band])
    {
        attackStrength[band] = juce::jlimit (0.0f, 1.0f, value);
        transientControls.at (band)->setAttackStrength (attackStrength[band]);
    }
}

void TrinsientAnalogModel_V1::setDecay (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != decay[band])
    {
        decay[band] = value;
        transientControls.at (band)->setDecay (value);
    }
}

void TrinsientAnalogModel_V1::setDecayStrength (Band band, float value)
{
    jassert (band >= low && band <= high);

    if (value != decayStrength[band])
    {
        decayStrength[band] = juce::jlimit (0.0f, 1.0f, value);
        transientControls.at (band)->setDecayStrength (decayStrength[band]);
    }
}

void TrinsientAnalogModel_V1::setInputGain (Band band, float gainDB)
{
    jassert (band >= low && band <= high);

    if (gainDB != inputGain[band])
    {
        inputGain[band] = gainDB;
        calc();
    }
}

void TrinsientAnalogModel_V1::setOutputGain (Band band, float gainDB)
{
    jassert (band >= low && band <= high);

    if (gainDB != outputGain[band])
    {
        outputGain[band] = gainDB;
        transientControls.at (band)->setOutput (gainDB);
    }
}

void TrinsientAnalogModel_V1::setGainCompensation (Band band, float gainDB)
{
    jassert (band >= low && band <= high);

    if (gainDB != gainCompensation[band])
    {
        gainCompensation[band] = gainDB;
        transientControls.at (band)->setGainCompensation (gainDB);
    }
}

void TrinsientAnalogModel_V1::setDetectionMode (Band band, float mode)
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
// Bypass Control

void TrinsientAnalogModel_V1::setBypassed (bool shouldBeBypassed)
{
    bypassed = shouldBeBypassed;
}

void TrinsientAnalogModel_V1::setBandBypassed (Band band, bool shouldBeBypassed)
{
    jassert (band >= low && band <= high);
    bandBypassed[band] = shouldBeBypassed;
}

void TrinsientAnalogModel_V1::setCrossoverBypassed (bool shouldBeBypassed)
{
    crossover.setBypassed (shouldBeBypassed);
}

//==============================================================================
// Private methods

void TrinsientAnalogModel_V1::calc()
{
    // Update crossover frequencies
    crossover.setLowCrossover (crossoverFrequencies[lowMid]);
    crossover.setHighCrossover (crossoverFrequencies[midHigh]);

    // Update input gain linear values
    for (int band = low; band <= high; ++band)
    {
        inputGainLinear[band] = std::pow (10.0, inputGain[band] / 20.0);
    }
}

}// namespace jam::dsp::Engine

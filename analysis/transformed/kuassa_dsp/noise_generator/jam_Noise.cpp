namespace jam::dsp
{
/*____________________________________________________________________________*/
Noise::Noise (Type newType)
{
    setNoiseType (newType);

    std::srand ((int) std::time (nullptr)); // use current time as seed for random generator
    /** offset noise timer start arbitrarily */
    milliseconds = jam::Math::roundToNearestNth (std::rand() % oneMinuteInMs, interval);
}

Noise::~Noise() {}

void Noise::setNoiseEnabled (bool shouldBeEnabled)
{
    if (shouldBeEnabled)
        startTimer (interval);
    else
        stopTimer();
}

void Noise::timerCallback()
{
    milliseconds += interval;
    milliseconds %= oneMinuteInMs;

    if (noise != nullptr)
        isNoiseStarting = milliseconds >= 0 and milliseconds < (noiseDuration + interval);
    else
        isNoiseStarting = false;

//    log();
}

void Noise::setNoiseType (Type newType)
{
    switch (newType)
    {
        case Type::white:
            noise = std::make_unique<White>();
            break;

        case Type::pink:
            noise = std::make_unique<Pink>();
            break;

        case Type::brown:
            noise = std::make_unique<Brown>();
            break;
    }
}

void Noise::setNoiseDuration (int newValueMiliseconds)
{
    noiseDuration = newValueMiliseconds;
}

template <typename SampleType>
void Noise::process (SampleType& sample)
{
    if (isNoiseStarting)
        sample = noise->generate() * amp (gain);
}

template <typename SampleType>
void Noise::process (int channel, SampleType& sample)
{
    process (sample);
}

//==============================================================================
/** @cond */
/** explicit instantiation for templatize functions */
template void Noise::process<float> (float& sample);
template void Noise::process<double> (double& sample);
template void Noise::process<float> (int channel, float& sample);
template void Noise::process<double> (int channel, double& sample);
/** @endcond */

/*____________________________________________________________________________*/
/** ------------------------------ PINK NOISE ------------------------------ */
Noise::Pink::Pink (int numRows)
{
    pinkIndex = 0;
    // mask the index so it does not spill outside of the pinkRows vector range
    pinkIndexMask = (1 << numRows) - 1;
    // initialize normalization variable
    pinkNorm = 1.0 / (numRows + 1);
    // in testing, I found it was better to initialize the rows with noise
    // this avoids a climb up to some max value during the first run through the rows
    for (int i = 0; i < numRows; i++)
        pinkRows.push_back (noiseSrc.nextDouble());
    pinkRunSum = noiseSrc.nextDouble();
}

// generates pink noise one sample at a time
double Noise::Pink::generate()
{
    double newRandom;
    double sum;

    // increment and mask index
    pinkIndex = (pinkIndex + 1) & pinkIndexMask;

    // ensure pink index is not zero, if it is, do not update any of the random vals
    if (pinkIndex != 0)
    {
        // determine the number of trailing zeros in pinkIndex
        int numZeros = 0;
        int n = pinkIndex;
        while ((n & 1) == 0)
        {
            // bit shift until you run out of trailing zeros
            n = n >> 1;
            numZeros++;
        }
        // McCARTNEY-VOSS ALGORITHM
        // subtract previous value from running sum
        pinkRunSum -= pinkRows[numZeros];
        // generate a new random number
        newRandom = noiseSrc.nextDouble();
        // add the new random number
        pinkRunSum += newRandom;
        // replace the row value at index numZeros with the new random value
        pinkRows[numZeros] = newRandom;
    }

    // add extra white noise value
    sum = pinkRunSum + noiseSrc.nextDouble();

    // scale and return value
    return (sum * pinkNorm);
}

// Changes the number of noise generating rows
// Note that this overrides the initialization found in the constructor
// AS WELL AS the pinkRows vector. Therefore, it is advised that this
// function only be called on initialization
void Noise::Pink::setRows (int newRows)
{
    // reset pinkIndex
    pinkIndex = 0;
    pinkIndexMask = (1 << newRows) - 1;
    pinkNorm = 1.0 / (newRows + 1);
    // clear the pinkRows vector
    pinkRows.clear();
    // reinitialize the pinkRows vector
    for (int i = 0; i < newRows; i++)
        pinkRows.push_back (noiseSrc.nextDouble());
    pinkRunSum = noiseSrc.nextDouble();
}
//==============================================================================
/** ------------------------------ BROWN NOISE ------------------------------ */

Noise::Brown::Brown (int bL)
{
    bLength = bL;
    // intiaize first sample with white noise
    fillBuffer (noiseSrc.nextDouble());
    itB = nBn.begin();
}

// input is the first sample, or seed sample
void Noise::Brown::fillBuffer (double input)
{
    // clear contents
    nB.clear();
    nBn.clear();
    // add first sample to buffer
    nB.push_back (input);

    // populate brown noise buffer
    for (int i = 0; i < bLength; i++)
    {
        nB.push_back (a * nB.back() + 2 * (noiseSrc.nextDouble()) - 1); // leaky integration
    }

    // NORMALIZE and scale by 0.9 to avoid clipping
    // max/min vals for normalization
    maxB = *std::max_element (nB.begin(), nB.end());
    minB = *std::min_element (nB.begin(), nB.end());
    // nBn = 2*(nB - minB)/(maxB - minB) - 1;
    for (auto it = nB.begin(); it != nB.end(); it++)
    {
        nBn.push_back (0.8 * (2 * (*it - minB) / (maxB - minB) - 1));
    }
}

double Noise::Brown::generate()
{
    // check to see if you hit the end of the buffer, and if yes refill
    if (! (itB < nBn.end()))
    {
        // use last used sample from unormalized buffer for start of new buffer
        fillBuffer (nB.back() + noiseSrc.nextDouble());
        itB = nBn.begin();
    }
    // get output samples and increment buffer iterator
    op = *itB;
    itB++;
    return op;
}

//==============================================================================
double Noise::White::generate()
{
    return jam::Value::map (static_cast<double> (std::rand() % range), 0.0, static_cast<double> (range), -1.0, 1.0);
}

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam::dsp

# Unit Testing for Audio DSP - Comprehensive Guide
**From Zero to Hero: Testing for Humans and LLMs**

---

## What is Unit Testing? (ELI5)

### Without Tests
```
You: "I fixed the highpass filter!"
[Releases plugin]
Customer: "The lowpass is broken now!"
You: "WTF? I didn't touch lowpass..."
[Spends 3 hours debugging]
You: "Oh... I accidentally changed a shared function"
```

### With Tests
```
You: "I fixed the highpass filter!"
[Runs tests]
Tests: "❌ FAIL: LowPass.process() expected 0.5, got 0.8"
You: "Caught it! Let me fix before releasing"
[Fixes bug]
Tests: "✅ PASS: All 47 tests pass"
[Releases with confidence]
```

**Unit Test = Automated proof that your code works**

Think of it like a robot that:
1. Feeds known input to your code
2. Checks if output matches expectation
3. Screams if something is wrong

---

## The 4 Ws + 1 H

### WHO needs unit tests?
- **You in 6 months** - "Did I break anything with this change?"
- **Your teammates** - "Is this pull request safe to merge?"
- **Your customers** - "Does this plugin actually work?"
- **LLMs generating code** - Prove the generated code is correct

### WHAT to test in audio plugins?
1. **DSP classes** (MOST IMPORTANT) - Filters, compressors, shapers
2. **Parameter mapping** - dB conversions, frequency scaling
3. **Preset loading** - V1 → V2 migration
4. **Edge cases** - Zero input, NaN, infinity, negative frequencies
5. **Determinism** - Same input → same output (session recall)

### WHEN to write tests?
- **Before fixing a bug** - Write test that fails, then fix until it passes
- **After adding a feature** - Prove the feature works
- **Before refactoring** - Safety net to prevent breaking things
- **During code review** - Require tests for all new DSP code

### WHERE do tests live?
```
___lib___/jam_dsp/
├── filter/
│   ├── jam_dsp_butterworth.h        ← DSP class
│   └── jam_dsp_butterworth_test.cpp ← Tests (same directory)
├── preamp/
│   ├── jam_dsp_preamp.h
│   └── jam_dsp_preamp_test.cpp
└── tests/
    ├── CMakeLists.txt                   ← Test executable
    └── main.cpp                         ← Test runner
```

### HOW to write tests?
See below for step-by-step guide!

---

## Why Test Audio DSP Specifically?

### Audio DSP is UNFORGIVING
❌ **GUI bug:** Button looks wrong → user notices, you fix, no big deal
❌ **DSP bug:** Filter is unstable → plugin crashes DAW → user loses 3 hours of work → 1-star review

### Audio DSP is MATHEMATICAL
✅ **Deterministic:** Same input MUST produce same output
✅ **Measurable:** You can verify magnitude response, phase, group delay
✅ **Provable:** Can check if filter meets spec (e.g., -3dB at cutoff)

### Audio DSP is MISSION-CRITICAL
- Recording studios depend on your plugin
- If compression is wrong, it ruins the mix
- If EQ is wrong, it ruins the master
- **You NEED proof it works correctly**

---

## Testing Levels (Start Small, Grow Smart)

### Level 0: No Tests (Current State)
```
Manual testing in DAW
Hope nothing breaks
Fix bugs after customers report them
```
**Risk:** HIGH
**Confidence:** LOW

### Level 1: Smoke Tests (Basic Sanity)
```cpp
// Does it crash?
TEST_CASE("Butterworth doesn't crash") {
    Butterworth filter;
    filter.setSampleRate(48000);
    filter.setFrequency(1000);
    double sample = 0.5;
    filter.process(sample);  // If we get here, it didn't crash!
    REQUIRE(true);
}
```
**Implementation time:** 30 minutes
**Recommended:** **START HERE**

### Level 2: Correctness Tests (Proves It Works)
```cpp
// Does it do what it's supposed to?
TEST_CASE("Butterworth DC blocking") {
    Butterworth filter;
    filter.setSampleRate(48000);
    filter.setType(HighPass);
    filter.setFrequency(20);

    // DC signal (0Hz) should be blocked
    double dc = 1.0;
    for (int i = 0; i < 1000; ++i)
        filter.process(dc);

    REQUIRE(std::abs(dc) < 0.01);  // DC attenuated to near-zero
}
```
**Implementation time:** 1 day
**Recommended:** After Level 1

### Level 3: Comprehensive Tests (Professional Grade)
```cpp
// Does it meet spec exactly?
TEST_CASE("Butterworth frequency response") {
    // Verify -3dB at cutoff frequency
    // Verify -40dB/decade rolloff
    // Verify phase response
    // Verify group delay
}
```
**Implementation time:** 1 week
**Recommended:** For commercial releases

---

## Step-by-Step: Add Testing to jam

### Step 1: Choose a Test Framework

**Recommended: Catch2** (Modern, Header-Only, C++17)
- ✅ Header-only (just drop it in)
- ✅ Beautiful output
- ✅ Supports BDD-style tests
- ✅ Widely used in JUCE projects

**Alternative: Google Test** (More features, bigger setup)

**For jam: Use Catch2**

### Step 2: Download Catch2

```bash
cd /Users/jreng/Documents/Poems/___PROJECT___/___lib___

# Download single-header version
curl -o catch2/catch_amalgamated.hpp \
  https://raw.githubusercontent.com/catchorg/Catch2/v3.5.0/extras/catch_amalgamated.hpp

curl -o catch2/catch_amalgamated.cpp \
  https://raw.githubusercontent.com/catchorg/Catch2/v3.5.0/extras/catch_amalgamated.cpp
```

### Step 3: Create Test Directory Structure

```bash
cd /Users/jreng/Documents/Poems/___PROJECT___/___lib___/jam_dsp

mkdir -p tests
cd tests

# Create main test runner
cat > main.cpp << 'EOF'
// jam DSP Test Runner
// Catch2 provides its own main()

#define CATCH_CONFIG_MAIN
#include "../../catch2/catch_amalgamated.hpp"

// Tests are in separate files and will be automatically discovered
EOF

# Create CMakeLists.txt for tests
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.22)
project(JamDSPTests)

# C++17 required (matches jam standard)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Catch2
include_directories(../../catch2)

# Collect all test files
file(GLOB_RECURSE TEST_SOURCES
    "../filter/*_test.cpp"
    "../preamp/*_test.cpp"
    "../shaper/*_test.cpp"
    "../utilities/*_test.cpp"
)

# Create test executable
add_executable(dsp_tests
    main.cpp
    ${TEST_SOURCES}
    ../../catch2/catch_amalgamated.cpp
)

# Enable all warnings
if(MSVC)
    target_compile_options(dsp_tests PRIVATE /W4)
else()
    target_compile_options(dsp_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Define test target
enable_testing()
add_test(NAME AllTests COMMAND dsp_tests)
EOF
```

### Step 4: Write Your First Test

Create `___lib___/jam_dsp/filter/jam_dsp_butterworth_test.cpp`:

```cpp
// Butterworth Filter Unit Tests
// These tests verify the Butterworth filter implementation meets specifications

#include "../../catch2/catch_amalgamated.hpp"
#include "jam_dsp_butterworth.h"
#include <cmath>
#include <vector>

using namespace jam::dsp;

// Helper function: Generate sine wave at specific frequency
std::vector<double> generateSine(double freqHz, double sampleRate, int numSamples)
{
    std::vector<double> signal(numSamples);
    double phase = 0.0;
    double phaseIncrement = 2.0 * M_PI * freqHz / sampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        signal[i] = std::sin(phase);
        phase += phaseIncrement;
    }
    return signal;
}

// Helper function: Measure RMS level of signal
double measureRMS(const std::vector<double>& signal)
{
    double sum = 0.0;
    for (double sample : signal)
        sum += sample * sample;
    return std::sqrt(sum / signal.size());
}

//==============================================================================
// LEVEL 1 TESTS: Basic Sanity
//==============================================================================

TEST_CASE("Butterworth basic instantiation", "[butterworth][smoke]")
{
    SECTION("Default constructor doesn't crash")
    {
        Butterworth filter;
        REQUIRE(true);  // If we got here, it didn't crash
    }

    SECTION("Can set sample rate")
    {
        Butterworth filter;
        filter.setSampleRate(48000.0);
        filter.setSampleRate(44100.0);
        filter.setSampleRate(96000.0);
        REQUIRE(true);
    }

    SECTION("Can set frequency")
    {
        Butterworth filter;
        filter.setSampleRate(48000.0);
        filter.setFrequency(100.0);
        filter.setFrequency(1000.0);
        filter.setFrequency(10000.0);
        REQUIRE(true);
    }
}

TEST_CASE("Butterworth process doesn't crash", "[butterworth][smoke]")
{
    Butterworth filter;
    filter.setSampleRate(48000.0);
    filter.setFrequency(1000.0);

    SECTION("Process single sample")
    {
        double sample = 0.5;
        filter.process(sample);
        REQUIRE(std::isfinite(sample));  // Not NaN or infinity
    }

    SECTION("Process 1000 samples")
    {
        for (int i = 0; i < 1000; ++i)
        {
            double sample = std::sin(i * 0.1);
            filter.process(sample);
            REQUIRE(std::isfinite(sample));
        }
    }
}

//==============================================================================
// LEVEL 2 TESTS: Correctness
//==============================================================================

TEST_CASE("Butterworth DC blocking (highpass)", "[butterworth][correctness]")
{
    Butterworth filter;
    filter.setSampleRate(48000.0);
    filter.setType(FilterType::HighPass);
    filter.setFrequency(20.0);  // 20Hz highpass

    // Feed DC signal (0Hz) - should be blocked
    double sample = 1.0;
    for (int i = 0; i < 1000; ++i)
        filter.process(sample);

    // After 1000 samples, DC should be attenuated to near-zero
    REQUIRE(std::abs(sample) < 0.01);
}

TEST_CASE("Butterworth passband (lowpass)", "[butterworth][correctness]")
{
    Butterworth filter;
    filter.setSampleRate(48000.0);
    filter.setType(FilterType::LowPass);
    filter.setFrequency(10000.0);  // 10kHz lowpass

    // Generate 100Hz sine (well below cutoff)
    auto input = generateSine(100.0, 48000.0, 1000);
    double inputRMS = measureRMS(input);

    // Process through filter
    filter.reset();
    for (auto& sample : input)
        filter.process(sample);

    double outputRMS = measureRMS(input);

    // In passband, signal should pass through with <1dB attenuation
    double gainDb = 20.0 * std::log10(outputRMS / inputRMS);
    REQUIRE(gainDb > -1.0);  // Less than 1dB loss
    REQUIRE(gainDb < 0.5);   // Not amplified
}

TEST_CASE("Butterworth determinism", "[butterworth][correctness]")
{
    // Same input + same settings = EXACT same output
    Butterworth filter1, filter2;

    filter1.setSampleRate(48000.0);
    filter2.setSampleRate(48000.0);
    filter1.setFrequency(1000.0);
    filter2.setFrequency(1000.0);

    auto signal1 = generateSine(500.0, 48000.0, 100);
    auto signal2 = signal1;  // Exact copy

    for (auto& s : signal1) filter1.process(s);
    for (auto& s : signal2) filter2.process(s);

    // Must be BIT-EXACT
    for (size_t i = 0; i < signal1.size(); ++i)
    {
        REQUIRE(signal1[i] == signal2[i]);
    }
}

TEST_CASE("Butterworth reset clears state", "[butterworth][correctness]")
{
    Butterworth filter;
    filter.setSampleRate(48000.0);
    filter.setFrequency(1000.0);

    // Process some signal to build up state
    for (int i = 0; i < 100; ++i)
    {
        double sample = 1.0;
        filter.process(sample);
    }

    // Reset should clear internal state
    filter.reset();

    // After reset, processing zero should yield zero
    // (No residual state from previous signal)
    double sample = 0.0;
    filter.process(sample);
    REQUIRE(sample == 0.0);
}

//==============================================================================
// LEVEL 3 TESTS: Specification Compliance
//==============================================================================

TEST_CASE("Butterworth cutoff frequency accuracy", "[butterworth][spec]")
{
    Butterworth filter;
    filter.setSampleRate(48000.0);
    filter.setType(FilterType::LowPass);
    filter.setFrequency(1000.0);  // 1kHz cutoff

    // At cutoff frequency, gain should be -3dB (0.707 linear)
    auto signal = generateSine(1000.0, 48000.0, 1000);
    double inputRMS = measureRMS(signal);

    filter.reset();
    for (auto& s : signal) filter.process(s);

    double outputRMS = measureRMS(signal);
    double gainLinear = outputRMS / inputRMS;

    // Butterworth: -3dB at cutoff = 0.707 linear gain
    REQUIRE(gainLinear == Catch::Approx(0.707).epsilon(0.05));  // ±5% tolerance
}

TEST_CASE("Butterworth edge cases", "[butterworth][edge-cases]")
{
    Butterworth filter;
    filter.setSampleRate(48000.0);
    filter.setFrequency(1000.0);

    SECTION("Zero input produces zero output")
    {
        filter.reset();
        double sample = 0.0;
        for (int i = 0; i < 100; ++i)
        {
            filter.process(sample);
            REQUIRE(sample == 0.0);
        }
    }

    SECTION("Very small input doesn't underflow")
    {
        filter.reset();
        double sample = 1e-20;  // Tiny signal
        for (int i = 0; i < 100; ++i)
            filter.process(sample);

        REQUIRE(std::isfinite(sample));  // Not NaN
    }

    SECTION("Handles Nyquist frequency")
    {
        filter.setFrequency(24000.0);  // Nyquist for 48kHz
        double sample = 1.0;
        filter.process(sample);
        REQUIRE(std::isfinite(sample));
    }
}

//==============================================================================
// CONTRACT COMPLIANCE TESTS
//==============================================================================

TEST_CASE("Butterworth calc() contract", "[butterworth][contract]")
{
    Butterworth filter;

    SECTION("setSampleRate calls calc()")
    {
        // After setSampleRate, filter should be ready to process
        filter.setSampleRate(48000.0);
        filter.setFrequency(1000.0);

        double sample = 1.0;
        filter.process(sample);
        REQUIRE(std::isfinite(sample));  // No crash = calc() was called
    }

    SECTION("setFrequency calls calc()")
    {
        filter.setSampleRate(48000.0);
        filter.setFrequency(1000.0);

        // Frequency change should update coefficients immediately
        filter.setFrequency(2000.0);

        double sample = 1.0;
        filter.process(sample);
        REQUIRE(std::isfinite(sample));
    }
}

TEST_CASE("Butterworth trivially copyable", "[butterworth][contract]")
{
    // CRITICAL: DSP classes MUST be trivially copyable
    STATIC_REQUIRE(std::is_trivially_copyable_v<Butterworth>);
}
```

### Step 5: Build and Run Tests

```bash
cd /Users/jreng/Documents/Poems/___PROJECT___/___lib___/jam_dsp/tests

# Configure
mkdir -p build && cd build
cmake -G Ninja ..

# Build
ninja

# Run tests
./dsp_tests

# Expected output:
# ===============================================================================
# All tests passed (25 assertions in 10 test cases)
```

---

## Test Organization Patterns

### Pattern 1: Co-located Tests (Recommended)
```
jam_dsp/
├── filter/
│   ├── jam_dsp_butterworth.h
│   └── jam_dsp_butterworth_test.cpp  ← Same directory as class
```
**Benefits:**
- Easy to find tests for a class
- Tests are versioned with the code
- Encourages writing tests

### Pattern 2: Separate Test Directory
```
jam_dsp/
├── filter/
│   └── jam_dsp_butterworth.h
└── tests/
    └── filter/
        └── butterworth_test.cpp
```
**Benefits:**
- Clean separation
- Can exclude tests from production builds

### Pattern 3: Integration Tests
```
jreng-filter-strip/
└── tests/
    ├── test_preset_loading.cpp      ← Tests across multiple layers
    ├── test_parameter_automation.cpp
    └── test_full_processing_chain.cpp
```

---

## What to Test (Priority Order)

### Priority 1: DSP Classes (CRITICAL)
```cpp
TEST_CASE("Filter doesn't crash") { /* ... */ }
TEST_CASE("Filter is deterministic") { /* ... */ }
TEST_CASE("Filter blocks DC (highpass)") { /* ... */ }
TEST_CASE("Filter passes DC (lowpass)") { /* ... */ }
TEST_CASE("Filter cutoff accuracy") { /* ... */ }
```

### Priority 2: Parameter Mapping
```cpp
TEST_CASE("dB to linear conversion") {
    REQUIRE(dbToLinear(0.0) == 1.0);
    REQUIRE(dbToLinear(-6.0) == Catch::Approx(0.501).epsilon(0.01));
    REQUIRE(dbToLinear(-inf) == 0.0);
}

TEST_CASE("Frequency to MIDI note") {
    REQUIRE(freqToMidi(440.0) == Catch::Approx(69.0).epsilon(0.01));  // A4
    REQUIRE(freqToMidi(261.63) == Catch::Approx(60.0).epsilon(0.01)); // C4
}
```

### Priority 3: Preset Loading/Migration
```cpp
TEST_CASE("Load V1 preset") {
    auto preset = loadPreset("test_preset_v1.xml");
    REQUIRE(preset.version == 1);
    REQUIRE(preset.parameters["GAIN"] == Catch::Approx(0.5));
}

TEST_CASE("Migrate V1 to V2") {
    auto v1 = loadPreset("v1_linear_gain.xml");
    auto v2 = migrateToV2(v1);

    // V2 uses dB, V1 used linear
    REQUIRE(v2.parameters["GAIN_DB"] == Catch::Approx(-6.0).epsilon(0.1));
}
```

### Priority 4: Edge Cases
```cpp
TEST_CASE("Handle edge cases") {
    SECTION("Zero sample rate") {
        // Should not crash or divide by zero
        filter.setSampleRate(0.0);
        REQUIRE(/* sensible fallback */);
    }

    SECTION("Negative frequency") {
        filter.setFrequency(-1000.0);
        REQUIRE(/* clamped to positive or ignored */);
    }

    SECTION("NaN input") {
        double sample = NAN;
        filter.process(sample);
        REQUIRE(std::isfinite(sample));  // Should sanitize NaN
    }
}
```

---

## Test-Driven Development (TDD) Workflow

### The Red-Green-Refactor Cycle

#### 1. RED: Write Failing Test
```cpp
TEST_CASE("Compressor gain reduction") {
    Compressor comp;
    comp.setThreshold(-10.0);  // -10dB threshold
    comp.setRatio(4.0);        // 4:1 ratio

    double grDb = 0.0;
    double sample = dbToLinear(-4.0);  // 6dB over threshold
    comp.process(sample, grDb);

    // Expected GR: 6dB over / 4:1 ratio = 1.5dB reduction
    REQUIRE(grDb == Catch::Approx(-1.5).epsilon(0.1));
}
```
**Run:** ❌ FAIL (Compressor::process doesn't exist yet)

#### 2. GREEN: Write Minimum Code to Pass
```cpp
void Compressor::process(double& sample, double& grDb)
{
    double sampleDb = linearToDb(std::abs(sample));
    double over = sampleDb - threshold;

    if (over > 0.0)
        grDb = -over / ratio;
    else
        grDb = 0.0;

    sample *= dbToLinear(grDb);
}
```
**Run:** ✅ PASS

#### 3. REFACTOR: Improve Code (Tests Still Pass)
```cpp
void Compressor::process(double& sample, double& grDb)
{
    // Extract to helper function
    grDb = calculateGainReduction(sample);
    sample *= dbToLinear(grDb);
}

double Compressor::calculateGainReduction(double sample) const
{
    double sampleDb = linearToDb(std::abs(sample));
    double over = std::max(0.0, sampleDb - threshold);
    return over > 0.0 ? -over / ratio : 0.0;
}
```
**Run:** ✅ PASS (still works, but cleaner)

---

## Integration with CMake Build

### Add Tests to Plugin CMakeLists.txt

```cmake
# jreng-filter-strip/CMakeLists.txt

cmake_minimum_required(VERSION 3.22)

# ... existing plugin configuration ...

# Option to build tests (default OFF for release builds)
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

### Plugin Tests CMakeLists.txt

```cmake
# jreng-filter-strip/tests/CMakeLists.txt

project(FilterStripTests)

# Catch2
include_directories(../../___lib___/catch2)

# Test sources
add_executable(filter_strip_tests
    main.cpp
    test_processor_chain.cpp
    test_parameter_mapping.cpp
    test_preset_loading.cpp
    ../../___lib___/catch2/catch_amalgamated.cpp
)

# Link plugin libraries
target_link_libraries(filter_strip_tests
    PRIVATE
        jam::core
        jam::dsp
)

# Add to CTest
add_test(NAME AllPluginTests COMMAND filter_strip_tests)
```

---

## Running Tests

### Command Line
```bash
# Build and run all tests
cd jreng-filter-strip/Builds/Ninja
cmake -DBUILD_TESTS=ON ../..
ninja
ctest

# Run specific test
./tests/filter_strip_tests "Butterworth"

# Verbose output
./tests/filter_strip_tests --success

# List all tests
./tests/filter_strip_tests --list-tests
```

### CI/CD Integration (Add to GitHub Actions)
```yaml
- name: Build and Test
  run: |
    cmake -DBUILD_TESTS=ON -B build
    cmake --build build
    cd build && ctest --output-on-failure
```

---

## Common Testing Patterns for Audio

### Pattern 1: Frequency Response Test
```cpp
double measureGainAtFrequency(Filter& filter, double freqHz, double sampleRate)
{
    auto input = generateSine(freqHz, sampleRate, 1000);
    double inputRMS = measureRMS(input);

    filter.reset();
    for (auto& s : input) filter.process(s);

    double outputRMS = measureRMS(input);
    return 20.0 * std::log10(outputRMS / inputRMS);
}

TEST_CASE("Lowpass frequency response") {
    LowPass filter;
    filter.setSampleRate(48000);
    filter.setFrequency(1000);

    REQUIRE(measureGainAtFrequency(filter, 100, 48000) > -1.0);    // Passband
    REQUIRE(measureGainAtFrequency(filter, 1000, 48000) == Approx(-3.0)); // Cutoff
    REQUIRE(measureGainAtFrequency(filter, 10000, 48000) < -20.0); // Stopband
}
```

### Pattern 2: State Snapshot Test
```cpp
TEST_CASE("Filter state is copyable") {
    Filter original;
    original.setSampleRate(48000);
    original.setFrequency(1000);

    // Process some signal to build up state
    for (int i = 0; i < 100; ++i) {
        double s = std::sin(i * 0.1);
        original.process(s);
    }

    // Copy filter (trivially copyable)
    Filter copy = original;

    // Both should produce identical output
    double s1 = 0.5, s2 = 0.5;
    original.process(s1);
    copy.process(s2);

    REQUIRE(s1 == s2);
}
```

### Pattern 3: Denormal Test
```cpp
TEST_CASE("Filter flushes denormals") {
    Filter filter;
    filter.setSampleRate(48000);
    filter.setFrequency(100);

    // Feed tiny signal that would create denormals
    double sample = 1e-40;
    for (int i = 0; i < 1000; ++i)
        filter.process(sample);

    // Should not have denormals (flushed to zero)
    REQUIRE(std::abs(sample) > 1e-15 || sample == 0.0);
}
```

---

## Debugging Failed Tests

### Technique 1: Use Sections for Isolation
```cpp
TEST_CASE("Complex test") {
    SECTION("Part 1") { /* ... */ }  // If this fails, only Part 1 is the problem
    SECTION("Part 2") { /* ... */ }
    SECTION("Part 3") { /* ... */ }
}
```

### Technique 2: Print Debug Info
```cpp
TEST_CASE("Filter gain") {
    double gain = measureGain(filter);

    INFO("Sample rate: " << filter.getSampleRate());
    INFO("Frequency: " << filter.getFrequency());
    INFO("Measured gain: " << gain << " dB");

    REQUIRE(gain == Approx(-3.0).epsilon(0.1));
    // If this fails, INFO statements are printed
}
```

### Technique 3: Generate Expected vs Actual Plots
```cpp
TEST_CASE("Filter response") {
    // ... test logic ...

    if (testFailed) {
        // Write CSV for plotting
        std::ofstream csv("filter_response.csv");
        csv << "Frequency,Expected,Actual\n";
        for (auto freq : frequencies)
            csv << freq << "," << expected[freq] << "," << actual[freq] << "\n";

        FAIL("Response mismatch - see filter_response.csv for plot data");
    }
}
```

---

## Advanced: Property-Based Testing

### Concept: Test Properties, Not Values
Instead of: "Input X should produce output Y"
Test: "For ALL inputs, property P holds"

### Example: Lowpass Should Never Amplify
```cpp
#include <random>

TEST_CASE("Lowpass never amplifies signal") {
    LowPass filter;
    filter.setSampleRate(48000);
    filter.setFrequency(1000);

    std::mt19937 rng(12345);  // Seeded for reproducibility
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    // Test 1000 random samples
    for (int i = 0; i < 1000; ++i) {
        double input = dist(rng);
        double output = input;
        filter.process(output);

        // Property: Output should never exceed input magnitude
        REQUIRE(std::abs(output) <= std::abs(input) + 1e-10);  // Small epsilon for FP error
    }
}
```

---

## TL;DR - Quick Start Checklist

### ✅ Setup (30 minutes)
- [ ] Download Catch2 to `___lib___/catch2/`
- [ ] Create `___lib___/jam_dsp/tests/CMakeLists.txt`
- [ ] Create `___lib___/jam_dsp/tests/main.cpp`

### ✅ Write First Test (30 minutes)
- [ ] Create `jam_dsp_butterworth_test.cpp`
- [ ] Write "doesn't crash" test
- [ ] Build: `cmake .. && ninja`
- [ ] Run: `./dsp_tests`
- [ ] See green ✅

### ✅ Add More Tests (1 day)
- [ ] DC blocking test
- [ ] Determinism test
- [ ] Reset test
- [ ] Trivially copyable test

### ✅ Integrate with CI (1 hour)
- [ ] Add `BUILD_TESTS=ON` to GitHub Actions
- [ ] Add `ctest` step
- [ ] Every push now runs tests automatically

---

## Resources

### Learning
- Catch2 docs: https://github.com/catchorg/Catch2
- Test-Driven Development by Kent Beck (book)
- Google Test primer: https://google.github.io/googletest/primer.html

### Examples
- JUCE DSP tests: https://github.com/juce-framework/JUCE/tree/master/modules/juce_dsp
- Tracktion Engine tests: https://github.com/Tracktion/tracktion_engine

---

**jam! (Now with proof that it works)**

*Version 1.0 - January 2025*
*Unit Testing Guide for Professional Audio Plugins*

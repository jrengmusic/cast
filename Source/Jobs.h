#pragma once
#include <JuceHeader.h>

struct Jobs
{
    static constexpr int indefiniteTimeoutMs { -1 };

    template <typename Function>
    static void run (int count, Function&& function)
    {
        juce::ThreadPool pool;

        for (int index { 0 }; index < count; ++index)
            pool.addJob ([&function, index] { function (index); });

        pool.removeAllJobs (false, indefiniteTimeoutMs);
    }
};

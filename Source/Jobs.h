#pragma once
#include <JuceHeader.h>

/**
 * @struct Jobs
 * @brief Static utility that dispatches a fixed count of indexed units of
 *        work across a transient juce::ThreadPool and blocks until every
 *        unit has completed.
 *
 * Jobs owns no state of its own -- the pool it creates lives for the
 * duration of a single run() call and is torn down before run() returns.
 */
struct Jobs
{
    /** Poll interval, in milliseconds, while waiting for queued jobs to
     *  drain from the pool. */
    static constexpr int drainPollMs { 1 };

    /**
     * @brief Runs @p function once per index in [0, @p count), each
     *        invocation dispatched to its own job on a transient thread
     *        pool, and blocks until every job has completed.
     *
     * @param count    The number of indexed units of work to run.
     * @param function Callable invoked as @c function(index) for each
     *                 index in [0, @p count).
     */
    template <typename Function>
    static void run (int count, Function&& function)
    {
        juce::ThreadPool pool;

        for (int index { 0 }; index < count; ++index)
            pool.addJob ([&function, index] { function (index); });

        while (pool.getNumJobs() > 0)
            juce::Thread::sleep (drainPollMs);
    }
};

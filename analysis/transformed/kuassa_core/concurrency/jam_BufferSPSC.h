/**
 * @file jam_BufferSPSC.h
 * @brief Index-only SPSC ring fork of juce::AbstractFifo with producer-side drop-oldest.
 */

#pragma once

namespace jam
{

/**
 * @class BufferSPSC
 * @brief Index-only fork of juce::AbstractFifo with producer-side drop-oldest.
 *
 * SPSC only (one producer thread, one consumer thread).  Multiple producers
 * race on validEnd; multiple consumers race on validStart.  Neither race is
 * guarded — violating the SPSC contract produces data races.
 *
 * The class is a fork of juce::AbstractFifo (JUCE 8.0.12): same index
 * arithmetic (validStart / validEnd / bufferSize), same free-space sentinel
 * (one permanently reserved slot), same wrap formulas.  The caller owns the
 * byte buffer and drives memcpy against the returned indices, exactly as with
 * AbstractFifo.  The interface is verbatim AbstractFifo with ONE signature
 * change: finishedRead takes (startIndex, numRead) — the startIndex captured
 * from prepareToRead — rather than (numRead) alone.  This is required for the
 * absolute-commit reconciliation against the concurrent producer drop-oldest
 * writer on validStart (see finishedRead doc).
 *
 * The ONE behavioral change versus AbstractFifo: prepareToWrite is non-const
 * and on overflow drops the oldest entry by advancing validStart instead of
 * clamping numToWrite.  The write therefore always succeeds — the producer
 * never refuses or stalls.
 *
 * validStart has two writers: the consumer (finishedRead) and the producer
 * (drop-oldest in prepareToWrite).  finishedRead commits an ABSOLUTE end
 * position — (capturedStart + numRead) % bufferSize — not a relative add.
 * This eliminates the read-index race: a concurrent producer drop cannot shift
 * the committed target (see finishedRead doc for full reconciliation).
 * validStart is std::atomic<int> (CAS required for two-writer reconciliation);
 * validEnd is juce::Atomic<int> (single writer, no CAS needed).
 *
 * Torn-read guard is the CALLER's responsibility.  BufferSPSC provides index
 * management and drop-oldest only; it does not guard against the producer
 * reclaiming bytes the consumer is mid-memcpy on.  CellFifo (the caller in
 * END) implements a per-slot seqlock epoch array for this.
 *
 * Drop-oldest operates in raw slot units.  Whole-entry framing is the
 * caller's concern.  BufferSPSC is payload-agnostic and names no type from
 * jam_graphics or any higher layer.
 */
class BufferSPSC
{
public:
    /** Constructs an index-only ring of bufferSize slots.
     *  Verbatim AbstractFifo ctor — bufferSize must be > 0.
     */
    explicit BufferSPSC (int bufferSize) noexcept
        : bufferSize (bufferSize)
    {
        jassert (bufferSize > 0);
    }

    ~BufferSPSC() = default;

    // =========================================================================
    // Verbatim AbstractFifo interface (with prepareToWrite made non-const)

    /** Returns the total capacity in slots (verbatim AbstractFifo). */
    int getTotalSize() const noexcept
    {
        return bufferSize;
    }

    /** Returns the number of free slots (verbatim AbstractFifo: rawFree - 1). */
    int getFreeSpace() const noexcept
    {
        return bufferSize - getNumReady() - 1;
    }

    /** Returns the number of slots currently occupied (verbatim AbstractFifo). */
    int getNumReady() const noexcept
    {
        const int vs { validStart.load (std::memory_order_acquire) };
        const int ve { validEnd.get() };
        return ve >= vs ? (ve - vs) : (bufferSize - (vs - ve));
    }

    /** Resets both indices to zero (verbatim AbstractFifo). */
    void reset() noexcept
    {
        validEnd = 0;
        validStart.store (0, std::memory_order_release);
    }

    /** Resets and resizes (verbatim AbstractFifo). */
    void setTotalSize (int newSize) noexcept
    {
        jassert (newSize > 0);
        reset();
        bufferSize = newSize;
    }

    /**
     * @brief Prepare a write of numToWrite slots, returning two contiguous
     *        block descriptors (verbatim AbstractFifo interface; NON-const here
     *        because drop-oldest mutates validStart when the ring is full).
     *
     * The ONE behavioral change versus AbstractFifo: when freeSpace - 1 is
     * less than numToWrite, advance validStart (drop oldest) until the
     * requested space fits.  The write always succeeds — numToWrite slots are
     * always reserved.  AbstractFifo would clamp and possibly return 0.
     *
     * validStart CAS reconciliation (two writers — producer and consumer):
     * One compare_exchange_strong per drop iteration.  CAS failure means the
     * consumer advanced validStart concurrently — space was already freed.
     * Re-read freeSpace and skip eviction for this iteration.  No retry loop.
     */
    void prepareToWrite (int numToWrite, int& startIndex1, int& blockSize1,
                         int& startIndex2, int& blockSize2) noexcept
    {
        // Drop-oldest loop: advance validStart until freeSpace - 1 >= numToWrite.
        // Each iteration attempts one CAS; on failure the consumer freed space —
        // re-read and check again without dropping.
        {
            int vs { validStart.load (std::memory_order_acquire) };
            int ve { validEnd.get() };
            int freeSpace { ve >= vs ? (bufferSize - (ve - vs)) : (vs - ve) };

            while (freeSpace - 1 < numToWrite)
            {
                // Re-read after each CAS (success or failure).
                vs = validStart.load (std::memory_order_acquire);
                ve = validEnd.get();
                freeSpace = ve >= vs ? (bufferSize - (ve - vs)) : (vs - ve);

                if (freeSpace - 1 >= numToWrite)
                    break;

                // Advance validStart by one slot (raw slot granularity).
                // BufferSPSC is payload-agnostic — it does not read entry
                // headers to find whole-entry boundaries.  Callers (e.g.
                // CellFifo) handle entry alignment at their layer: the
                // producer calls prepareToWrite with numToWrite = full entry
                // size, so each prepareToWrite either drops enough raw slots
                // to make room for one complete entry or not at all.
                int nextVs { vs + 1 };

                if (nextVs >= bufferSize)
                    nextVs -= bufferSize;

                int expected { vs };
                const bool casSucceeded { validStart.compare_exchange_strong (
                    expected, nextVs,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed) };

                // Whether CAS succeeded or failed, re-read and retry the space check.
                // CAS success → we dropped one slot; CAS failure → consumer freed
                // space, space may now be sufficient without dropping.
                (void) casSucceeded;
            }
        }

        // Compute write blocks — verbatim AbstractFifo body (positive-check form).
        const int vs { validStart.load (std::memory_order_acquire) };
        const int ve { validEnd.get() };
        const int freeSpace { ve >= vs ? (bufferSize - (ve - vs)) : (vs - ve) };
        const int toWrite { juce::jmin (numToWrite, freeSpace - 1) };

        if (toWrite <= 0)
        {
            startIndex1 = 0;
            blockSize1 = 0;
            startIndex2 = 0;
            blockSize2 = 0;
        }
        else
        {
            startIndex1 = ve;
            startIndex2 = 0;
            blockSize1 = juce::jmin (bufferSize - ve, toWrite);
            const int remaining { toWrite - blockSize1 };
            blockSize2 = remaining <= 0 ? 0 : juce::jmin (remaining, vs);
        }
    }

    /** Advances validEnd after a write (verbatim AbstractFifo). */
    void finishedWrite (int numWritten) noexcept
    {
        int newEnd { validEnd.get() + numWritten };

        if (newEnd >= bufferSize)
            newEnd -= bufferSize;

        validEnd = newEnd;
    }

    /** Prepare a read of numWanted slots (verbatim AbstractFifo, const). */
    void prepareToRead (int numWanted, int& startIndex1, int& blockSize1,
                        int& startIndex2, int& blockSize2) const noexcept
    {
        const int vs { validStart.load (std::memory_order_acquire) };
        const int ve { validEnd.get() };
        const int numReady { ve >= vs ? (ve - vs) : (bufferSize - (vs - ve)) };
        const int toRead { juce::jmin (numWanted, numReady) };

        if (toRead <= 0)
        {
            startIndex1 = 0;
            blockSize1 = 0;
            startIndex2 = 0;
            blockSize2 = 0;
        }
        else
        {
            startIndex1 = vs;
            startIndex2 = 0;
            blockSize1 = juce::jmin (bufferSize - vs, toRead);
            const int remaining { toRead - blockSize1 };
            blockSize2 = remaining <= 0 ? 0 : juce::jmin (remaining, ve);
        }
    }

    /**
     * @brief Advances validStart to the absolute end of the completed read.
     *
     * @param startIndex  The validStart value captured during prepareToRead
     *                    (the startIndex1 returned by that call).  This is the
     *                    first slot this read occupied.
     * @param numRead     Number of slots consumed (verbatim AbstractFifo: 0 is
     *                    valid — no-op).
     *
     * ABSOLUTE COMMIT — why it is necessary
     * ======================================
     * validStart has two writers: the consumer (this function) and the producer
     * (drop-oldest in prepareToWrite).  The previous implementation used a
     * relative advance (CAS: newStart = currentValidStart + numRead).  That is
     * incorrect: if the producer's drop-CAS fires between the consumer's load and
     * CAS attempt, the consumer retries from the post-drop value and advances by
     * drop + numRead instead of the correct max(drop, numRead) — corrupting the
     * index (consumer appears to have freed more slots than it actually read).
     *
     * The fix commits an absolute end: validStart = (startIndex + numRead) %
     * bufferSize.  The consumer already holds startIndex from prepareToRead;
     * the read end is fully determined before finishedRead is called.  A
     * concurrent producer drop cannot shift this target.
     *
     * CAS RECONCILIATION — two writers, absolute commit
     * ==================================================
     * The CAS uses startIndex as the expected value (validStart at the moment
     * the consumer began reading) and absoluteEnd as the desired value.
     *
     * Case 1 — CAS succeeds: the producer did not advance validStart during
     * the read.  We place the index exactly at our read end.  Done.
     *
     * Case 2 — CAS fails: the producer's drop-CAS fired concurrently and
     * advanced validStart from startIndex to producerVS.  We need
     * max(producerVS, absoluteEnd) in forward-ring order from startIndex.
     * Compare forward distances:
     *   distProducer = (producerVS - startIndex + bufferSize) % bufferSize
     *   if distProducer >= numRead: producer already moved past our read end —
     *       accept the producer's position, done.
     *   if distProducer < numRead: producer dropped fewer slots than we read;
     *       we must still push to absoluteEnd.  CAS again with expected =
     *       producerVS, desired = absoluteEnd.  Repeat until either we succeed
     *       or the producer surpasses absoluteEnd on its own.
     *
     * Under the SPSC contract (one consumer), the loop contends only against
     * the producer's single-slot drop-CAS.  Each iteration makes forward
     * progress: on CAS failure the producer has advanced validStart at least
     * one slot closer to absoluteEnd, so the distance to close strictly shrinks.
     * The loop terminates in at most numRead iterations; in practice ≤ 2.
     *
     * INDEX COHERENCE GUARANTEE
     * =========================
     * After finishedRead returns, validStart lies at or past absoluteEnd in
     * forward-ring order from startIndex.  getNumReady() never underflows or
     * over-counts: the committed position is always the furthest of (what the
     * producer dropped, what the consumer read).  No index drift; no negative
     * ready-count; no double-free of slots.
     *
     * NOTE: the caller's bytes may have been reclaimed by the producer mid-read
     * (torn read).  That is detected and discarded by CellFifo's per-slot
     * seqlock guard; BufferSPSC's responsibility ends at index coherence.
     */
    void finishedRead (int startIndex, int numRead) noexcept
    {
        jassert (numRead >= 0);

        if (numRead > 0)
        {
            int absoluteEnd { startIndex + numRead };

            if (absoluteEnd >= bufferSize)
                absoluteEnd -= bufferSize;

            // CAS expected = startIndex (the validStart we captured in prepareToRead).
            // On failure, expected is updated by compare_exchange_weak to the producer's
            // current validStart.  We then check whether the producer already surpassed
            // our absolute end and either stop or retry with the new base.
            int expected { startIndex };

            while (true)
            {
                if (validStart.compare_exchange_weak (
                        expected, absoluteEnd,
                        std::memory_order_acq_rel,
                        std::memory_order_acquire))
                {
                    // Success: validStart is now at absoluteEnd.
                    break;
                }

                // CAS failed: expected now holds the producer's current validStart.
                // Compute how far ahead of startIndex the producer has moved.
                const int distProducer { (expected - startIndex + bufferSize) % bufferSize };

                if (distProducer >= numRead)
                {
                    // Producer already advanced past our read end — index is correct.
                    break;
                }

                // Producer dropped fewer slots than we read; retry to push to absoluteEnd.
                // expected already holds the new validStart; next CAS will use it.
            }
        }
    }

private:
    // =========================================================================
    // State (verbatim AbstractFifo member layout)

    int bufferSize;

    // validEnd: single writer (producer via finishedWrite) — juce::Atomic suffices.
    juce::Atomic<int> validEnd { 0 };

    // validStart: two writers (consumer via finishedRead; producer via drop-oldest
    // in prepareToWrite) — std::atomic<int> for compare_exchange_strong.
    std::atomic<int> validStart { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BufferSPSC)
};

}// namespace jam

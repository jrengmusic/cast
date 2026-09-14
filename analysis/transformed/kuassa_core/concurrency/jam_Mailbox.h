/**
 * @file jam_Mailbox.h
 * @brief Single-slot, lock-free pointer handoff between one writer and one reader thread.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Single-slot, lock-free handoff of a heap-allocated snapshot between
 *        one writer thread and one reader thread.
 *
 * write() installs a new snapshot and returns whatever was previously in the
 * slot (the caller is responsible for deleting it — Mailbox does not retain
 * ownership of superseded snapshots). read() atomically takes the current
 * snapshot, leaving the slot empty. Only the latest write between two reads
 * is ever observed — this is not a queue.
 *
 * @tparam SnapshotType Type of the heap-allocated object exchanged through the slot.
 */
template <typename SnapshotType>
class Mailbox
{
public:
    /** @brief Constructs an empty mailbox. */
    Mailbox() = default;

    /** @brief Deletes whatever snapshot is currently in the slot, if any. */
    ~Mailbox() noexcept { delete slot.exchange (nullptr, std::memory_order_acq_rel); }

    /**
     * @brief Installs a new snapshot, to be called from the message thread.
     *
     * @param latest  Newly allocated snapshot taking ownership of the slot.
     * @return The previously installed snapshot, or nullptr if none — the
     *         caller must delete the returned pointer.
     */
    SnapshotType* write (SnapshotType* latest) noexcept
    {
        return slot.exchange (latest, std::memory_order_acq_rel);
    }

    /**
     * @brief Takes the current snapshot, to be called from the render thread.
     *
     * @return The snapshot installed by the most recent write(), or nullptr
     *         if none is pending. The caller takes ownership of the returned
     *         pointer.
     */
    SnapshotType* read() noexcept { return slot.exchange (nullptr, std::memory_order_acq_rel); }

    /** @brief Returns true when a snapshot is pending (a write not yet consumed by read()). */
    bool isReady() const noexcept { return slot.load (std::memory_order_acquire) != nullptr; }

private:
    std::atomic<SnapshotType*> slot { nullptr };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mailbox)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam

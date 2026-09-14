/**
 * @file jam_Instance.h
 * @brief CRTP base providing scoped global-access lifecycle for a derived type
 *        via a process-global LIFO instance chain.
 */

namespace jam
{
/*____________________________________________________________________________*/
/**
 * @brief Global-access contract for instance-bounded ownership and lifecycle.
 *
 * Process-global LIFO intrusive linked list. Each instance pushes itself onto
 * the chain on construction and pops on destruction, synchronized by an
 * internal mutex. Multiple instances of the same derived type may coexist in
 * one process without collision — the latest constructed wins (LIFO). When
 * that instance is destroyed the previous one resumes as the active instance.
 * `getInstance()` is a lock-free atomic read, visible from any thread.
 *
 * Two-layer contract: `Instance<T>` is the argless CRTP access layer only — it
 * grants scoped global lookup, it does not own. Shared ownership across
 * multiple owners (create-or-join, ref-counted, last drop destroys) is
 * `SharedInstance<T>` (jam_SharedInstance.h). Direct plain-member
 * construction remains legal wherever the member is that object's sole
 * owner.
 *
 * Public API:
 *  - Derived type inherits `Instance<Derived>` (CRTP).
 *  - Owner declares the derived instance; ctor registers, dtor unregisters.
 *  - `getInstance()` returns the active instance for the current scope.
 *  - Copy and move are deleted — ownership is non-transferable.
 *
 * Usage:
 * @code
 * struct Appearance : jam::Instance<Appearance> { ... };
 *
 * // owner (e.g. JUCEApplication or PluginProcessor):
 * Appearance appearance;          // registers on ctor, unregisters on dtor
 *
 * // consumer (anywhere downstream):
 * if (auto* a = Appearance::getInstance()) { ... }
 * @endcode
 *
 * @tparam ObjectType The derived class. MUST be the class itself (CRTP).
 */
template <typename ObjectType>
struct Instance
{
public:

    /**
     * @brief Constructs and registers this instance on the process-global chain.
     *
     * Under the chain mutex: saves the current chain head as `previous`, then
     * installs this instance as the new head. On destruction the saved
     * pointer is restored.
     *
     * Contract: noexcept. Static assert enforces correct CRTP usage at
     * compile time — ObjectType must derive from `Instance<ObjectType>`.
     */
    Instance() noexcept
    {
        static_assert (std::is_base_of<Instance<ObjectType>, ObjectType>::value,
                       "Instance<T> must be used with the derived class itself. "
                       "Correct usage: class MyClass : public Instance<MyClass> { }");

        const std::scoped_lock chainLock { chainMutex() };
        previous = head().load (std::memory_order_relaxed);
        head().store (static_cast<ObjectType*> (this), std::memory_order_release);
    }

    /**
     * @brief Unlinks this instance from the process-global chain, wherever it sits.
     *
     * Under the chain mutex: LIFO destruction order is the common case
     * (head() == this) — the chain head simply steps back to `previous`.
     * Arbitrary-order destruction is also supported — if this instance is
     * not the head, the chain is walked from head() via `previous` links
     * until the node whose `previous == this` is found, and that node is
     * spliced to skip over `this` (`node->previous = this->previous`). This
     * keeps the chain intact when instances of the same ObjectType die out
     * of construction order (e.g. plugin instances closing in a different
     * order than they were opened).
     */
    virtual ~Instance() noexcept
    {
        const std::scoped_lock chainLock { chainMutex() };

        if (head().load (std::memory_order_relaxed) == static_cast<ObjectType*> (this))
        {
            head().store (previous, std::memory_order_release);
        }
        else
        {
            ObjectType* node { head().load (std::memory_order_relaxed) };

            while (node != nullptr and node->previous != static_cast<ObjectType*> (this))
                node = node->previous;

            if (node != nullptr)
                node->previous = previous;
        }
    }

    /**
     * @brief Returns the top of the process-global instance chain.
     *
     * Follows LIFO semantics — the most recently constructed live instance of
     * ObjectType, visible from any thread. Lock-free atomic read. Returns
     * nullptr if no instance exists.
     *
     * @return Pointer to the active ObjectType instance, or nullptr.
     */
    static ObjectType* getInstance() noexcept { return head().load (std::memory_order_acquire); }

    /** @brief Copy construction deleted — Instance ownership is non-transferable. */
    Instance (const Instance&) = delete;

    /** @brief Copy assignment deleted — Instance ownership is non-transferable. */
    Instance& operator= (const Instance&) = delete;

    /** @brief Move construction deleted — Instance ownership is non-transferable. */
    Instance (Instance&&) = delete;

    /** @brief Move assignment deleted — Instance ownership is non-transferable. */
    Instance& operator= (Instance&&) = delete;

private:

    /**
     * @brief Pointer to the previous head at the time this instance was constructed.
     *
     * Forms the intrusive linked-list chain. Restored to head() on destruction
     * so the chain remains consistent across nested lifetimes.
     */
    ObjectType* previous { nullptr };

    /**
     * @brief Process-global singleton accessor for the chain head.
     *
     * Returns a reference to the atomic pointer that always holds the active
     * (most recently pushed) instance of ObjectType, shared across all
     * threads.
     *
     * @return Reference to the process-global head pointer.
     */
    static std::atomic<ObjectType*>& head() noexcept
    {
        static std::atomic<ObjectType*> h { nullptr };
        return h;
    }

    /**
     * @brief Mutex serializing chain push/pop on construction and destruction.
     *
     * @return Reference to the process-global chain mutex.
     */
    static std::mutex& chainMutex() noexcept
    {
        static std::mutex m;
        return m;
    }
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} // namespace jam

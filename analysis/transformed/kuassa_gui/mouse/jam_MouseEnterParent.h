/**
 * @file jam_MouseEnterParent.h
 * @brief Group-level mouse enter/exit notifications across a parent Component and its children.
 */
namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class MouseEnterParent
 * @brief Base for receiving group-level mouse enter/exit notifications across
 * a parent Component and all of its children.
 *
 * Attaches a recursive `juce::MouseListener` to the parent Component and
 * debounces raw per-child enter/exit events into a single mouseEnterParent()/
 * mouseExitParent() pair per group-level transition, via an async update.
 */
class MouseEnterParent
{
public:
    /**
     * @brief Constructs and attaches the group mouse listener to @p parentComponent.
     * @param parentComponent  Component (and descendants) to observe mouse enter/exit on.
     */
    explicit MouseEnterParent (juce::Component& parentComponent)
        : parent (parentComponent)
        , helper (*this)
        , mouseInside (false)
        , mouseInsideNext (false)
    {
    }

    virtual ~MouseEnterParent()
    {
    }

    /** @return `true` when the mouse is currently inside the parent Component or any of its children. */
    bool isMouseInsideParent() const noexcept
    {
        return mouseInside;
    }

    /**
     * @brief Called when the mouse enters the group.
     * @param e  Mouse event, relative to the parent Component.
     */
    virtual void mouseEnterParent (const juce::MouseEvent& e) = 0;

    /**
     * @brief Called when the mouse exits the group.
     * @param e  Mouse event, relative to the parent Component.
     */
    virtual void mouseExitParent (const juce::MouseEvent& e) = 0;

private:
    /** @brief Fires mouseEnterParent()/mouseExitParent() when the debounced state has changed. */
    void updateState()
    {
        if (mouseInside != mouseInsideNext)
        {
            if (mouseInsideNext)
            {
                mouseInside = true;
                mouseEnterParent (getMouseEvent());
            }
            else
            {
                mouseInside = false;
                mouseExitParent (getMouseEvent());
            }
        }
    }

private:
    /** @brief Stores the triggering mouse event by raw byte copy into the fixed-size buffer. */
    inline void setMouseEvent (const juce::MouseEvent& event)
    {
        memcpy (mouseEvent, &event, sizeof (event)); // HACK because of Juce
    }

    /** @return The most recently stored mouse event, reinterpreted from the raw byte buffer. */
    inline const juce::MouseEvent& getMouseEvent() const noexcept
    {
        return *reinterpret_cast<const juce::MouseEvent*> (mouseEvent); // HACK because of Juce
    }

    //==============================================================================
    /** @brief Recursive MouseListener attached to the parent; debounces raw per-child
     *  enter/exit events into a single async-updated group transition.
     */
    class Helper
        : private juce::MouseListener
        , private juce::AsyncUpdater
    {
    public:
        /** @brief Attaches this as a recursive mouse listener on @p parentOwner's parent Component. */
        explicit Helper (MouseEnterParent& parentOwner)
            : owner (parentOwner)
        {
            owner.parent.addMouseListener (this, true);
        }

        /** @brief Detaches this from the parent Component's mouse listeners. */
        ~Helper()
        {
            owner.parent.removeMouseListener (this);
        }

        /** @brief Records a pending enter transition and schedules updateState(). */
        void mouseEnter (const juce::MouseEvent& e)
        {
            owner.mouseInsideNext = true;
            owner.setMouseEvent (e.getEventRelativeTo (&owner.parent));
            triggerAsyncUpdate();
        }

        /** @brief Records a pending exit transition and schedules updateState(). */
        void mouseExit (const juce::MouseEvent& e)
        {
            owner.mouseInsideNext = false;
            owner.setMouseEvent (e.getEventRelativeTo (&owner.parent));
            triggerAsyncUpdate();
        }

        /** @brief Applies the debounced transition via MouseEnterParent::updateState(). */
        void handleAsyncUpdate()
        {
            owner.updateState();
        }

    private:
        MouseEnterParent& owner; ///< The owning MouseEnterParent instance.

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Helper)
    };

private:
    juce::Component& parent;                    ///< Component (and descendants) being observed.
    Helper helper;                               ///< Recursive mouse listener and debouncer.
    bool mouseInside;                            ///< Current (applied) inside/outside state.
    bool mouseInsideNext;                        ///< Pending state recorded by Helper, applied by updateState().
    char mouseEvent[sizeof (juce::MouseEvent)]; // HACK because of Juce!

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MouseEnterParent)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */

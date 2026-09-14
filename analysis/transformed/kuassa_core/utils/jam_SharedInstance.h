// SharedInstance<T> — create-or-join ref-counted holder for a process-wide
// shared ObjectType. First holder's args construct; later holders join and
// their own args are discarded; last drop destroys.

namespace jam
{
/*____________________________________________________________________________*/

// Default-constructed holder starts empty and joins/creates later via
// create(). The in_place_t constructor joins or creates immediately: the
// first live holder constructs ObjectType from perfect-forwarded args under
// the per-type create mutex and stores a weak reference; later holders lock
// that weak reference and join (their args are unused — deterministic
// construction across sites makes first-wins sound). Copy joins the same
// shared object; move transfers this holder's reference; the shared object
// is destroyed once the last holder releases it.
template <typename ObjectType>
class SharedInstance
{
public:

    SharedInstance() noexcept = default;

    // Joins the live shared object if one exists; otherwise constructs it
    // from the forwarded args (empty pack joins/creates via the default
    // constructor). The in_place_t tag disambiguates this constructor from
    // copy/move/default.
    template <typename... Args>
    explicit SharedInstance (std::in_place_t, Args&&... args)
    {
        create (std::forward<Args> (args)...);
    }

    SharedInstance (const SharedInstance&) = default;
    SharedInstance& operator= (const SharedInstance&) = default;
    SharedInstance (SharedInstance&&) noexcept = default;
    SharedInstance& operator= (SharedInstance&&) noexcept = default;
    ~SharedInstance() = default;

    // Joins the live shared object if one exists; otherwise constructs it
    // from the forwarded args and becomes the object every later join sees.
    // Idempotent once this holder already holds an object.
    template <typename... Args>
    void create (Args&&... args)
    {
        if (object == nullptr)
        {
            const std::scoped_lock createLock { createMutex() };

            object = weak().lock();

            if (object == nullptr)
            {
                object = std::make_shared<ObjectType> (std::forward<Args> (args)...);
                weak() = object;
            }
        }
    }

    ObjectType* get() const noexcept
    {
        assert (object != nullptr);
        return object.get();
    }

    static ObjectType* getInstance() noexcept
    {
        auto locked { weak().lock() };
        assert (locked != nullptr);
        return locked.get();
    }

    ObjectType* operator-> () const noexcept { return get(); }

    ObjectType& operator* () const noexcept
    {
        assert (object != nullptr);
        return *object;
    }

    explicit operator bool() const noexcept { return object != nullptr; }

private:

    std::shared_ptr<ObjectType> object {};

    static std::mutex& createMutex() noexcept
    {
        static std::mutex m;
        return m;
    }

    static std::weak_ptr<ObjectType>& weak() noexcept
    {
        static std::weak_ptr<ObjectType> w;
        return w;
    }
};

/**_____________________________END_OF_NAMESPACE______________________________*/
} // namespace jam

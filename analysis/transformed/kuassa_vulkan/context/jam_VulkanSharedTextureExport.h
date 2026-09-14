namespace jam
{

/** @brief RAII wrapper around an NT shared-texture export HANDLE
 *  (IDXGIResource1::CreateSharedHandle), closing it on destruction.
 *
 *  Move-only — a shared-texture handle has exactly one owner at a time;
 *  copying an open HANDLE would double-close it. */
class VulkanSharedTextureExport
{
public:
    /** Constructs an empty export holding no handle. */
    VulkanSharedTextureExport() = default;
    /** @param handle Shared-texture export handle to take ownership of. */
    explicit VulkanSharedTextureExport (HANDLE handle) noexcept : handle (handle) {}

    ~VulkanSharedTextureExport()
    {
        if (handle != nullptr)
            CloseHandle (handle);
    }

    VulkanSharedTextureExport (const VulkanSharedTextureExport&) = delete;
    VulkanSharedTextureExport& operator= (const VulkanSharedTextureExport&) = delete;

    VulkanSharedTextureExport (VulkanSharedTextureExport&& other) noexcept
        : handle (std::exchange (other.handle, nullptr))
    {
    }

    VulkanSharedTextureExport& operator= (VulkanSharedTextureExport&& other) noexcept
    {
        if (this != &other)
        {
            if (handle != nullptr)
                CloseHandle (handle);

            handle = std::exchange (other.handle, nullptr);
        }

        return *this;
    }

    /** @brief Releases any handle already owned, nulls it, and returns the
     *  now-empty slot's address — mirrors
     *  juce::ComSmartPtr::resetAndGetPointerAddress()'s own reset-then-fill
     *  contract, used identically throughout this file for the ComSmartPtr
     *  members below.
     *  @return Address of this object's own (now-null) handle slot, ready
     *          for an out-parameter call to fill. */
    HANDLE* resetAndGetPointerAddress() noexcept
    {
        if (handle != nullptr)
            CloseHandle (handle);

        handle = nullptr;

        return &handle;
    }

    /** @brief Transfers ownership of the held handle to the caller, nulling
     *  this object's own copy so the destructor never closes a handle
     *  already owned elsewhere.
     *  @return The handle previously held, or nullptr if none was held. */
    HANDLE releaseTextureExport() noexcept
    {
        return std::exchange (handle, nullptr);
    }

private:
    HANDLE handle { nullptr };
};

}// namespace jam

namespace jam
{
/*____________________________________________________________________________*/
/** @brief RAII owner of a vk::Image + vk::ImageView backed by a VmaAllocation.
 *
 *  Constructor creates both the image (via vmaCreateImage) and the view
 *  (via vk::Device::createImageView). Destructor destroys both in reverse order.
 *  Partial construction is safe — the destructor guards each handle.
 *
 *  The (general, non-factory) constructor below ALWAYS (never a
 *  conditional branch on mip count — uniform construction/teardown
 *  regardless of numMipLevels or caller) builds a second, level-0-only
 *  view (attachmentView, getAttachmentView()) alongside the primary
 *  full-mip-range sampling view (view, getView()) — a framebuffer
 *  attachment must reference exactly one mip level
 *  (VUID-VkFramebufferCreateInfo-pAttachments-00891), which a >1-level
 *  sampling view can no longer satisfy once VulkanShaderInstance::build() starts
 *  requesting real mip chains (mipmap_inputN sweep) — every
 *  VulkanImage-backed framebuffer attachment site in jam_vulkan binds
 *  getAttachmentView() instead of getView() from this sweep forward, even
 *  where numMipLevels stays 1 (uniform contract, not a per-call-site
 *  special case). create2D() delegates to this constructor, so it gets
 *  attachmentView for free. Uniformity is the contract, not caller
 *  knowledge of whether an attachment follows: VulkanGraphics::
 *  createCalibrationTargets() builds msaaColorImage through this general
 *  constructor (it needs a pre-creation vmaFindMemoryTypeIndexForImageInfo()
 *  probe, so it cannot use the create2D() factory) and binds it as a
 *  framebuffer attachment via getAttachmentView() — a general-ctor caller
 *  requiring attachmentView is not a hypothetical.
 *
 *  Move-only. Non-copyable.
 */
class VulkanImage
{
public:
    /** @brief Default constructor — produces an empty, invalid VulkanImage. */
    VulkanImage() = default;

    /** @brief Creates a vk::Image + VmaAllocation + vk::ImageView from the given create infos.
     *  @param newAllocator  VMA allocator — owns the image memory.
     *  @param newDevice     Logical device — owns the image view.
     *  @param imageInfo     vk::ImageCreateInfo for the image.
     *  @param allocInfo     VmaAllocationCreateInfo for the backing memory.
     *  @param viewInfo      vk::ImageViewCreateInfo for the view (image field is overwritten).
     */
    VulkanImage (VmaAllocator newAllocator, vk::Device newDevice,
           const vk::ImageCreateInfo& imageInfo,
           const VmaAllocationCreateInfo& allocInfo,
           const vk::ImageViewCreateInfo& viewInfo)
        : allocator (newAllocator), device (newDevice)
    {
        VkImage createdImage { VK_NULL_HANDLE };
        const vk::Result imageResult { static_cast<vk::Result> (vmaCreateImage (allocator, imageInfo, &allocInfo,
                                                                                &createdImage, &allocation, nullptr)) };
        image = createdImage;

        if (imageResult == vk::Result::eSuccess)
        {
            vk::ImageViewCreateInfo viewInfoWithImage { viewInfo };
            viewInfoWithImage.image = image;

            const vk::Result viewResult { device.createImageView (&viewInfoWithImage, nullptr, &view) };

            if (viewResult == vk::Result::eSuccess)
            {
                // ALWAYS built alongside the primary view (no aliasing branch —
                // see class doc comment): a single-level, level-0-only view for
                // framebuffer-attachment binding. Copies viewInfoWithImage so every
                // other field (baseMipLevel, aspect, format, viewType, ...) is
                // preserved verbatim for whichever caller supplied it.
                vk::ImageViewCreateInfo attachmentViewInfo { viewInfoWithImage };
                attachmentViewInfo.subresourceRange.levelCount = 1;

                const vk::Result attachmentViewResult { device.createImageView (
                    &attachmentViewInfo, nullptr, &attachmentView) };

                if (attachmentViewResult != vk::Result::eSuccess)
                {
                   #if JUCE_DEBUG
                    jam::debug::Log::write ("VulkanImage: attachment view creation failed: "
                                            + juce::String (vk::to_string (attachmentViewResult)));
                   #endif
                    release();
                }
            }
            else
            {
               #if JUCE_DEBUG
                jam::debug::Log::write ("VulkanImage: view creation failed: "
                                        + juce::String (vk::to_string (viewResult)));
               #endif
                release();
            }
        }
        else
        {
            jassertfalse;
        }
    }

    /** @brief Factory for the common 2D image + matching 2D view shape
     *  repeated across every render-target/scratch-image call site in jam_vulkan:
     *  imageType 2D, arrayLayers 1, OPTIMAL tiling, EXCLUSIVE sharing,
     *  VMA_MEMORY_USAGE_GPU_ONLY, viewType 2D, layerCount 1 — all
     *  invariant. Format/extent/samples/usage/aspect/numMipLevels vary across call sites.
     *  Sites whose vk::ImageCreateInfo/VmaAllocationCreateInfo must be inspected before
     *  construction (e.g. a pre-creation vmaFindMemoryTypeIndexForImageInfo() probe)
     *  cannot use this factory — it does not expose the intermediate structs.
     *  @param allocator     The VMA allocator that will own the backing memory.
     *  @param device        Logical device that will own the image views.
     *  @param format        Pixel format, shared by the image and both its views.
     *  @param extent        2D image extent (depth is fixed at 1).
     *  @param samples       MSAA sample count.
     *  @param usage         VulkanImage usage flags (attachment/sampled/transfer bits).
     *  @param aspect        View subresource aspect mask (color/stencil).
     *  @param numMipLevels  Mip levels the image itself is created with, and
     *                       view's own full-range subresourceRange.levelCount —
     *                       defaulted to 1 (every pre-existing call site's
     *                       exact prior behavior, unchanged). >1 only for a
     *                       VulkanShaderInstance buffer-pass target whose downstream
     *                       consumer pass declares mipmap_inputN
     *                       (VulkanShaderInstance::build(), jam_VulkanShaderInstance.cpp) —
     *                       VulkanGraphics::recordMipChainGeneration() then blits
     *                       level 0 down through level numMipLevels - 1 after
     *                       that pass's own draw. attachmentView (below) is
     *                       ALWAYS a single-level, level-0-only view regardless
     *                       of this value — built by the general constructor
     *                       this factory delegates to, see class doc comment.
     *  @returns           A constructed VulkanImage, or an invalid one on allocation failure.
     */
    static VulkanImage create2D (VmaAllocator allocator, vk::Device device, vk::Format format,
                           vk::Extent2D extent, vk::SampleCountFlagBits samples,
                           vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect,
                           uint32_t numMipLevels = 1)
    {
        const vk::ImageCreateInfo imageInfo { {}, vk::ImageType::e2D, format,
                                              vk::Extent3D { extent, 1 },
                                              numMipLevels, 1, samples, vk::ImageTiling::eOptimal, usage };

        VmaAllocationCreateInfo allocInfo {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        const vk::ImageViewCreateInfo viewInfo { {}, {}, vk::ImageViewType::e2D, format, {},
                                                 vk::ImageSubresourceRange { aspect, 0, numMipLevels, 0, 1 } };

        // attachmentView is built by the general constructor above (delegated
        // to here) — see class doc comment.
        return VulkanImage (allocator, device, imageInfo, allocInfo, viewInfo);
    }

   #if JUCE_WINDOWS
    static VulkanImage importD3D11 (vk::Device device, HANDLE textureExport, vk::Format format, vk::Extent2D extent)
    {
        VulkanImage image;
        image.device = device;

        const vk::ExternalMemoryImageCreateInfo externalInfo { vk::ExternalMemoryHandleTypeFlagBits::eD3D11Texture };

        const vk::ImageCreateInfo imageInfo { {}, vk::ImageType::e2D, format, vk::Extent3D { extent, 1 },
                                              1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                                              vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive,
                                              0, nullptr, vk::ImageLayout::eUndefined, &externalInfo };

        vk::Image createdImage {};
        const vk::Result imageResult { device.createImage (&imageInfo, nullptr, &createdImage) };
        jassert (imageResult == vk::Result::eSuccess);
        image.image = createdImage;

        if (imageResult != vk::Result::eSuccess)
            return image;

        const vk::ImageMemoryRequirementsInfo2 requirementsInfo { image.image };

        vk::MemoryDedicatedRequirements dedicatedRequirements {};
        vk::MemoryRequirements2 requirements2 { {}, &dedicatedRequirements };
        device.getImageMemoryRequirements2 (&requirementsInfo, &requirements2);

        jassert (dedicatedRequirements.requiresDedicatedAllocation == vk::True
                 or dedicatedRequirements.prefersDedicatedAllocation == vk::True);

        // vkGetMemoryWin32HandlePropertiesKHR is not resolvable through this
        // factory's own vk::Device — no vk::Instance is available here to
        // build the dynamic dispatcher VulkanDevice::createDebugMessenger() uses —
        // so it is resolved directly through vkGetDeviceProcAddr (a core,
        // statically-linked entry point) via vk::Device::getProcAddr().
        const auto getMemoryWin32HandleProperties {
            reinterpret_cast<PFN_vkGetMemoryWin32HandlePropertiesKHR> (
                device.getProcAddr ("vkGetMemoryWin32HandlePropertiesKHR")) };
        jassert (getMemoryWin32HandleProperties != nullptr);

        VkMemoryWin32HandlePropertiesKHR handlePropertiesRaw {};
        handlePropertiesRaw.sType = VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR;

        const VkResult handlePropertiesRawResult { getMemoryWin32HandleProperties (
            static_cast<VkDevice> (device), VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT,
            textureExport, &handlePropertiesRaw) };
        jassert (handlePropertiesRawResult == VK_SUCCESS);

        if (handlePropertiesRawResult != VK_SUCCESS)
        {
            image.release();
            return image;
        }

        const vk::MemoryWin32HandlePropertiesKHR handleProperties { handlePropertiesRaw };

        // VUID-VkMemoryAllocateInfo-memoryTypeIndex-00645 — the index must be
        // one reported by vkGetMemoryWin32HandlePropertiesKHR; the first set
        // bit is as valid a choice as any other reported bit.
        uint32_t memoryTypeIndex { 0 };
        while ((handleProperties.memoryTypeBits & (1u << memoryTypeIndex)) == 0)
            ++memoryTypeIndex;

        const vk::ImportMemoryWin32HandleInfoKHR importInfo {
            vk::ExternalMemoryHandleTypeFlagBits::eD3D11Texture, textureExport };

        const vk::MemoryDedicatedAllocateInfo dedicatedInfo { image.image, nullptr, &importInfo };

        // allocationSize is ignored by the driver for this handle type (OS-queried) —
        // supplied anyway from requirements2 for struct completeness.
        const vk::MemoryAllocateInfo allocateInfo {
            requirements2.memoryRequirements.size, memoryTypeIndex, &dedicatedInfo };

        // Attachment-only view — this image is eColorAttachment usage
        // only, never eSampled, so no sampling view is built (see class
        // doc comment for the attachmentView/view split).
        const vk::ImageViewCreateInfo viewInfo { {}, image.image, vk::ImageViewType::e2D, format, {},
                                                 vk::ImageSubresourceRange { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };

        vk::DeviceMemory allocatedMemory {};
        vk::Result result { device.allocateMemory (&allocateInfo, nullptr, &allocatedMemory) };

        // allocateMemory failed — importedMemory stays null (Vulkan leaves
        // pMemory unmodified on failure), so release() only needs to tear
        // down the image.
        image.importedMemory = allocatedMemory;

        if (result == vk::Result::eSuccess) result = device.bindImageMemory (image.image, image.importedMemory, 0);
        if (result == vk::Result::eSuccess) result = device.createImageView (&viewInfo, nullptr, &image.attachmentView);

        jassert (result == vk::Result::eSuccess);

        if (result != vk::Result::eSuccess)
            image.release();

        return image;
    }
   #endif

    /** @brief Destroys both views (attachmentView guarded identically to
     *  view — both are built by every constructor path, see class doc
     *  comment) then the image+allocation. */
    ~VulkanImage()
    {
        release();
    }

    VulkanImage (const VulkanImage&) = delete;
    VulkanImage& operator= (const VulkanImage&) = delete;

    /** @brief Move constructor — transfers ownership, nulls the source. */
    VulkanImage (VulkanImage&& other) noexcept
        : allocator (other.allocator), device (other.device),
          image (other.image), view (other.view), attachmentView (other.attachmentView),
          allocation (other.allocation)
         #if JUCE_WINDOWS
          , importedMemory (other.importedMemory)
         #endif
    {
        other.allocator     = VK_NULL_HANDLE;
        other.device        = vk::Device {};
        other.image         = vk::Image {};
        other.view          = vk::ImageView {};
        other.attachmentView = vk::ImageView {};
        other.allocation    = VK_NULL_HANDLE;
       #if JUCE_WINDOWS
        other.importedMemory = vk::DeviceMemory {};
       #endif
    }

    /** @brief Move assignment — destroys current state, then transfers ownership. */
    VulkanImage& operator= (VulkanImage&& other) noexcept
    {
        if (this != &other)
        {
            release();

            allocator      = other.allocator;
            device         = other.device;
            image          = other.image;
            view           = other.view;
            attachmentView = other.attachmentView;
            allocation     = other.allocation;

            other.allocator      = VK_NULL_HANDLE;
            other.device         = vk::Device {};
            other.image          = vk::Image {};
            other.view           = vk::ImageView {};
            other.attachmentView = vk::ImageView {};
            other.allocation     = VK_NULL_HANDLE;

           #if JUCE_WINDOWS
            importedMemory       = other.importedMemory;
            other.importedMemory = vk::DeviceMemory {};
           #endif
        }

        return *this;
    }

    /** @brief Returns the underlying vk::Image handle. */
    vk::Image getImage() const noexcept { return image; }

    /** @brief Returns the full-mip-range sampling view (subresourceRange.levelCount
     *  == the numMipLevels this image was created with, create2D()'s own
     *  default 1 for every pre-existing call site) — bind this for every
     *  texture/descriptor sampling read. Never a valid framebuffer attachment
     *  once numMipLevels > 1 (VUID-VkFramebufferCreateInfo-pAttachments-00891) —
     *  use getAttachmentView() for that instead. */
    vk::ImageView getView() const noexcept { return view; }

    /** @brief Returns the single-level, level-0-only view — the ONLY view
     *  valid to bind as a vk::FramebufferCreateInfo attachment
     *  (VUID-VkFramebufferCreateInfo-pAttachments-00891 forbids a >1-level
     *  subresourceRange there). Built ALWAYS by the general constructor
     *  (and therefore by create2D(), which delegates to it), even when this
     *  image's own numMipLevels is 1 (uniform contract, no aliasing branch,
     *  no caller-knowledge special case — see class doc comment). */
    vk::ImageView getAttachmentView() const noexcept { return attachmentView; }

    /** @brief Returns true when the image handle is non-null. */
    bool isValid() const noexcept { return static_cast<bool> (image); }

private:
    /** @brief Destroys every handle actually created (both views, then the
     *  image via its owning path — VMA on macOS/general construction, raw
     *  vk::Device calls for a Windows-imported image), then nulls them.
     *  Guarded on device validity — a default-constructed VulkanImage has
     *  no device and no work to do. Safe to call from the destructor, move
     *  assignment, and any factory failure path. */
    void release() noexcept
    {
        if (device != nullptr)
        {
            device.destroyImageView (view, nullptr);
            device.destroyImageView (attachmentView, nullptr);

           #if JUCE_WINDOWS
            if (allocator != VK_NULL_HANDLE)
            {
                vmaDestroyImage (allocator, image, allocation);
            }
            else
            {
                device.destroyImage (image, nullptr);
                device.freeMemory (importedMemory, nullptr);
            }
           #else
            vmaDestroyImage (allocator, image, allocation);
           #endif
        }

        image          = vk::Image {};
        view           = vk::ImageView {};
        attachmentView = vk::ImageView {};
        allocation     = VK_NULL_HANDLE;

       #if JUCE_WINDOWS
        importedMemory = vk::DeviceMemory {};
       #endif
    }

    /** @brief VMA allocator that owns the image memory. */
    VmaAllocator  allocator  { VK_NULL_HANDLE };

    /** @brief Logical device that owns both image views. */
    vk::Device    device     {};

    /** @brief The Vulkan image handle. */
    vk::Image     image      {};

    /** @brief The full-mip-range sampling view — see getView(). */
    vk::ImageView view       {};

    /** @brief The single-level, level-0-only attachment view — see
     *  getAttachmentView(). Built by every constructor path (general
     *  constructor and create2D(), which delegates to it). */
    vk::ImageView attachmentView {};

    /** @brief VMA allocation backing the image. */
    VmaAllocation allocation { VK_NULL_HANDLE };

   #if JUCE_WINDOWS
    vk::DeviceMemory importedMemory { VK_NULL_HANDLE };
   #endif
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
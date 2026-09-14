//
// GPU-mirror file: createAtlasImage(), getTexture() —
// the GPU-resident mirror (migrated from the since-deleted
// jam::TextureCache). Upload + the per-window bindless registry
// were extracted out of this file into jam::VulkanBindlessTexture (see
// jam_VulkanBindlessTexture.h) — this file now only decides per-type
// dimension/format and orchestrates BindlessTexture::create().

namespace jam
{
/*____________________________________________________________________________*/
bool GlyphAtlas::createAtlasImage (Type type, vk::Format format)
{
    jam::VulkanBindlessTexture texture;

    const bool created { texture.create (device, dimension, dimension, format,
                                         vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled) };

    if (created)
        gpuImages.emplace (type, std::move (texture));

    return created;
}

// getTexture() below is the only public accessor — valid once gpuImages
// holds both entries, which the constructor guarantees.

jam::VulkanBindlessTexture& GlyphAtlas::getTexture (Type type) noexcept { return gpuImages.at (type); }

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam

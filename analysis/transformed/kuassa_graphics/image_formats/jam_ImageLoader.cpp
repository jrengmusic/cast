/**
 * @file jam_ImageLoader.cpp
 * @brief SIMD-defiltering PNG decoder: memory buffer to premultiplied juce::Image.
 */

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Vendored zlib 1.2.x, compiled under its own namespace.
 *
 *  Isolates this TU's zlib instance from juce::zlibNamespace (JUCE's own
 *  vendored copy) and from FreeType's gzip zlib — three copies of the same
 *  C symbols coexist in one binary only if each is namespace-scoped; ODR
 *  would otherwise pick one copy at random across TUs. */
namespace zlib
{
#pragma push_macro ("register")
#define register

#pragma push_macro ("MIN")
#undef MIN

#pragma push_macro ("read")
#pragma push_macro ("write")
#pragma push_macro ("open")
#pragma push_macro ("close")

#undef OS_CODE
#undef fdopen
#define ZLIB_CONST 1
#define ZLIB_INTERNAL
#define NO_DUMMY_DECL
#include <juce_core/zip/zlib/adler32.c>
#include <juce_core/zip/zlib/compress.c>
#undef DO1
#undef DO8
#include <juce_core/zip/zlib/crc32.c>
#undef N
#include <juce_core/zip/zlib/deflate.c>
#include <juce_core/zip/zlib/inffast.c>
#undef PULLBYTE
#undef LOAD
#undef RESTORE
#undef INITBITS
#undef NEEDBITS
#undef DROPBITS
#undef BYTEBITS
#undef GZIP
#include <juce_core/zip/zlib/inflate.c>
#include <juce_core/zip/zlib/inftrees.c>
#include <juce_core/zip/zlib/trees.c>
#include <juce_core/zip/zlib/zutil.c>
#undef Byte
#undef fdopen
#undef local
#undef Freq
#undef Code
#undef Dad
#undef Len

#pragma pop_macro ("close")
#pragma pop_macro ("open")
#pragma pop_macro ("write")
#pragma pop_macro ("read")
#pragma pop_macro ("MIN")
#pragma pop_macro ("register")
}

/*____________________________________________________________________________*/

/** @brief Vendored libpng 1.6.37, compiled under its own namespace with the
 *  intel/arm SIMD defilter sources enabled.
 *
 *  Isolates this TU's libpng instance from juce::pnglibNamespace (JUCE's own
 *  vendored copy, compiled without the intel/arm defilter path) — same ODR
 *  concern as jam::zlib above. Links against jam::zlib rather than
 *  JUCE's or the system's zlib. */
namespace pnglib
{
using namespace zlib;

#undef check
#define NO_DUMMY_DECL
#define PNGLCONF_H 1

#ifndef Byte
using Byte = uint8_t;
#endif

#if JAM_SIMD_SSE2
#define PNG_INTEL_SSE
#define PNG_ARM_NEON_OPT 0
#elif JAM_SIMD_NEON
#define PNG_ARM_NEON_OPT 2
#define PNG_ARM_NEON_IMPLEMENTATION 1
#else
#define PNG_ARM_NEON_OPT 0
#endif

#define PNG_16BIT_SUPPORTED
#define PNG_ALIGNED_MEMORY_SUPPORTED
#define PNG_BENIGN_ERRORS_SUPPORTED
#define PNG_BENIGN_READ_ERRORS_SUPPORTED
#define PNG_CHECK_FOR_INVALID_INDEX_SUPPORTED
#define PNG_COLORSPACE_SUPPORTED
#define PNG_CONSOLE_IO_SUPPORTED
#define PNG_EASY_ACCESS_SUPPORTED
#define PNG_FIXED_POINT_SUPPORTED
#define PNG_FLOATING_ARITHMETIC_SUPPORTED
#define PNG_FLOATING_POINT_SUPPORTED
#define PNG_FORMAT_AFIRST_SUPPORTED
#define PNG_FORMAT_BGR_SUPPORTED
#define PNG_GAMMA_SUPPORTED
#define PNG_GET_PALETTE_MAX_SUPPORTED
#define PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#define PNG_INCH_CONVERSIONS_SUPPORTED
#define PNG_INFO_IMAGE_SUPPORTED
#define PNG_IO_STATE_SUPPORTED
#define PNG_POINTER_INDEXING_SUPPORTED
#define PNG_PROGRESSIVE_READ_SUPPORTED
#define PNG_READ_16BIT_SUPPORTED
#define PNG_READ_ALPHA_MODE_SUPPORTED
#define PNG_READ_ANCILLARY_CHUNKS_SUPPORTED
#define PNG_READ_BACKGROUND_SUPPORTED
#define PNG_READ_BGR_SUPPORTED
#define PNG_READ_CHECK_FOR_INVALID_INDEX_SUPPORTED
#define PNG_READ_COMPOSITE_NODIV_SUPPORTED
#define PNG_READ_COMPRESSED_TEXT_SUPPORTED
#define PNG_READ_EXPAND_16_SUPPORTED
#define PNG_READ_EXPAND_SUPPORTED
#define PNG_READ_FILLER_SUPPORTED
#define PNG_READ_GAMMA_SUPPORTED
#define PNG_READ_GET_PALETTE_MAX_SUPPORTED
#define PNG_READ_GRAY_TO_RGB_SUPPORTED
#define PNG_READ_INTERLACING_SUPPORTED
#define PNG_READ_INT_FUNCTIONS_SUPPORTED
#define PNG_READ_INVERT_ALPHA_SUPPORTED
#define PNG_READ_INVERT_SUPPORTED
#define PNG_READ_OPT_PLTE_SUPPORTED
#define PNG_READ_PACKSWAP_SUPPORTED
#define PNG_READ_PACK_SUPPORTED
#define PNG_READ_QUANTIZE_SUPPORTED
#define PNG_READ_RGB_TO_GRAY_SUPPORTED
#define PNG_READ_SCALE_16_TO_8_SUPPORTED
#define PNG_READ_SHIFT_SUPPORTED
#define PNG_READ_STRIP_16_TO_8_SUPPORTED
#define PNG_READ_STRIP_ALPHA_SUPPORTED
#define PNG_READ_SUPPORTED
#define PNG_READ_SWAP_ALPHA_SUPPORTED
#define PNG_READ_SWAP_SUPPORTED
#define PNG_READ_TEXT_SUPPORTED
#define PNG_READ_TRANSFORMS_SUPPORTED
#define PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
#define PNG_READ_USER_CHUNKS_SUPPORTED
#define PNG_READ_USER_TRANSFORM_SUPPORTED
#define PNG_READ_bKGD_SUPPORTED
#define PNG_READ_cHRM_SUPPORTED
#define PNG_READ_gAMA_SUPPORTED
#define PNG_READ_hIST_SUPPORTED
#define PNG_READ_iCCP_SUPPORTED
#define PNG_READ_iTXt_SUPPORTED
#define PNG_READ_oFFs_SUPPORTED
#define PNG_READ_pCAL_SUPPORTED
#define PNG_READ_pHYs_SUPPORTED
#define PNG_READ_sBIT_SUPPORTED
#define PNG_READ_sCAL_SUPPORTED
#define PNG_READ_sPLT_SUPPORTED
#define PNG_READ_sRGB_SUPPORTED
#define PNG_READ_tEXt_SUPPORTED
#define PNG_READ_tIME_SUPPORTED
#define PNG_READ_tRNS_SUPPORTED
#define PNG_READ_zTXt_SUPPORTED
#define PNG_SAVE_INT_32_SUPPORTED
#define PNG_SAVE_UNKNOWN_CHUNKS_SUPPORTED
#define PNG_SEQUENTIAL_READ_SUPPORTED
#define PNG_SET_CHUNK_CACHE_LIMIT_SUPPORTED
#define PNG_SET_CHUNK_MALLOC_LIMIT_SUPPORTED
#define PNG_SET_UNKNOWN_CHUNKS_SUPPORTED
#define PNG_SET_USER_LIMITS_SUPPORTED
#define PNG_SIMPLIFIED_READ_AFIRST_SUPPORTED
#define PNG_SIMPLIFIED_READ_BGR_SUPPORTED
#define PNG_SIMPLIFIED_WRITE_AFIRST_SUPPORTED
#define PNG_SIMPLIFIED_WRITE_BGR_SUPPORTED
#define PNG_STDIO_SUPPORTED
#define PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED
#define PNG_TEXT_SUPPORTED
#define PNG_TIME_RFC1123_SUPPORTED
#define PNG_UNKNOWN_CHUNKS_SUPPORTED
#define PNG_USER_CHUNKS_SUPPORTED
#define PNG_USER_LIMITS_SUPPORTED
#define PNG_USER_TRANSFORM_INFO_SUPPORTED
#define PNG_USER_TRANSFORM_PTR_SUPPORTED
#define PNG_WARNINGS_SUPPORTED
#define PNG_WRITE_16BIT_SUPPORTED
#define PNG_WRITE_ANCILLARY_CHUNKS_SUPPORTED
#define PNG_WRITE_BGR_SUPPORTED
#define PNG_WRITE_CHECK_FOR_INVALID_INDEX_SUPPORTED
#define PNG_WRITE_COMPRESSED_TEXT_SUPPORTED
#define PNG_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED
#define PNG_WRITE_FILLER_SUPPORTED
#define PNG_WRITE_FILTER_SUPPORTED
#define PNG_WRITE_FLUSH_SUPPORTED
#define PNG_WRITE_GET_PALETTE_MAX_SUPPORTED
#define PNG_WRITE_INTERLACING_SUPPORTED
#define PNG_WRITE_INT_FUNCTIONS_SUPPORTED
#define PNG_WRITE_INVERT_ALPHA_SUPPORTED
#define PNG_WRITE_INVERT_SUPPORTED
#define PNG_WRITE_OPTIMIZE_CMF_SUPPORTED
#define PNG_WRITE_PACKSWAP_SUPPORTED
#define PNG_WRITE_PACK_SUPPORTED
#define PNG_WRITE_SHIFT_SUPPORTED
#define PNG_WRITE_SUPPORTED
#define PNG_WRITE_SWAP_ALPHA_SUPPORTED
#define PNG_WRITE_SWAP_SUPPORTED
#define PNG_WRITE_TEXT_SUPPORTED
#define PNG_WRITE_TRANSFORMS_SUPPORTED
#define PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
#define PNG_WRITE_USER_TRANSFORM_SUPPORTED
#define PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
#define PNG_WRITE_bKGD_SUPPORTED
#define PNG_WRITE_cHRM_SUPPORTED
#define PNG_WRITE_gAMA_SUPPORTED
#define PNG_WRITE_hIST_SUPPORTED
#define PNG_WRITE_iCCP_SUPPORTED
#define PNG_WRITE_iTXt_SUPPORTED
#define PNG_WRITE_oFFs_SUPPORTED
#define PNG_WRITE_pCAL_SUPPORTED
#define PNG_WRITE_pHYs_SUPPORTED
#define PNG_WRITE_sBIT_SUPPORTED
#define PNG_WRITE_sCAL_SUPPORTED
#define PNG_WRITE_sPLT_SUPPORTED
#define PNG_WRITE_sRGB_SUPPORTED
#define PNG_WRITE_tEXt_SUPPORTED
#define PNG_WRITE_tIME_SUPPORTED
#define PNG_WRITE_tRNS_SUPPORTED
#define PNG_WRITE_zTXt_SUPPORTED
#define PNG_bKGD_SUPPORTED
#define PNG_cHRM_SUPPORTED
#define PNG_gAMA_SUPPORTED
#define PNG_hIST_SUPPORTED
#define PNG_iCCP_SUPPORTED
#define PNG_iTXt_SUPPORTED
#define PNG_oFFs_SUPPORTED
#define PNG_pCAL_SUPPORTED
#define PNG_pHYs_SUPPORTED
#define PNG_sBIT_SUPPORTED
#define PNG_sCAL_SUPPORTED
#define PNG_sPLT_SUPPORTED
#define PNG_sRGB_SUPPORTED
#define PNG_tEXt_SUPPORTED
#define PNG_tIME_SUPPORTED
#define PNG_tRNS_SUPPORTED
#define PNG_zTXt_SUPPORTED

#define PNG_STRING_COPYRIGHT "";
#define PNG_STRING_NEWLINE "\n"
#define PNG_LITERAL_SHARP 0x23
#define PNG_LITERAL_LEFT_SQUARE_BRACKET 0x5b
#define PNG_LITERAL_RIGHT_SQUARE_BRACKET 0x5d

#define PNG_API_RULE 0
#define PNG_CALLOC_SUPPORTED
#define PNG_COST_SHIFT 3
#define PNG_DEFAULT_READ_MACROS 1
#define PNG_GAMMA_THRESHOLD_FIXED 5000
#define PNG_IDAT_READ_SIZE PNG_ZBUF_SIZE
#define PNG_INFLATE_BUF_SIZE 1024
#define PNG_MAX_GAMMA_8 11
#define PNG_QUANTIZE_BLUE_BITS 5
#define PNG_QUANTIZE_GREEN_BITS 5
#define PNG_QUANTIZE_RED_BITS 5
#define PNG_TEXT_Z_DEFAULT_COMPRESSION (-1)
#define PNG_TEXT_Z_DEFAULT_STRATEGY 0
#define PNG_WEIGHT_SHIFT 8
#define PNG_ZBUF_SIZE 8192
#define PNG_Z_DEFAULT_COMPRESSION (-1)
#define PNG_Z_DEFAULT_NOFILTER_STRATEGY 0
#define PNG_Z_DEFAULT_STRATEGY 1
#define PNG_sCAL_PRECISION 5
#define PNG_sRGB_PROFILE_CHECKS 2

#define PNG_LINKAGE_API
#define PNG_LINKAGE_FUNCTION

#if ! defined (PNG_USER_WIDTH_MAX)
#define PNG_USER_WIDTH_MAX 1000000
#endif

#if ! defined (PNG_USER_HEIGHT_MAX)
#define PNG_USER_HEIGHT_MAX 1000000
#endif

#define png_debug(a, b)
#define png_debug1(a, b, c)
#define png_debug2(a, b, c, d)

#include <juce_graphics/image_formats/pnglib/png.h>
#include <juce_graphics/image_formats/pnglib/pngconf.h>

#define PNG_NO_EXTERN
#include <juce_graphics/image_formats/pnglib/png.c>
#include <juce_graphics/image_formats/pnglib/pngerror.c>
#include <juce_graphics/image_formats/pnglib/pngget.c>
#include <juce_graphics/image_formats/pnglib/pngmem.c>
#include <juce_graphics/image_formats/pnglib/pngread.c>
#include <juce_graphics/image_formats/pnglib/pngpread.c>
#include <juce_graphics/image_formats/pnglib/pngrio.c>

void png_do_expand_palette (png_row_infop, png_bytep, png_const_colorp, png_const_bytep, int);
void png_do_expand (png_row_infop, png_bytep, png_const_color_16p);
void png_do_chop (png_row_infop, png_bytep);
void png_do_quantize (png_row_infop, png_bytep, png_const_bytep, png_const_bytep);
void png_do_gray_to_rgb (png_row_infop, png_bytep);
void png_do_unshift (png_row_infop, png_bytep, png_const_color_8p);
void png_do_unpack (png_row_infop, png_bytep);
int png_do_rgb_to_gray (png_structrp, png_row_infop, png_bytep);
void png_do_compose (png_row_infop, png_bytep, png_structrp);
void png_do_gamma (png_row_infop, png_bytep, png_structrp);
void png_do_encode_alpha (png_row_infop, png_bytep, png_structrp);
void png_do_scale_16_to_8 (png_row_infop, png_bytep);
void png_do_expand_16 (png_row_infop, png_bytep);
void png_do_read_filler (png_row_infop, png_bytep, png_uint_32, png_uint_32);
void png_do_read_invert_alpha (png_row_infop, png_bytep);
void png_do_read_swap_alpha (png_row_infop, png_bytep);

#include <juce_graphics/image_formats/pnglib/pngrtran.c>
#include <juce_graphics/image_formats/pnglib/pngrutil.c>
#include <juce_graphics/image_formats/pnglib/pngset.c>
#include <juce_graphics/image_formats/pnglib/pngtrans.c>
#include <juce_graphics/image_formats/pnglib/pngwio.c>
#include <juce_graphics/image_formats/pnglib/pngwrite.c>
#include <juce_graphics/image_formats/pnglib/pngwtran.c>
#include <juce_graphics/image_formats/pnglib/pngwutil.c>

#if JAM_SIMD_SSE2
#include "pnglib/intel_init.c"
#include "pnglib/filter_sse2_intrinsics.c"
#elif JAM_SIMD_NEON
#include "pnglib/arm_init.c"
#include "pnglib/filter_neon_intrinsics.c"
#include "pnglib/palette_neon_intrinsics.c"
#endif
}

/*____________________________________________________________________________*/
struct PngMemoryReadState
{
    const uint8_t* data;
    size_t size;
    size_t position;
};

static void pngReadCallback (pnglib::png_structp png, pnglib::png_bytep outData, pnglib::png_size_t length)
{
    auto* state { static_cast<PngMemoryReadState*> (pnglib::png_get_io_ptr (png)) };

    std::memcpy (outData, state->data + state->position, length);
    state->position += length;
}

static void pngErrorCallback (pnglib::png_structp png, pnglib::png_const_charp)
{
    std::longjmp (*static_cast<std::jmp_buf*> (pnglib::png_get_error_ptr (png)), 1);
}

static void pngWarningCallback (pnglib::png_structp, pnglib::png_const_charp) {}

/** @brief Creates the libpng read/info structs, wires the longjmp-based error
 *  handler to @p errorJumpBuf, wires the memory-backed read callback, and
 *  reads the PNG header via png_read_info.
 *
 *  @param errorJumpBuf   Jump target invoked from pngErrorCallback on any libpng error.
 *  @param readState      Memory-read cursor consumed by pngReadCallback.
 *  @param outReadStruct  Receives the created png_structp.
 *  @param outInfoStruct  Receives the created png_infop. */
static void createPngReadState (std::jmp_buf& errorJumpBuf, PngMemoryReadState& readState,
                                 pnglib::png_structp& outReadStruct, pnglib::png_infop& outInfoStruct)
{
    using namespace pnglib;

    outReadStruct = png_create_read_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    jassert (outReadStruct != nullptr);

    outInfoStruct = png_create_info_struct (outReadStruct);
    jassert (outInfoStruct != nullptr);

    png_set_error_fn (outReadStruct, &errorJumpBuf, pngErrorCallback, pngWarningCallback);
    png_set_read_fn (outReadStruct, &readState, pngReadCallback);

    png_read_info (outReadStruct, outInfoStruct);
}

/** @brief Applies the JUCE-mirrored normalization transforms (16-to-8-bit
 *  strip, palette/sub-8-bit expand, gray-to-RGB, tRNS-to-alpha expand, alpha
 *  filler) and reports the resulting width, height, and alpha presence.
 *
 *  @param pngReadStruct       The libpng read struct, already past png_read_info.
 *  @param pngInfoStruct       The matching info struct.
 *  @param outWidth            Receives the image width, in pixels.
 *  @param outHeight           Receives the image height, in pixels.
 *  @param outHasAlphaChannel  Receives whether the source PNG carries alpha
 *                             (direct alpha channel or tRNS transparency chunk). */
static void applyPngTransforms (pnglib::png_structp pngReadStruct, pnglib::png_infop pngInfoStruct,
                                 pnglib::png_uint_32& outWidth, pnglib::png_uint_32& outHeight, bool& outHasAlphaChannel)
{
    using namespace pnglib;

    int bitDepth { 0 };
    int colorType { 0 };
    int interlaceType { 0 };

    png_get_IHDR (pngReadStruct, pngInfoStruct, &outWidth, &outHeight, &bitDepth, &colorType, &interlaceType, nullptr, nullptr);

    if (bitDepth == 16)
        png_set_strip_16 (pngReadStruct);

    if (colorType == PNG_COLOR_TYPE_PALETTE)
        png_set_expand (pngReadStruct);

    if (bitDepth < 8)
        png_set_expand (pngReadStruct);

    if (colorType == PNG_COLOR_TYPE_GRAY or colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb (pngReadStruct);

    const bool hasTransparencyChunk { png_get_valid (pngReadStruct, pngInfoStruct, PNG_INFO_tRNS) != 0 };

    if (hasTransparencyChunk)
        png_set_expand (pngReadStruct);

    png_set_add_alpha (pngReadStruct, 0xff, PNG_FILLER_AFTER);

    outHasAlphaChannel = (colorType & PNG_COLOR_MASK_ALPHA) != 0 or hasTransparencyChunk;
}

static void convertRow (const uint8_t* srcRow, uint8_t* destRow, int width, bool destHasAlphaChannel, int pixelStride)
{
    if (destHasAlphaChannel)
    {
        jam::simd::convertRgbaPremultiply (srcRow, reinterpret_cast<uint32_t*> (destRow), width);
    }
    else
    {
        for (int x { 0 }; x < width; ++x)
        {
            const uint8_t* srcPixel { srcRow + x * 4 };
            uint8_t* destPixel { destRow + x * pixelStride };

            destPixel[0] = srcPixel[2];
            destPixel[1] = srcPixel[1];
            destPixel[2] = srcPixel[0];
        }
    }
}

/** @brief Reads the defiltered PNG rows and converts them into a premultiplied
 *  juce::Image via jam::simd::convertRgbaPremultiply.
 *
 *  @param pngReadStruct     The libpng read struct, past all transform setup.
 *  @param pngInfoStruct     The matching info struct.
 *  @param width             Image width, in pixels.
 *  @param height            Image height, in pixels.
 *  @param hasAlphaChannel   Whether the source PNG carries alpha.
 *  @return The decoded image. */
static juce::Image readPngRowsToImage (pnglib::png_structp pngReadStruct, pnglib::png_infop pngInfoStruct,
                                        pnglib::png_uint_32 width, pnglib::png_uint_32 height, bool hasAlphaChannel)
{
    using namespace pnglib;

    const size_t rowStride { static_cast<size_t> (width) * 4 };
    const int interlaceType { png_get_interlace_type (pngReadStruct, pngInfoStruct) };

    juce::Image image (juce::SoftwareImageType().create (
        hasAlphaChannel ? juce::Image::ARGB : juce::Image::RGB,
        static_cast<int> (width), static_cast<int> (height), hasAlphaChannel));

    image.getProperties()->set ("originalImageHadAlpha", image.hasAlphaChannel());
    const bool destHasAlphaChannel { image.hasAlphaChannel() };

    const juce::Image::BitmapData destData (image, juce::Image::BitmapData::writeOnly);

    if (interlaceType == PNG_INTERLACE_NONE)
    {
        jam::Array<uint8_t> row;
        row.malloc (static_cast<int> (rowStride));

        png_set_interlace_handling (pngReadStruct);
        png_start_read_image (pngReadStruct);

        for (png_uint_32 y { 0 }; y < height; ++y)
        {
            png_read_row (pngReadStruct, row.getData(), nullptr);
            uint8_t* destRow { destData.getLinePointer (static_cast<int> (y)) };

            convertRow (row.getData(), destRow, static_cast<int> (width), destHasAlphaChannel, destData.pixelStride);
        }

        png_read_end (pngReadStruct, pngInfoStruct);
    }
    else
    {
        jam::Array<uint8_t> rowArena;
        rowArena.malloc (static_cast<int> (rowStride * height));

        jam::Array<png_bytep> rows;
        rows.malloc (static_cast<int> (height));

        for (png_uint_32 y { 0 }; y < height; ++y)
            rows.getData()[y] = rowArena.getData() + static_cast<size_t> (y) * rowStride;

        png_read_image (pngReadStruct, rows.getData());
        png_read_end (pngReadStruct, pngInfoStruct);

        for (png_uint_32 y { 0 }; y < height; ++y)
        {
            const uint8_t* srcRow { rowArena.getData() + static_cast<size_t> (y) * rowStride };
            uint8_t* destRow { destData.getLinePointer (static_cast<int> (y)) };

            convertRow (srcRow, destRow, static_cast<int> (width), destHasAlphaChannel, destData.pixelStride);
        }
    }

    return image;
}

/** @brief Decodes a memory-resident PNG into a premultiplied juce::Image.
 *
 *  Reads via jam::pnglib (SIMD row defilter on SSE2/NEON), then converts
 *  each defiltered RGBA row through jam::simd::convertRgbaPremultiply —
 *  the result is byte-identical to juce::PNGImageFormat's output (same
 *  premultiply rounding, same ARGB/RGB image-type selection by alpha
 *  presence), only decoded through this TU's isolated SIMD-enabled libpng.
 *
 *  @param data  Pointer to the encoded PNG bytes.
 *  @param size  Size of @p data, in bytes.
 *  @return The decoded image, or a null juce::Image on decode failure. */
juce::Image decodePng (const void* data, size_t size)
{
    using namespace pnglib;

    png_structp pngReadStruct { nullptr };
    png_infop pngInfoStruct { nullptr };
    std::jmp_buf errorJumpBuf;
    juce::Image image;

    if (setjmp (errorJumpBuf) == 0)
    {
        PngMemoryReadState readState { static_cast<const uint8_t*> (data), size, 0 };
        png_uint_32 width { 0 };
        png_uint_32 height { 0 };
        bool hasAlphaChannel { false };

        createPngReadState (errorJumpBuf, readState, pngReadStruct, pngInfoStruct);
        applyPngTransforms (pngReadStruct, pngInfoStruct, width, height, hasAlphaChannel);
        image = readPngRowsToImage (pngReadStruct, pngInfoStruct, width, height, hasAlphaChannel);
    }
    else
    {
        jassert (false);
    }

    png_destroy_read_struct (&pngReadStruct, &pngInfoStruct, nullptr);

    return image;
}

/*____________________________________________________________________________*/

juce::Image ImageLoader::getFromBinary (const juce::String& resourceFileName)
{
    using namespace BinaryData;

    Raw binary { resourceFileName };

    if (not binary.exists())
    {
#if JUCE_DEBUG
        debug::Log::write ("ImageLoader::getFromBinary missing resource=" + resourceFileName);
#endif
        jassertfalse;
    }

    return getInstance()->getOrRasterize (binary.data, static_cast<size_t> (binary.size));
}

/*____________________________________________________________________________*/

void ImageLoader::registerImages()
{
    BinaryData::forEachResource ([] (const void* data, int size)
    {
        auto* loader { ImageLoader::getInstance() };
        jassert (loader != nullptr);

        loader->decodeJobs.addJob ([loader, data, size]
        {
            loader->getOrRasterize (data, static_cast<size_t> (size));
        });
    });
}

juce::Image ImageLoader::getOrRasterize (const void* data, size_t size) noexcept
{
    {
        const juce::CriticalSection::ScopedLockType lock { imagesLock };

        if (images.contains (data))
            return images.at (data);
    }

    juce::Image decoded {};

    if (isPngSignature (data, static_cast<int> (size)))
        decoded = decodePng (data, size);
    else
        decoded = juce::ImageFileFormat::loadFrom (data, size);

    if (decoded.isValid())
    {
        const juce::CriticalSection::ScopedLockType lock { imagesLock };
        images.try_emplace (data, decoded);
        return images.at (data);
    }

    return decoded;
}

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */

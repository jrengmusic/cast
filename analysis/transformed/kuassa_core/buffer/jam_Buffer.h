/**
 * @file jam_Buffer.h
 * @brief Multi-channel flat contiguous storage template — the AudioBuffer of the jam domain.
 *
 * `jam::Buffer<ElementType>` owns a single SIMD-aligned `juce::HeapBlock<char, true>` allocation
 * laid out as:  channel 0 rows … channel 1 rows … channel N-1 rows.
 *
 * Storage model mirrors `juce::AudioBuffer`: one base pointer per channel, physical row computed
 * on access via byte-correct `rowAddress(ch, row)` (FAM-aware, scales by rowStrideBytes).  No per-row pointer arrays.
 * A stack-allocated fast path (`preallocatedChannelSpace`) handles up to 8 channels without
 * heap allocation for the pointer array itself.
 *
 * ElementType must be trivially copyable.  Per-channel ring addressing via head positions.  No FIFO, no opinions.
 */

#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/** @brief Detects FAM element types that define a nested FlexType alias. */
template <typename T, typename = void>
struct has_flex_type : std::false_type {};

template <typename T>
struct has_flex_type<T, std::void_t<typename T::FlexType>> : std::true_type {};

template <typename T>
inline constexpr bool hasFlexType = has_flex_type<T>::value;

/**
 * @class Buffer
 * @brief Multi-channel flat, SIMD-aligned 2-D storage block with per-channel ring addressing.
 *
 * Mirrors the juce::AudioBuffer<ElementType> API extended to a 2-D (row × col) domain.
 * Storage is a single `juce::HeapBlock<char, true>` allocation; all channels are contiguous:
 * channel 0 occupies rows [0, numRows), channel 1 occupies rows [numRows, 2*numRows), etc.
 *
 * Access model: `channels[ch]` is the base pointer for channel ch.  A logical (ch, row) pair
 * maps to the physical element pointer via `rowAddress(ch, row)`, which uses byte arithmetic
 * (scales by rowStrideBytes) and is correct for both flat and FAM ElementTypes.
 * No per-row pointer array is maintained.
 *
 * Ring addressing: each channel has an independent head position stored in `headPositions`
 * (default 0).  Logical row → physical row: `(headPositions[channel] + row) % numRows`.
 * All accessors and clear methods accept logical row indices; physicalRow() maps internally.
 * advanceHead / reverseHead rotate the ring forward or backward.
 * Backward compatible: head defaults to 0, so (0 + row) % numRows == row for row < numRows —
 * existing callers observe no behaviour change.
 *
 * @tparam ElementType  Element type.  Must be trivially copyable.
 */
template <typename ElementType>
class Buffer
{
public:
    static_assert (std::is_trivially_copyable_v<ElementType>,
                   "jam::Buffer<ElementType> requires ElementType to be trivially copyable");

    //==========================================================================
    // Construction
    //==========================================================================

    /** Default constructor — empty buffer, no allocation. */
    Buffer() noexcept = default;

    ~Buffer() noexcept = default;

    //==========================================================================
    // Capacity
    //==========================================================================

    /**
     * @brief Allocate or resize the multi-channel buffer.
     *
     * Mirrors juce::AudioBuffer::setSize with an added numChannels parameter.
     * Per-channel data is laid out contiguously: channel 0 rows first, then channel 1, etc.
     * numCols = number of ElementType columns per row.
     * Row stride = alignedCols * sizeof(ElementType) for flat types,
     * sizeof(ElementType) + alignedCols * sizeof(FlexType) for FAM types.
     *
     * @param newNumChannels  Target channel count.
     * @param newNumRows      Target row count per channel.
     * @param newNumCols      Target column count per row.
     */
    void setSize (int newNumChannels, int newNumRows, int newNumCols) noexcept
    {
        jassert (newNumChannels >= 0 and newNumRows >= 0 and newNumCols >= 0);
        jassert (newNumChannels <= maxPreallocatedChannels);


        int newAlignedCols { 0 };
        size_t newRowStride { 0 };

        if constexpr (hasFlexType<ElementType>)
        {
            constexpr int alignment { juce::jmax (1, 64 / static_cast<int> (sizeof (typename ElementType::FlexType))) };
            newAlignedCols = (newNumCols + alignment - 1) & ~(alignment - 1);
            newRowStride = sizeof (ElementType)
                         + static_cast<size_t> (newAlignedCols) * sizeof (typename ElementType::FlexType);
        }
        else
        {
            constexpr int alignment { juce::jmax (1, 64 / static_cast<int> (sizeof (ElementType))) };
            newAlignedCols = (newNumCols + alignment - 1) & ~(alignment - 1);
            newRowStride = static_cast<size_t> (newAlignedCols) * sizeof (ElementType);
        }

        const size_t totalBytes { static_cast<size_t> (newNumChannels * newNumRows) * newRowStride };
        const bool hasContent { allocation.getData() != nullptr };
        const bool needsRealloc { totalBytes > allocatedBytes };

        if (needsRealloc)
        {
            juce::HeapBlock<char, true> newAllocation;
            newAllocation.allocate (totalBytes, true);
            char* newData { newAllocation.getData() };

            if (hasContent)
                copyExistingContent (
                    allocation.getData(), newData, newRowStride, newNumChannels, newNumRows);

            allocation = std::move (newAllocation);
            allocatedBytes = totalBytes;
        }

        numChannels  = newNumChannels;
        numRows      = newNumRows;
        numCols      = newNumCols;
        alignedCols  = newAlignedCols;
        rowStrideBytes = newRowStride;

        // Point channels[] at the preallocated stack space (jassert above guards overflow).
        channels = preallocatedChannelSpace;

        // Set per-channel base pointers into the flat allocation.
        char* base { allocation.getData() };

        for (int ch { 0 }; ch < newNumChannels; ++ch)
        {
            const size_t channelOffset { static_cast<size_t> (ch * newNumRows) * newRowStride };
            channels[ch] = base + channelOffset;
        }

        // Per-channel ring head positions — reset only on first allocation;
        // preserve on resize. Clamp when row count shrinks below current head.
        if (hasContent)
        {
            for (int ch { 0 }; ch < newNumChannels; ++ch)
            {
                if (headPositions[ch] >= newNumRows)
                    headPositions[ch] = 0;
            }
        }
        else
        {
            std::fill_n (headPositions, newNumChannels, 0);
        }

        // Single clear flag — reset to not-hasContent on resize.
        isClear = not hasContent;
    }

    //==========================================================================
    // Accessors
    //==========================================================================

    /** @return Number of channels in the buffer. */
    int getNumChannels() const noexcept { return numChannels; }

    /** @return Number of rows per channel. */
    int getNumRows() const noexcept { return numRows; }

    /** @return Number of columns per row. */
    int getNumCols() const noexcept { return numCols; }

    /**
     * @brief Read pointer to the start of a row on a channel.
     * @param channel  Zero-based channel index.
     * @param row      Zero-based row index.
     */
    const ElementType* getReadPointer (int channel, int row) const noexcept
    {
        jassert (channel >= 0 and channel < numChannels);
        jassert (row >= 0 and row < numRows);
        return rowAddress (channel, row);
    }

    /**
     * @brief Read pointer to a specific element on a channel.
     *
     * col is an ElementType column index; returned pointer = row_start + col.
     *
     * @param channel  Zero-based channel index.
     * @param row      Zero-based row index.
     * @param col      Zero-based column index.
     */
    const ElementType* getReadPointer (int channel, int row, int col) const noexcept
    {
        static_assert (not hasFlexType<ElementType>,
                       "Column-indexed access is not valid for FAM types — use getReadPointer(ch, row)->chars[col]");
        jassert (channel >= 0 and channel < numChannels);
        jassert (row >= 0 and row < numRows);
        jassert (col >= 0 and col < numCols);
        return rowAddress (channel, row) + col;
    }

    /**
     * @brief Write pointer to the start of a row on a channel.  Clears isClear.
     * @param channel  Zero-based channel index.
     * @param row      Zero-based row index.
     */
    ElementType* getWritePointer (int channel, int row) noexcept
    {
        jassert (channel >= 0 and channel < numChannels);
        jassert (row >= 0 and row < numRows);
        isClear = false;
        return rowAddress (channel, row);
    }

    /**
     * @brief Write pointer to a specific element on a channel.  Clears isClear.
     *
     * col is an ElementType column index; returned pointer = row_start + col.
     *
     * @param channel  Zero-based channel index.
     * @param row      Zero-based row index.
     * @param col      Zero-based column index.
     */
    ElementType* getWritePointer (int channel, int row, int col) noexcept
    {
        static_assert (not hasFlexType<ElementType>,
                       "Column-indexed access is not valid for FAM types — use getWritePointer(ch, row)->chars[col]");
        jassert (channel >= 0 and channel < numChannels);
        jassert (row >= 0 and row < numRows);
        jassert (col >= 0 and col < numCols);
        isClear = false;
        return rowAddress (channel, row) + col;
    }

    /**
     * @brief Base pointer for a channel — first element of the channel's flat storage.
     *
     * Equivalent to `juce::AudioBuffer::getReadPointer(channel)`.
     * The channel's rows are accessed via `rowAddress(ch, row)` (byte-correct, FAM-aware).
     * Pointer is stable for the current allocation; invalidated by setSize() realloc.
     *
     * @param channel  Zero-based channel index.
     * @return Pointer to the first element of the channel's contiguous storage.
     */
    const ElementType* getChannelPointer (int channel) const noexcept
    {
        jassert (channel >= 0 and channel < numChannels);
        return static_cast<const ElementType*> (static_cast<const void*> (channels[channel]));
    }

    //==========================================================================
    // Ring addressing
    //==========================================================================

    /** Advance the head position for a channel by count rows.
     *  Wraps at numRows. Equivalent to ring rotation forward. */
    void advanceHead (int channel, int count) noexcept
    {
        jassert (channel >= 0 and channel < numChannels);
        jassert (numRows > 0);
        headPositions[channel] = (headPositions[channel] + count) % numRows;
    }

    /** Reverse the head position for a channel by count rows.
     *  Wraps at numRows. Equivalent to ring rotation backward. */
    void reverseHead (int channel, int count) noexcept
    {
        jassert (channel >= 0 and channel < numChannels);
        jassert (numRows > 0);
        headPositions[channel] = (headPositions[channel] - count + numRows) % numRows;
    }

    /** Returns the current head position for a channel. */
    int getHead (int channel) const noexcept
    {
        jassert (channel >= 0 and channel < numChannels);
        return headPositions[channel];
    }

    /** Sets the head position for a channel directly. */
    void setHead (int channel, int position) noexcept
    {
        jassert (channel >= 0 and channel < numChannels);
        jassert (numRows > 0);
        headPositions[channel] = position % numRows;
    }

    //==========================================================================
    // Clear
    //==========================================================================

    /** Zero all channels. Early-outs when buffer is already clear. */
    void clear() noexcept
    {
        if (not isClear)
        {
            for (int ch { 0 }; ch < numChannels; ++ch)
                clearChannelRows (ch);

            isClear = true;
        }
    }

    /**
     * @brief Zero all rows of one channel.
     *
     * Zeroes rowStrideBytes per row.  Sets isClear only when this is the sole channel —
     * with a single buffer-wide flag and no per-channel tracking, multi-channel partial
     * clears cannot be reflected here.  Callers clearing all channels should use clear().
     *
     * @param channel  Zero-based channel index.
     */
    void clear (int channel) noexcept
    {
        jassert (channel >= 0 and channel < numChannels);
        clearChannelRows (channel);
        isClear = (numChannels == 1);
    }

    /**
     * @brief Zero a single row of a channel.
     *
     * Zeroes rowStrideBytes.
     *
     * @param channel  Zero-based channel index.
     * @param row      Zero-based row index.
     */
    void clear (int channel, int row) noexcept
    {
        jassert (channel >= 0 and channel < numChannels);
        jassert (row >= 0 and row < numRows);
        std::memset (rowAddress (channel, row), 0, rowStrideBytes);
    }

    /**
     * @brief Zero a column range within a row of a channel.
     *
     * @param channel         Zero-based channel index.
     * @param row             Zero-based row index.
     * @param startCol        First ElementType column to clear (inclusive).
     * @param numColsToClear  Number of ElementType columns to clear.
     */
    void clear (int channel, int row, int startCol, int numColsToClear) noexcept
    {
        static_assert (not hasFlexType<ElementType>,
                       "Column-indexed access is not valid for FAM types — use getWritePointer(ch, row)->chars[col]");
        jassert (channel >= 0 and channel < numChannels);
        jassert (row >= 0 and row < numRows);
        jassert (startCol >= 0 and startCol + numColsToClear <= numCols);

        std::memset (rowAddress (channel, row) + startCol,
                     0,
                     static_cast<size_t> (numColsToClear) * sizeof (ElementType));
    }

    //==========================================================================
    // Copy
    //==========================================================================

    /**
     * @brief Copies one row from another Buffer (channel-aware).
     *
     * Like AudioBuffer::copyFrom (destChannel, destStart, source, srcChannel, srcStart, numSamples).
     * Copies min(rowStrideBytes, source.rowStrideBytes) bytes — safe for differing col counts.
     *
     * @param destChannel    Destination channel in this buffer.
     * @param destRow        Destination row in this buffer.
     * @param source         Source buffer (may be this buffer if rows don't overlap).
     * @param sourceChannel  Source channel.
     * @param sourceRow      Source row.
     */
    void copyFrom (int destChannel, int destRow, const Buffer& source, int sourceChannel, int sourceRow) noexcept
    {
        jassert (destChannel >= 0 and destChannel < numChannels);
        jassert (destRow >= 0 and destRow < numRows);
        jassert (sourceChannel >= 0 and sourceChannel < source.numChannels);
        jassert (sourceRow >= 0 and sourceRow < source.numRows);

        const size_t copyBytes { juce::jmin (rowStrideBytes, source.rowStrideBytes) };
        std::memcpy (
            rowAddress (destChannel, destRow),
            source.rowAddress (sourceChannel, sourceRow),
            copyBytes);
        isClear = false;
    }

    /**
     * @brief Copies one row from a raw pointer into a specific channel.
     *
     * @param destChannel   Destination channel.
     * @param destRow       Destination row in this buffer.
     * @param source        Source pointer.
     * @param numElements   Number of ElementType elements to copy (clamped to numCols).
     */
    void copyFrom (int destChannel, int destRow, const ElementType* source, int numElements) noexcept
    {
        static_assert (not hasFlexType<ElementType>,
                       "Column-indexed access is not valid for FAM types — use getWritePointer(ch, row)->chars[col]");
        jassert (destChannel >= 0 and destChannel < numChannels);
        jassert (destRow >= 0 and destRow < numRows);
        jassert (source != nullptr);

        const int count { juce::jmin (numElements, numCols) };
        std::memcpy (rowAddress (destChannel, destRow),
                     source,
                     static_cast<size_t> (count) * sizeof (ElementType));
        isClear = false;
    }

    //==========================================================================
    // State query
    //==========================================================================

    /**
     * @brief Returns true when the buffer has been fully cleared and not written since.
     *
     * Single flag — any write via getWritePointer or copyFrom sets this to false.
     * Cleared by calling clear() on all channels (or the no-arg overload).
     */
    bool hasBeenCleared() const noexcept
    {
        return isClear;
    }

    /** @return Stride between rows in elements (>= numCols, SIMD-aligned). */
    int getAlignedCols() const noexcept { return alignedCols; }

    /** @return Row stride in bytes (FAM-aware). */
    size_t getRowStrideBytes() const noexcept { return rowStrideBytes; }

    /** @return Raw pointer to the first element of the allocation. */
    const ElementType* getData() const noexcept
    {
        return static_cast<const ElementType*> (static_cast<const void*> (allocation.getData()));
    }

private:
    //==========================================================================
    // Ring helper
    //==========================================================================

    int physicalRow (int channel, int row) const noexcept
    {
        return (headPositions[channel] + row) % numRows;
    }

    /** @brief Byte-correct row address for any ElementType (FAM or flat). */
    const ElementType* rowAddress (int channel, int row) const noexcept
    {
        return static_cast<const ElementType*> (
            static_cast<const void*> (channels[channel] + static_cast<size_t> (physicalRow (channel, row)) * rowStrideBytes));
    }

    ElementType* rowAddress (int channel, int row) noexcept
    {
        return const_cast<ElementType*> (std::as_const (*this).rowAddress (channel, row));
    }

    //==========================================================================
    // Clear helper
    //==========================================================================

    void clearChannelRows (int channel) noexcept
    {
        for (int r { 0 }; r < numRows; ++r)
            std::memset (channels[channel] + static_cast<size_t> (r) * rowStrideBytes, 0, rowStrideBytes);
    }

    //==========================================================================
    // Content migration helper
    //==========================================================================

    void copyExistingContent (char* srcBase,
                              char* destBase,
                              size_t newRowStride,
                              int newNumChannels,
                              int newNumRows) noexcept
    {
        const int chansToCopy { juce::jmin (numChannels, newNumChannels) };
        const int rowsToCopy  { juce::jmin (numRows, newNumRows) };
        const size_t copyBytes { juce::jmin (rowStrideBytes, newRowStride) };

        for (int ch { 0 }; ch < chansToCopy; ++ch)
        {
            for (int r { 0 }; r < rowsToCopy; ++r)
            {
                const size_t srcOffset  { static_cast<size_t> (ch * numRows + r) * rowStrideBytes };
                const size_t destOffset { static_cast<size_t> (ch * newNumRows + r) * newRowStride };
                std::memcpy (destBase + destOffset, srcBase + srcOffset, copyBytes);
            }
        }
    }

    //==========================================================================
    // Storage
    //==========================================================================

public:
    static constexpr int maxPreallocatedChannels { 8 };
private:

    juce::HeapBlock<char, true> allocation;
    char*  preallocatedChannelSpace[maxPreallocatedChannels] {};       ///< Stack-allocated channel base pointers (avoids heap for <= 8 channels).
    char** channels { nullptr };                                       ///< [ch] → base pointer for channel ch's flat storage.
    int    headPositions[maxPreallocatedChannels] {};                  ///< Per-channel ring head position (default 0).
    bool   isClear { true };                                           ///< True when no write has occurred since last full clear.
    size_t rowStrideBytes { 0 };                                       ///< Bytes per row (aligned elements).
    size_t        allocatedBytes { 0 };                                ///< Total bytes in current allocation.
    int           numChannels { 0 };
    int           numRows { 0 };
    int           numCols { 0 };
    int           alignedCols { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Buffer)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam

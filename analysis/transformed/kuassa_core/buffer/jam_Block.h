/**
 * @file jam_Block.h
 * @brief Non-owning view into a `jam::Buffer<ElementType>` — the AudioBlock of the jam domain.
 *
 * `jam::Block<ElementType>` is a zero-cost, trivially copyable view into a region of a
 * `jam::Buffer`.  It stores a channel base pointer, the head position at capture time,
 * row extents, strideBytes, and ring size; no allocation occurs and no per-row pointer array
 * is maintained.
 *
 * Ring mapping is deferred to getRowPointer() / getWritePointer():
 * `(head + startRow + row) % ringSize` gives the physical row; the element address is
 * computed via byte arithmetic:
 * `static_cast<ElementType*>(static_cast<void*>(base + physical * strideBytes))`.
 * Valid only for the lifetime of the source Buffer and while its size is unchanged.
 *
 * Pattern mirrors `juce::dsp::AudioBlock` extended to a 2-D ring-aware domain.
 * Mutable access is provided via non-const Buffer constructors and getWritePointer().
 * Block does not manage head.  Head is received at construction time and used for
 * ring mapping.  Head state is managed by the Buffer owner.
 */

#pragma once

#include <cstring>

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @class Block
 * @brief Non-owning, zero-cost, ring-aware view into a `jam::Buffer<ElementType>` channel region.
 *
 * Trivially copyable and assignable — copy semantics produce an alias, not a deep copy.
 * Row access:
 *   - `getRowPointer(row)` — const read, applies ring mapping.
 *   - `getWritePointer(row)` — mutable write, applies ring mapping.
 * Block does not manage head.  Head is set at construction time and used purely for
 * ring-address mapping.  Head state lives in the State machine (WriteHead Parameter).
 *
 * Lifetime is bounded by the source Buffer; caller is responsible for not outliving it or
 * triggering a resize.
 *
 * @tparam ElementType  Element type.  Must be trivially copyable.
 */
template <typename ElementType>
class Block
{
public:
    static_assert (std::is_trivially_copyable_v<ElementType>,
                   "jam::Block<ElementType> requires ElementType to be trivially copyable");

    //==========================================================================
    // Construction
    //==========================================================================

    /** Default — empty view. `isEmpty()` returns true. */
    Block() noexcept = default;

    /**
     * @brief Full-channel read-only view.
     * @param buffer   Source buffer (const).  Must outlive this Block.
     * @param channel  Channel index to view.
     */
    Block (const Buffer<ElementType>& buffer, int channel) noexcept
        : base        { static_cast<char*> (static_cast<void*> (const_cast<ElementType*> (buffer.getChannelPointer (channel)))) }
        , head        { buffer.getHead (channel) }
        , numRows     { buffer.getNumRows() }
        , numCols     { buffer.getNumCols() }
        , strideBytes { buffer.getRowStrideBytes() }
        , ringSize    { buffer.getNumRows() }
    {
        jassert (channel >= 0 and channel < buffer.getNumChannels());
    }

    /**
     * @brief Sub-region read-only view of a specific channel.
     * @param buffer     Source buffer (const).  Must outlive this Block.
     * @param channel    Channel index to view.
     * @param rowOffset  First logical row of the view (zero-based, within the channel).
     * @param rowCount   Number of rows in the view.
     */
    Block (const Buffer<ElementType>& buffer, int channel, int rowOffset, int rowCount) noexcept
        : base        { static_cast<char*> (static_cast<void*> (const_cast<ElementType*> (buffer.getChannelPointer (channel)))) }
        , head        { buffer.getHead (channel) }
        , startRow    { rowOffset }
        , numRows     { rowCount }
        , numCols     { buffer.getNumCols() }
        , strideBytes { buffer.getRowStrideBytes() }
        , ringSize    { buffer.getNumRows() }
    {
        jassert (channel >= 0 and channel < buffer.getNumChannels());
        jassert (rowOffset >= 0 and rowCount >= 0);
    }

    /**
     * @brief Full-channel mutable view.
     * @param buffer   Source buffer (non-const).  Must outlive this Block.
     * @param channel  Channel index to view.
     */
    Block (Buffer<ElementType>& buffer, int channel) noexcept
        : base        { static_cast<char*> (static_cast<void*> (const_cast<ElementType*> (buffer.getChannelPointer (channel)))) }
        , head        { buffer.getHead (channel) }
        , numRows     { buffer.getNumRows() }
        , numCols     { buffer.getNumCols() }
        , strideBytes { buffer.getRowStrideBytes() }
        , ringSize    { buffer.getNumRows() }
    {
        jassert (channel >= 0 and channel < buffer.getNumChannels());
    }

    /**
     * @brief Sub-region mutable view of a specific channel.
     * @param buffer     Source buffer (non-const).  Must outlive this Block.
     * @param channel    Channel index to view.
     * @param rowOffset  First logical row of the view (zero-based, within the channel).
     * @param rowCount   Number of rows in the view.
     */
    Block (Buffer<ElementType>& buffer, int channel, int rowOffset, int rowCount) noexcept
        : base        { static_cast<char*> (static_cast<void*> (const_cast<ElementType*> (buffer.getChannelPointer (channel)))) }
        , head        { buffer.getHead (channel) }
        , startRow    { rowOffset }
        , numRows     { rowCount }
        , numCols     { buffer.getNumCols() }
        , strideBytes { buffer.getRowStrideBytes() }
        , ringSize    { buffer.getNumRows() }
    {
        jassert (channel >= 0 and channel < buffer.getNumChannels());
        jassert (rowOffset >= 0 and rowCount >= 0);
    }

    /**
     * @brief Ring-aware constructor — base pointer + ring metadata.
     *
     * For constructing a view from raw ring state (e.g. DST snapshot).
     * All ring fields are captured at construction time; getRowPointer() and
     * getWritePointer() apply the mapping on access.
     *
     * @param channelBase    Base pointer for the channel's flat storage.
     * @param headPosition   Ring head at capture time.
     * @param rowOffset      First logical row of the view.
     * @param rowCount       Number of rows in the view.
     * @param colCount       Number of columns per row.
     * @param rowStride      Row stride in bytes (FAM-aware — full physical row size).
     * @param ringSizeRows   Total rows in the source buffer (modulo operand).
     */
    Block (const ElementType* channelBase,
           int headPosition,
           int rowOffset,
           int rowCount,
           int colCount,
           size_t rowStride,
           int ringSizeRows) noexcept
        : base        { static_cast<char*> (static_cast<void*> (const_cast<ElementType*> (channelBase))) }
        , head        { headPosition }
        , startRow    { rowOffset }
        , numRows     { rowCount }
        , numCols     { colCount }
        , strideBytes { rowStride }
        , ringSize    { ringSizeRows }
    {
        jassert (channelBase != nullptr or rowCount == 0);
    }

    //==========================================================================
    // Accessors — read
    //==========================================================================

    /** @return Number of rows in this view. */
    int getNumRows() const noexcept { return numRows; }

    /** @return Number of columns per row in this view. */
    int getNumCols() const noexcept { return numCols; }

    /** @return Ring head position stored in this view (as captured at construction time). */
    int getHead() const noexcept { return head; }

    /**
     * @brief Const pointer to the start of a row within this view.
     *
     * Applies ring mapping: physical row = (head + startRow + row) % ringSize.
     *
     * @param row  Zero-based row index relative to this Block's origin.
     */
    const ElementType* getRowPointer (int row) const noexcept
    {
        jassert (row >= 0 and row < numRows);
        jassert (ringSize > 0);
        const int physical { (head + startRow + row) % ringSize };
        return static_cast<const ElementType*> (
            static_cast<const void*> (base + static_cast<size_t> (physical) * strideBytes));
    }

    /** @return `true` when this view covers no elements. */
    bool isEmpty() const noexcept { return numRows == 0 or numCols == 0; }

    //==========================================================================
    // Accessors — write
    //==========================================================================

    /**
     * @brief Mutable pointer to the start of a row within this view.
     *
     * Applies ring mapping: physical row = (head + startRow + row) % ringSize.
     * Caller must have constructed this Block from a non-const Buffer or raw mutable pointer.
     *
     * @param row  Zero-based row index relative to this Block's origin.
     */
    ElementType* getWritePointer (int row) noexcept
    {
        jassert (row >= 0 and row < numRows);
        jassert (ringSize > 0);
        const int physical { (head + startRow + row) % ringSize };
        return static_cast<ElementType*> (
            static_cast<void*> (base + static_cast<size_t> (physical) * strideBytes));
    }

    /**
     * @brief Mutable pointer with caller-supplied head position.
     *
     * Same ring mapping as getWritePointer(row), but uses @p headPosition instead
     * of the Block's stored head.  Used by Video, which tracks its own write position
     * and flushes it to State — Block's stored head may be stale relative to Video's
     * current position.
     *
     * @param row           Zero-based row index relative to this Block's origin.
     * @param headPosition  Ring head position supplied by the caller.
     */
    ElementType* getWritePointer (int row, int headPosition) noexcept
    {
        jassert (row >= 0 and row < numRows);
        jassert (ringSize > 0);
        const int physical { (headPosition + startRow + row) % ringSize };
        return static_cast<ElementType*> (
            static_cast<void*> (base + static_cast<size_t> (physical) * strideBytes));
    }

    //==========================================================================
    // Row mutation
    //==========================================================================

    /**
     * @brief Zero-fill a single row.
     *
     * Zeroes rowStrideBytes at the physical address of the row.
     *
     * @param row  Zero-based row index relative to this Block's origin.
     */
    void clear (int row) noexcept
    {
        jassert (row >= 0 and row < numRows);
        jassert (ringSize > 0);
        const int physical { (head + startRow + row) % ringSize };
        std::memset (
            base + static_cast<size_t> (physical) * strideBytes,
            0,
            strideBytes);
    }

    /**
     * @brief Zero-fill a single row with caller-supplied head position.
     * @param row           Zero-based row index relative to this Block's origin.
     * @param headPosition  Ring head position supplied by the caller.
     */
    void clear (int row, int headPosition) noexcept
    {
        jassert (row >= 0 and row < numRows);
        jassert (ringSize > 0);
        const int physical { (headPosition + startRow + row) % ringSize };
        std::memset (
            base + static_cast<size_t> (physical) * strideBytes,
            0,
            strideBytes);
    }

    /**
     * @brief Copy one row from another position within this Block.
     *
     * Both rows are ring-mapped.  Source and destination must not alias the same
     * physical row.
     *
     * @param destRow  Destination row index (zero-based, relative to this Block's origin).
     * @param srcRow   Source row index (zero-based, relative to this Block's origin).
     */
    void copyRow (int destRow, int srcRow) noexcept
    {
        jassert (destRow >= 0 and destRow < numRows);
        jassert (srcRow >= 0 and srcRow < numRows);
        jassert (ringSize > 0);
        const int physDest { (head + startRow + destRow) % ringSize };
        const int physSrc  { (head + startRow + srcRow)  % ringSize };
        std::memcpy (
            base + static_cast<size_t> (physDest) * strideBytes,
            base + static_cast<size_t> (physSrc) * strideBytes,
            strideBytes);
    }

    /**
     * @brief Copy one row with caller-supplied head position.
     * @param destRow       Destination row index.
     * @param srcRow        Source row index.
     * @param headPosition  Ring head position supplied by the caller.
     */
    void copyRow (int destRow, int srcRow, int headPosition) noexcept
    {
        jassert (destRow >= 0 and destRow < numRows);
        jassert (srcRow >= 0 and srcRow < numRows);
        jassert (ringSize > 0);
        const int physDest { (headPosition + startRow + destRow) % ringSize };
        const int physSrc  { (headPosition + startRow + srcRow)  % ringSize };
        std::memcpy (
            base + static_cast<size_t> (physDest) * strideBytes,
            base + static_cast<size_t> (physSrc) * strideBytes,
            strideBytes);
    }

    //==========================================================================
    // Sub-views
    //==========================================================================

    /**
     * @brief Sub-view starting at `offset` with `length` rows.
     *
     * All ring fields (base, head, strideBytes, ringSize, numCols) carry through unchanged.
     *
     * @param offset  Row offset from this Block's origin.
     * @param length  Number of rows in the sub-view.
     */
    Block getSubBlock (int offset, int length) const noexcept
    {
        jassert (offset >= 0 and length >= 0);
        jassert (offset + length <= numRows);

        Block sub;
        sub.base        = base;
        sub.head        = head;
        sub.startRow    = startRow + offset;
        sub.numRows     = length;
        sub.numCols     = numCols;
        sub.strideBytes = strideBytes;
        sub.ringSize    = ringSize;
        return sub;
    }

    /**
     * @brief Sub-view from `offset` to end of this Block.
     * @param offset  Row offset from this Block's origin.
     */
    Block getSubBlock (int offset) const noexcept
    {
        jassert (offset >= 0 and offset <= numRows);
        return getSubBlock (offset, numRows - offset);
    }

private:
    //==========================================================================
    // Storage  (trivially copyable — no destructor work, no allocation)
    //==========================================================================

    char*        base        { nullptr }; ///< Channel base pointer as byte pointer (FAM-aware stride arithmetic).
    int          head        { 0 };       ///< Ring head position at capture time.
    int          startRow    { 0 };       ///< Logical row offset within the view.
    int          numRows     { 0 };       ///< Number of rows in this view.
    int          numCols     { 0 };       ///< Number of columns per row.
    size_t       strideBytes { 0 };       ///< Row stride in bytes (FAM-aware).
    int          ringSize    { 0 };       ///< Total rows in source buffer (modulo operand).
};

static_assert (std::is_trivially_copyable_v<Block<char>>,
               "jam::Block must remain trivially copyable");

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam

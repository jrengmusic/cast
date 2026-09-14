/**
 * @file jam_Row.h
 * @brief FAM grid cell-row: per-row metadata + inline `AttributedChar` array.
 *
 * `jam::Row` is Video's volatile terminal surface — one row of the viewport grid.
 * The element type of `jam::Buffer<Row>`. Trivially copyable; written via
 * `Buffer::getWritePointer`. Storage is width-free: viewport width is not baked in.
 *
 * @par FlexType protocol
 * `Row::FlexType = AttributedChar` tells `Buffer` to compute stride from the FAM element
 * type rather than `sizeof(Row)` alone.
 *
 * @par Flag semantics
 * - `flexWrap`  — flex-wrap point: cursor wrapped at right margin.
 *   Default 0 = line terminates (hard newline or short line).
 * - `collapsed` — reflow tombstone: Row consumed by unwrap/join, excluded from output.
 *   Cleared on `Buffer::clear()`.
 * - `justify`   — Row contains `AttributedChar::flexGap` — distribute free space to elastic chars.
 * - `mark`      — bits 3-5: shell-integration semantic classification (OSC 133),
 *   `jam::TextLine::Mark` value. Packed into the existing `flags` byte —
 *   `flexWrap`/`collapsed`/`justify` occupy bits 0-2, leaving 5 spare bits;
 *   the 3-bit mark field fits without growing `Row` or breaking its
 *   FAM/trivially-copyable contract. 2 bits (6-7) remain spare. `copyRow()`
 *   memcpy's the whole row, so mark travels with the row through scroll shifts.
 *
 * @see jam::AttributedChar
 * @see jam::TextLine::Mark
 * @see jam::Buffer
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @brief Video's viewport grid row — a FAM array of `jam::AttributedChar` (metadata + inline `chars`).
 *
 * Trivially copyable. Lives in `Buffer<Row>`. `usedCols` is the authoritative
 * content width. This type never reaches `StringArray` or `Arrangement` — it is
 * Video's private grid type only.
 */
struct Row
{
    using FlexType = AttributedChar;

    uint16_t usedCols { 0 };   ///< Rightmost non-blank column + 1. Content boundary for shaper and reflow.
    uint8_t  flags    { 0 };   ///< Bitfield: flexWrap | collapsed | justify | mark (bits 3-5).

    static constexpr uint8_t flexWrap  { 1 << 0 };  ///< Flex-wrap point — content continues on next Row.
    static constexpr uint8_t collapsed { 1 << 1 };  ///< Reflow tombstone — Row consumed by unwrap/join.
    static constexpr uint8_t justify   { 1 << 2 };  ///< Row contains flexGap — distribute free space to elastic chars.

    static constexpr uint8_t markShift { 3 };                       ///< Bit position of the 3-bit mark field within `flags`.
    static constexpr uint8_t markMask  { uint8_t (0x7) << 3 };       ///< Mask for the 3-bit mark field (bits 3-5) — a `jam::TextLine::Mark` value.

    AttributedChar chars[];              ///< C99 FAM. Sized at allocation by Buffer. Accessed as chars[col].
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam

/**
 * @file jam_ShapedTextOptions.h
 * @brief Layout configuration for Arrangement and JustifiedText.
 *
 * Mirrors juce::ShapedTextOptions. Carries wrap width, justification mode,
 * and word-break policy. Passed to Arrangement::shape() and consumed by
 * JustifiedText.
 */
#pragma once

namespace jam
{

/**
 * @struct ShapedTextOptions
 * @brief Immutable fluent-builder configuration for text shaping and layout.
 *
 * Mirrors juce::ShapedTextOptions. Passed to Arrangement::shape() and
 * consumed by JustifiedText. Each `with*` method returns a modified copy —
 * the receiver is never mutated in place.
 */
struct ShapedTextOptions
{
    /** @brief Column width to wrap at; 0 means no wrapping. */
    int wrapColumns { 0 };

    /** @brief Text alignment within the layout area. */
    juce::Justification justification { juce::Justification::topLeft };

    /** @brief When true, allows a line break inside a word if it would otherwise overflow. */
    bool allowBreakingInsideWord { false };

    /** @brief Returns a copy with wrapColumns set to @p cols.
     *  @param cols  New wrap column width.
     *  @return Modified copy.
     */
    ShapedTextOptions withWrapColumns (int cols) const noexcept
    {
        auto copy { *this };
        copy.wrapColumns = cols;
        return copy;
    }

    /** @brief Returns a copy with justification set to @p j.
     *  @param j  New justification mode.
     *  @return Modified copy.
     */
    ShapedTextOptions withJustification (juce::Justification j) const noexcept
    {
        auto copy { *this };
        copy.justification = j;
        return copy;
    }

    /** @brief Returns a copy with allowBreakingInsideWord set to @p allow.
     *  @param allow  New in-word breaking policy.
     *  @return Modified copy.
     */
    ShapedTextOptions withAllowBreakingInsideWord (bool allow) const noexcept
    {
        auto copy { *this };
        copy.allowBreakingInsideWord = allow;
        return copy;
    }
};

} // namespace jam

/**
 * @file        jam_Bounds.h
 * @brief       Packed x/y/width/height quad — jam::Union<int16_t, int16_t, int16_t, int16_t> with typed constructors.
 */
namespace jam
{
/*____________________________________________________________________________*/

/** @brief Packed x + y + width + height quad of int16_t elements.
 *
 *  Wraps jam::Union<int16_t, int16_t, int16_t, int16_t> with typed constructors
 *  to avoid casting at call sites. Round-trips through ValueTree as int64 (see
 *  the (const juce::var&) and (juce::int64 packed) constructors) — the Backing
 *  is 64 bits, four int16_t fields summing to 8 bytes.
 *
 *  @code
 *  // Integral packed bounds (ValueTree round-trip)
 *  jam::Bounds paneBounds (rect);
 *  auto [x, y, width, height] = paneBounds;
 *  auto rectangle = paneBounds.toRectangle();
 *  @endcode
 */
struct Bounds : jam::Union<int16_t, int16_t, int16_t, int16_t>
{
    /** @brief Pack from int x/y/width/height, narrowing to int16_t.
     *  @param x       X position in pixels.
     *  @param y       Y position in pixels.
     *  @param width   Width in pixels.
     *  @param height  Height in pixels.
     */
    Bounds (int x, int y, int width, int height) noexcept
        : jam::Union<int16_t, int16_t, int16_t, int16_t> {
              jam::Union<int16_t, int16_t, int16_t, int16_t>::pack (static_cast<int16_t> (x),
                                                                     static_cast<int16_t> (y),
                                                                     static_cast<int16_t> (width),
                                                                     static_cast<int16_t> (height)) }
    {
    }

    /** @brief Pack from a juce::Rectangle<int>, narrowing to int16_t. */
    explicit Bounds (const juce::Rectangle<int>& rect) noexcept
        : jam::Union<int16_t, int16_t, int16_t, int16_t> {
              jam::Union<int16_t, int16_t, int16_t, int16_t>::pack (static_cast<int16_t> (rect.getX()),
                                                                     static_cast<int16_t> (rect.getY()),
                                                                     static_cast<int16_t> (rect.getWidth()),
                                                                     static_cast<int16_t> (rect.getHeight())) }
    {
    }

    /** @brief Unpack from ValueTree property (int64 round-trip).
     *  @param v  var holding the packed int64 written by the (juce::Rectangle<int>) constructor.
     */
    explicit Bounds (const juce::var& v) noexcept
        : jam::Union<int16_t, int16_t, int16_t, int16_t> { static_cast<
              typename jam::Union<int16_t, int16_t, int16_t, int16_t>::Backing> (
              static_cast<juce::int64> (v)) }
    {
    }

    /** @brief Constructs from a raw packed int64 (unpacking). */
    explicit Bounds (juce::int64 packed) noexcept
        : jam::Union<int16_t, int16_t, int16_t, int16_t> { static_cast<
              typename jam::Union<int16_t, int16_t, int16_t, int16_t>::Backing> (packed) }
    {
    }

    /** @brief Unpack to a juce::Rectangle<int>. */
    juce::Rectangle<int> toRectangle() const noexcept
    {
        return { static_cast<int> (this->template unpack<0>()),
                 static_cast<int> (this->template unpack<1>()),
                 static_cast<int> (this->template unpack<2>()),
                 static_cast<int> (this->template unpack<3>()) };
    }

    /** @brief True when width <= height (portrait / vertical seam). */
    bool isVertical() const noexcept
    {
        return this->template unpack<2>() <= this->template unpack<3>();
    }

    /** @brief True when width > height (landscape / horizontal seam). */
    bool isHorizontal() const noexcept
    {
        return this->template unpack<2>() > this->template unpack<3>();
    }

    /** @brief Unpack to a normalised Rectangle<float>, dividing each element by scale. */
    juce::Rectangle<float> toNormalised (int16_t scale) const noexcept
    {
        const auto s { static_cast<float> (scale) };
        return { static_cast<float> (this->template unpack<0>()) / s,
                 static_cast<float> (this->template unpack<1>()) / s,
                 static_cast<float> (this->template unpack<2>()) / s,
                 static_cast<float> (this->template unpack<3>()) / s };
    }

    /** @brief Pack from a normalised Rectangle<float>, multiplying each element by scale. */
    static Bounds fromNormalised (juce::Rectangle<float> rect, int16_t scale) noexcept
    {
        return { juce::roundToInt (rect.getX() * scale),
                 juce::roundToInt (rect.getY() * scale),
                 juce::roundToInt (rect.getWidth() * scale),
                 juce::roundToInt (rect.getHeight() * scale) };
    }

    /** @brief Project normalised bounds into a pixel container. */
    juce::Rectangle<int> toRect (juce::Rectangle<int> container, int16_t scale) const noexcept
    {
        const auto s { static_cast<float> (scale) };
        return juce::Rectangle<float> {
            static_cast<float> (this->template unpack<0>()) / s * static_cast<float> (container.getWidth()) + static_cast<float> (container.getX()),
            static_cast<float> (this->template unpack<1>()) / s * static_cast<float> (container.getHeight()) + static_cast<float> (container.getY()),
            static_cast<float> (this->template unpack<2>()) / s * static_cast<float> (container.getWidth()),
            static_cast<float> (this->template unpack<3>()) / s * static_cast<float> (container.getHeight()) }.toNearestInt();
    }
};

static_assert (sizeof (Bounds) == 8);
static_assert (std::is_trivially_copyable_v<Bounds>);
static_assert (std::is_standard_layout_v<Bounds>);

/** @brief ADL get() for jam::Bounds structured binding. */
template <size_t I>
constexpr auto get (const Bounds& b) noexcept
{
    return b.template unpack<I>();
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam

namespace std
{
/*____________________________________________________________________________*/
/** @brief Structured binding support for jam::Bounds. */
template <>
struct tuple_size<jam::Bounds> : std::integral_constant<size_t, 4>
{
};
template <size_t I>
struct tuple_element<I, jam::Bounds>
{
    using type = int16_t;
};
/**______________________________END OF NAMESPACE______________________________*/
}// namespace std

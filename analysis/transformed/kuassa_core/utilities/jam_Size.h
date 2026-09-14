/**
 * @file        jam_Size.h
 * @brief       Packed width/height pair — jam::Union\<T, T\> with typed constructors.
 */
namespace jam
{
/*____________________________________________________________________________*/

/** @brief Packed width + height pair of matching element type T.
 *
 *  Wraps jam::Union\<T, T\> with typed constructors to avoid casting at call
 *  sites. Round-trips through ValueTree as int (see the (const juce::var&)
 *  and (int packed) constructors) — meaningful only when Backing is 32 bits
 *  (T no larger than int16_t/uint16_t); for T = float (Backing = uint64_t)
 *  those two constructors remain available for template uniformity but are
 *  not the intended construction path.
 *
 *  @code
 *  // Integral packed dimensions (ValueTree round-trip)
 *  jam::Size<int16_t> windowSize (1920, 1080);
 *  auto [w, h] = windowSize;
 *  int packed = windowSize.toInt();
 *
 *  // Float dimensions (GPU device-space position/size, no rounding)
 *  jam::Size<float> deviceSize (bounds.getWidth(), bounds.getHeight());
 *  auto [dw, dh] = deviceSize;
 *  @endcode
 *
 *  @tparam T  Element type packed twice into jam::Union\<T, T\>.
 */
template <typename T>
struct Size : jam::Union<T, T>
{
    /** @brief Pack from int dimensions, narrowing to T.
     *  @param width   Width in pixels.
     *  @param height  Height in pixels.
     */
    Size (int width, int height) noexcept
        : jam::Union<T, T> { jam::Union<T, T>::pack (static_cast<T> (width),
                                                     static_cast<T> (height)) }
    {
    }

    /** @brief Pack from float dimensions.
     *  Exact (no rounding) when T is float — sub-pixel precision preserved for
     *  GPU device-space records. Rounds to T via jam::toInt otherwise.
     *  @param width   Width in pixels.
     *  @param height  Height in pixels.
     */
    Size (float width, float height) noexcept
        : jam::Union<T, T> { packFloat (width, height) }
    {
    }

    /** @brief Unpack from ValueTree property (int round-trip).
     *  @param v  var holding the packed int written by toInt().
     */
    explicit Size (const juce::var& v) noexcept
        : jam::Union<T, T> { static_cast<typename jam::Union<T, T>::Backing> (
              static_cast<int> (v)) }
    {
    }

    /** @brief Constructs from a raw packed int (unpacking). */
    explicit Size (int packed) noexcept
        : jam::Union<T, T> { static_cast<typename jam::Union<T, T>::Backing> (packed) }
    {
    }

    /** @brief Pack to int for ValueTree storage. */
    int toInt() const noexcept { return static_cast<int> (this->bits); }

    int getArea() const noexcept
    {
        return this->template unpack<0>() * this->template unpack<1>();
    }

    Size operator* (float scaleFactor) const noexcept
    {
        return Size { jam::toInt (scaleFactor * this->template unpack<0>()),
                      jam::toInt (scaleFactor * this->template unpack<1>()) };
    }

    Size topAndBottomPadded (int pad) const noexcept
    {
        return Size { this->template unpack<0>(), this->template unpack<1>() + (2 * pad) };
    }

    //==============================================================================
    /** @brief Checks if the size is in landscape orientation.
     *  @param size  The size to check.
     *  @return bool  True if width is greater than height.
     */
    static bool isLandscape (const Size& size) noexcept
    {
        return size.template unpack<0>() > size.template unpack<1>();
    }

    /** @brief Checks if the size is in portrait orientation.
     *  @param size  The size to check.
     *  @return bool  True if width is less than or equal to height.
     */
    static bool isPortrait (const Size& size) noexcept
    {
        return size.template unpack<0>() <= size.template unpack<1>();
    }

    static int getOrientation (const std::array<Size, 2>& sizesToCompare)
    {
        if (std::all_of (sizesToCompare.begin(), sizesToCompare.end(), [] (auto& s)
                         {
                             return isPortrait (s);
                         }))
            return map::ViewOrientation::portrait;

        return map::ViewOrientation::landscape;
    }

    static int getOrientation (const Size& size)
    {
        if (isPortrait (size))
            return map::ViewOrientation::portrait;

        return map::ViewOrientation::landscape;
    }

    static Size getPortrait (const Size& size) noexcept
    {
        if (isLandscape (size))
            return Size { size.template unpack<1>(), size.template unpack<0>() };
        return size;
    }

    static Size getLandscape (const Size& size) noexcept
    {
        if (isPortrait (size))
            return Size { size.template unpack<1>(), size.template unpack<0>() };
        return size;
    }

    static float getRelativeScale (const Size& outer, const Size& inner, float maxScale = 1.0f)
    {
        const float outWidth { static_cast<float> (outer.template unpack<0>()) };
        const float outHeight { static_cast<float> (outer.template unpack<1>()) };
        const float inWidth { static_cast<float> (inner.template unpack<0>()) };
        const float inHeight { static_cast<float> (inner.template unpack<1>()) };
        const bool isSameOrientation { getOrientation (inner) == getOrientation (outer) };

        const auto& scale = [maxScale, isSameOrientation] (float constraint, float parallel, float perpendicular)
        {
            return (constraint * maxScale) / (isSameOrientation ? parallel : perpendicular);
        };

        switch (getOrientation (outer))
        {
            case map::ViewOrientation::portrait:
                return scale (outWidth, inHeight, inWidth);

            case map::ViewOrientation::landscape:
                return scale (outHeight, inWidth, inHeight);
        }

        return maxScale;
    }

private:
    /** @brief T-dependent float-pack branch for the Size(float, float) constructor —
     *  exact pack when T is float, jam::toInt-rounded pack otherwise.
     */
    static jam::Union<T, T> packFloat (float width, float height) noexcept
    {
        if constexpr (std::is_same_v<T, float>)
            return jam::Union<T, T>::pack (width, height);
        else
            return jam::Union<T, T>::pack (static_cast<T> (jam::toInt (width)),
                                           static_cast<T> (jam::toInt (height)));
    }
};

static_assert (sizeof (Size<int16_t>) == 4);
static_assert (sizeof (Size<float>) == 8, "Size<float> must be bit-identical to a raw float[2]");
static_assert (std::is_trivially_copyable_v<Size<int16_t>>);
static_assert (std::is_trivially_copyable_v<Size<float>>);
static_assert (std::is_standard_layout_v<Size<float>>);

/** @brief ADL get() for jam::Size structured binding. */
template <size_t I, typename T>
constexpr auto get (const Size<T>& s) noexcept
{
    return s.template unpack<I>();
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam

namespace std
{
/*____________________________________________________________________________*/
/** @brief Structured binding support for jam::Size. */
template <typename T>
struct tuple_size<jam::Size<T>> : std::integral_constant<size_t, 2>
{
};
template <size_t I, typename T>
struct tuple_element<I, jam::Size<T>>
{
    using type = T;
};
/**______________________________END OF NAMESPACE______________________________*/
}// namespace std

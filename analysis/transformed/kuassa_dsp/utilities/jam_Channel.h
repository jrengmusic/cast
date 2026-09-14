/**
 * @file jam_Channel.h
 * @brief Mid/side encode-decode utilities and per-side processing helpers for stereo channels.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @struct Channel
 * @brief Mid/side encode-decode utilities and per-side processing helpers for stereo channels.
 */
struct Channel
{
    /** @brief Channel processing/summing mode selector. */
    enum processing
    {
        off = 0,
        stereo,
        mid,
        side,
        left,
        right
    };

    //==============================================================================
    /**
     * @brief Encodes a left/right sample pair into mid/side in place.
     * @tparam SampleType The sample's numeric type.
     * @param left The left sample in, mid sample out.
     * @param right The right sample in, side sample out.
     */
    template <typename SampleType>
    static void encode (SampleType& left, SampleType& right)
    {
        SampleType mid { (SampleType) 0.5 * (left + right) };
        SampleType side { (SampleType) 0.5 * (left - right) };

        left = mid;
        right = side;
    }

    /**
     * @brief Decodes a mid/side sample pair back into left/right in place.
     * @tparam SampleType The sample's numeric type.
     * @param left The mid sample in, left sample out.
     * @param right The side sample in, right sample out.
     */
    template <typename SampleType>
    static void decode (SampleType& left, SampleType& right)
    {
        SampleType mid { left + right };
        SampleType side { left - right };
        left = mid;
        right = side;
    }

    /**
     * @brief Encodes to mid/side, applies process to the mid channel only, then decodes back to left/right.
     * @tparam SampleType The sample's numeric type.
     * @tparam FunctionType A callable taking (SampleType& sample).
     * @param sampleL The left sample in, processed left sample out.
     * @param sampleR The right sample in, processed right sample out.
     * @param process The callable applied to the mid channel.
     */
    template <typename SampleType, typename FunctionType>
    static void processMid (SampleType& sampleL,
                            SampleType& sampleR,
                            const FunctionType& process)
    {
        encode (sampleL, sampleR);
        process (sampleL);
        decode (sampleL, sampleR);
    }

    /**
     * @brief Encodes to mid/side, applies process to the side channel only, then decodes back to left/right.
     * @tparam SampleType The sample's numeric type.
     * @tparam FunctionType A callable taking (SampleType& sample).
     * @param sampleL The left sample in, processed left sample out.
     * @param sampleR The right sample in, processed right sample out.
     * @param process The callable applied to the side channel.
     */
    template <typename SampleType, typename FunctionType>
    static void processSide (SampleType& sampleL,
                             SampleType& sampleR,
                             const FunctionType& process)
    {
        encode (sampleL, sampleR);
        process (sampleR);
        decode (sampleL, sampleR);
    }

};

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam::dsp

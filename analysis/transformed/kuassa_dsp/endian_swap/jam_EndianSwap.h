/**
 * @file jam_EndianSwap.h
 * @brief Byte-order reversal utilities for primitive value endianness conversion.
 */

#pragma once

namespace jam::dsp
{
/*__________________________________________________________________________________________*/
/**
 * @class EndianSwap
 * @brief Utility for reversing the byte order of primitive values (endianness conversion).
 */
class EndianSwap
{
public:
    //----------------------------------------------------------------------------
    /**
     * @brief Reverses the byte order of a signed 16-bit value.
     * @param s The value whose bytes are swapped.
     * @return The byte-swapped value.
     */
    short shortswap (short s)
    {
        unsigned char b1, b2;

        b1 = s & 255;
        b2 = (s >> 8) & 255;

        return (b1 << 8) + b2;
    };
    //----------------------------------------------------------------------------
    /**
     * @brief Reverses the byte order of an unsigned 16-bit value.
     * @param s The value whose bytes are swapped.
     * @return The byte-swapped value.
     */
    unsigned short unsignedshortswap (unsigned short s)
    {
        unsigned char b1, b2;

        b1 = s & 255;
        b2 = (s >> 8) & 255;

        return (b1 << 8) + b2;
    };
    //----------------------------------------------------------------------------
    /**
     * @brief Reverses the byte order of a 32-bit signed integer.
     * @param i The value whose bytes are swapped.
     * @return The byte-swapped value.
     */
    int intswap (int i)
    {
        unsigned char b1, b2, b3, b4;

        b1 = i & 255;
        b2 = (i >> 8) & 255;
        b3 = (i >> 16) & 255;
        b4 = (i >> 24) & 255;

        return ((int) b1 << 24) + ((int) b2 << 16) + ((int) b3 << 8) + b4;
    };
    //----------------------------------------------------------------------------
    /**
     * @brief Reverses the byte order of a 32-bit floating-point value.
     * @param f The value whose bytes are swapped.
     * @return The byte-swapped value.
     */
    float floatswap (float f)
    {
        union
        {
            float f;
            unsigned char b[4];
        } dat1, dat2;

        dat1.f = f;
        dat2.b[0] = dat1.b[3];
        dat2.b[1] = dat1.b[2];
        dat2.b[2] = dat1.b[1];
        dat2.b[3] = dat1.b[0];
        return dat2.f;
    };
    //----------------------------------------------------------------------------
};

/**____________________________________END OF NAMESPACE_____________________________________*/
} /** namespace jam::dsp */

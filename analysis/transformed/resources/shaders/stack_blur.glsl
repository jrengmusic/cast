// stack_blur.glsl — glslc-include-only, not a standalone shader.
// GPU counterpart of stackBlurLine (jam_SimdStackBlur.h) — same running-sum
// walk over one scan line: edge-replicate the first/last element, normalize by
// 1/(radius+1)^2, round by +0.5 (stackBlurRoundOffset), clamp to
// [stackBlurLowestByte, stackBlurHighestByte]. This is the CPU/GPU shared
// contract both kernels must produce byte-identical output against.
//
// The CPU kernel keeps a ring buffer (std::vector<float> stack) because a
// scalar loop cannot re-read arbitrary offsets of the source line cheaply
// once it has overwritten earlier elements in place. This kernel never writes
// its input, so every stack slot the CPU ring buffer would hold is provably
// just a clamped re-read of the original line at a fixed offset from the
// current position — loadElement (defined by the including .comp above this
// #include, clamped to [0, lineLength - 1] exactly like the CPU kernel's
// jlimit) replaces the ring buffer entirely. This is a transformation, not an
// approximation: every element loadElement returns is the exact value the CPU
// ring buffer would hold at that stack-recurrence step.
//
// Consumed by: stack_blur_texture.comp, stack_blur_buffer.comp.

const float stackBlurRoundOffset  = 0.5;
const float stackBlurLowestByte   = 0.0;
const float stackBlurHighestByte  = 255.0;

uint packStackBlurElement (vec4 byteColor)
{
    uint red   = uint (byteColor.r);
    uint green = uint (byteColor.g);
    uint blue  = uint (byteColor.b);
    uint alpha = uint (byteColor.a);

    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

void stackBlurLine ()
{
    const float normalization = 1.0 / float ((radius + 1) * (radius + 1));

    vec4 first  = loadElement (0);
    vec4 sum    = vec4 (0.0);
    vec4 sumIn  = vec4 (0.0);
    vec4 sumOut = vec4 (0.0);

    for (int i = 0; i <= radius; ++i)
    {
        sum += first * float (i + 1);
        sumOut += first;
    }

    for (int i = 1; i <= radius; ++i)
    {
        vec4 element = loadElement (i);
        sum += element * float (radius + 1 - i);
        sumIn += element;
    }

    for (int x = 0; x < lineLength; ++x)
    {
        vec4 blurred = clamp (sum * normalization + vec4 (stackBlurRoundOffset),
                               vec4 (stackBlurLowestByte), vec4 (stackBlurHighestByte));
        stackBlurOutput[x * lineCount + stackBlurCurrentLine] = packStackBlurElement (blurred);

        sum -= sumOut;
        sumOut -= loadElement (x - radius);

        vec4 incoming = loadElement (x + radius + 1);
        sumIn += incoming;
        sum += sumIn;

        vec4 next = loadElement (x + 1);
        sumOut += next;
        sumIn -= next;
    }
}

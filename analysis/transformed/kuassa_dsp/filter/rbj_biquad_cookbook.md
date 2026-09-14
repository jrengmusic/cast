# Cookbook Formulae for Audio EQ Biquad Filter Coefficients

**by Robert Bristow-Johnson** <rbj@audioimagination.com>

---

## Introduction

All filter transfer functions were derived from analog prototypes and digitized using the Bilinear Transform (BLT). BLT frequency warping has been taken into account for both significant frequency relocation (normal "prewarping") and bandwidth readjustment (since bandwidth is compressed when mapped from analog to digital using the BLT).

## Biquad Transfer Function

First, given a biquad transfer function defined as:

```
        b0 + b1*z⁻¹ + b2*z⁻²
H(z) = ------------------------     (Eq 1)
        a0 + a1*z⁻¹ + a2*z⁻²
```

This shows 6 coefficients instead of 5. Depending on your architecture, you will likely normalize `a0` to be 1 and perhaps also `b0` to 1 (collecting that into an overall gain coefficient). Then your transfer function would look like:

```
        (b0/a0) + (b1/a0)*z⁻¹ + (b2/a0)*z⁻²
H(z) = ---------------------------------------     (Eq 2)
           1 + (a1/a0)*z⁻¹ + (a2/a0)*z⁻²
```

or

```
                  1 + (b1/b0)*z⁻¹ + (b2/b0)*z⁻²
H(z) = (b0/a0) * ---------------------------------     (Eq 3)
                  1 + (a1/a0)*z⁻¹ + (a2/a0)*z⁻²
```

## Implementation (Direct Form 1)

The most straightforward implementation is "Direct Form 1" (Eq 2):

```
y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2]
                - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]     (Eq 4)
```

This is probably both the best and easiest method to implement in the 56K and other fixed-point or floating-point architectures with a double-wide accumulator.

---

## User-Defined Parameters

Begin with these user-defined parameters:

- **Fs**: The sampling frequency
- **f0**: Center Frequency, Corner Frequency, or shelf midpoint frequency (depending on filter type). The "significant frequency."
- **dBgain**: Used only for peaking and shelving filters
- **Q**: The EE kind of definition, except for peakingEQ in which A*Q is the classic EE Q. This adjustment was made so that a boost of N dB followed by a cut of N dB for identical Q and f0/Fs results in a precisely flat unity gain filter.

**Alternative parameters:**

- **BW**: Bandwidth in octaves (between -3 dB frequencies for BPF and notch, or between midpoint (dBgain/2) gain frequencies for peaking EQ)
- **S**: A "shelf slope" parameter (for shelving EQ only). When S = 1, the shelf slope is as steep as it can be while remaining monotonically increasing or decreasing gain with frequency. The shelf slope, in dB/octave, remains proportional to S for all other values for a fixed f0/Fs and dBgain.

---

## Intermediate Variables

Compute these intermediate variables:

```
A = sqrt(10^(dBgain/20))
  = 10^(dBgain/40)     (for peaking and shelving EQ filters only)

w0 = 2*pi*f0/Fs

cos(w0)
sin(w0)

alpha = sin(w0)/(2*Q)                                        (case: Q)
      = sin(w0)*sinh(ln(2)/2 * BW * w0/sin(w0))            (case: BW)
      = sin(w0)/2 * sqrt((A + 1/A)*(1/S - 1) + 2)          (case: S)
```

### FYI: Relationships

The relationship between bandwidth and Q:

```
1/Q = 2*sinh(ln(2)/2*BW*w0/sin(w0))     (digital filter with BLT)
1/Q = 2*sinh(ln(2)/2*BW)                (analog filter prototype)
```

The relationship between shelf slope and Q:

```
1/Q = sqrt((A + 1/A)*(1/S - 1) + 2)
```

A handy intermediate variable for shelving EQ filters:

```
2*sqrt(A)*alpha = sin(w0) * sqrt((A² + 1)*(1/S - 1) + 2*A)
```

---

## Filter Coefficients

Compute the coefficients for each filter type. The analog prototypes H(s) are shown for each filter type for normalized frequency.

### Low Pass Filter (LPF)

**Analog prototype:** `H(s) = 1 / (s² + s/Q + 1)`

```
b0 =  (1 - cos(w0))/2
b1 =   1 - cos(w0)
b2 =  (1 - cos(w0))/2
a0 =   1 + alpha
a1 =  -2*cos(w0)
a2 =   1 - alpha
```

### High Pass Filter (HPF)

**Analog prototype:** `H(s) = s² / (s² + s/Q + 1)`

```
b0 =  (1 + cos(w0))/2
b1 = -(1 + cos(w0))
b2 =  (1 + cos(w0))/2
a0 =   1 + alpha
a1 =  -2*cos(w0)
a2 =   1 - alpha
```

### Band Pass Filter (BPF) - Constant Skirt Gain

**Analog prototype:** `H(s) = s / (s² + s/Q + 1)` (peak gain = Q)

```
b0 =   sin(w0)/2  =   Q*alpha
b1 =   0
b2 =  -sin(w0)/2  =  -Q*alpha
a0 =   1 + alpha
a1 =  -2*cos(w0)
a2 =   1 - alpha
```

### Band Pass Filter (BPF) - Constant 0 dB Peak Gain

**Analog prototype:** `H(s) = (s/Q) / (s² + s/Q + 1)`

```
b0 =   alpha
b1 =   0
b2 =  -alpha
a0 =   1 + alpha
a1 =  -2*cos(w0)
a2 =   1 - alpha
```

### Notch Filter

**Analog prototype:** `H(s) = (s² + 1) / (s² + s/Q + 1)`

```
b0 =   1
b1 =  -2*cos(w0)
b2 =   1
a0 =   1 + alpha
a1 =  -2*cos(w0)
a2 =   1 - alpha
```

### All-Pass Filter (APF)

**Analog prototype:** `H(s) = (s² - s/Q + 1) / (s² + s/Q + 1)`

```
b0 =   1 - alpha
b1 =  -2*cos(w0)
b2 =   1 + alpha
a0 =   1 + alpha
a1 =  -2*cos(w0)
a2 =   1 - alpha
```

### Peaking EQ

**Analog prototype:** `H(s) = (s² + s*(A/Q) + 1) / (s² + s/(A*Q) + 1)`

```
b0 =   1 + alpha*A
b1 =  -2*cos(w0)
b2 =   1 - alpha*A
a0 =   1 + alpha/A
a1 =  -2*cos(w0)
a2 =   1 - alpha/A
```

### Low Shelf

**Analog prototype:** `H(s) = A * (s² + (sqrt(A)/Q)*s + A)/(A*s² + (sqrt(A)/Q)*s + 1)`

```
b0 =    A*((A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha)
b1 =  2*A*((A-1) - (A+1)*cos(w0))
b2 =    A*((A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha)
a0 =       (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha
a1 =   -2*((A-1) + (A+1)*cos(w0))
a2 =       (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha
```

### High Shelf

**Analog prototype:** `H(s) = A * (A*s² + (sqrt(A)/Q)*s + 1)/(s² + (sqrt(A)/Q)*s + A)`

```
b0 =    A*((A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha)
b1 = -2*A*((A-1) + (A+1)*cos(w0))
b2 =    A*((A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha)
a0 =       (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha
a1 =    2*((A-1) - (A+1)*cos(w0))
a2 =       (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha
```

---

## Bilinear Transform Details

The bilinear transform (with compensation for frequency warping) substitutes:

```
                        1         1 - z⁻¹
(normalized)   s  <--  ----------- * ----------
                      tan(w0/2)     1 + z⁻¹
```

Making use of these trigonometric identities:

```
             sin(w0)                           1 - cos(w0)
tan(w0/2) = -------------       (tan(w0/2))² = -------------
           1 + cos(w0)                         1 + cos(w0)
```

After factoring out common terms, the final substitutions are:

```
1    <--  (1 + 2*z⁻¹ + z⁻²) * (1 - cos(w0))

s    <--  (1         -  z⁻²) * sin(w0)

s²   <--  (1 - 2*z⁻¹ + z⁻²) * (1 + cos(w0))

1+s² <--  2 * (1 - 2*cos(w0)*z⁻¹ + z⁻²)
```

The biquad coefficient formulae above come out after a little simplification.
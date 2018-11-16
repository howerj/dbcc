#include <stdint.h>
#include <math.h> /* Only used for the macros NAN, INFINITY, signbit */

#ifndef INFINITY
#define INFINITY (1.0 / 0)
#endif

#ifndef NAN
#define NAN (0.0 / 0)
#endif

/* From https://beej.us/guide/bgnet/html/multi/index.html, 
 *
 * Special cases:
 *
 * Zero and sign bit set -> Negative Zero
 *
 * All Exponent Bits Set
 * - Mantissa is zero and sign bit is zero ->  Infinity
 * - Mantissa is zero and sign bit is on   -> -Infinity
 * - Mantissa is non-zero -> NaN */

/* pack754() -- pack a floating point number into IEEE-754 format */ 
static uint64_t pack754(double f, unsigned bits, unsigned expbits)
{
	if (f == 0.0) /* get this special case out of the way */
		return signbit(f) ? (1uLL << (bits - 1)) :  0;
	if (f != f) /* NaN, encoded as Exponent == all-bits-set, Mantissa != 0, Signbit == Do not care */
		return (1uLL << (bits - 1)) - 1uLL;
	if (f == INFINITY) /* +INFINITY encoded as Mantissa == 0, Exponent == all-bits-set */
		return ((1uLL << expbits) - 1uLL) << (bits - expbits - 1);
	if (f == -INFINITY) /* -INFINITY encoded as Mantissa == 0, Exponent == all-bits-set, Signbit == 1 */
		return (1uLL << (bits - 1)) | ((1uLL << expbits) - 1uLL) << (bits - expbits - 1);

	long long sign = 0;
	double fnorm = f;
	/* check sign and begin normalization */
	if (f < 0) { sign = 1; fnorm = -f; }

	/* get the normalized form of f and track the exponent */
	int shift = 0;
	while (fnorm >= 2.0) { fnorm /= 2.0; shift++; }
	while (fnorm < 1.0)  { fnorm *= 2.0; shift--; }
	fnorm = fnorm - 1.0;

	const unsigned significandbits = bits - expbits - 1; // -1 for sign bit

	/* calculate the binary form (non-float) of the significand data */
	const long long significand = fnorm * (( 1LL << significandbits) + 0.5f);

	/* get the biased exponent */
	const long long exp = shift + ((1LL << (expbits - 1)) - 1); // shift + bias

	/* return the final answer */
	return (sign << (bits - 1)) | (exp << (bits - expbits - 1)) | significand;
}

static inline uint32_t   pack754_32(float  f)   { return   pack754(f, 32, 8); }
static inline uint64_t   pack754_64(double f)   { return   pack754(f, 64, 11); }

/* unpack754() -- unpack a floating point number from IEEE-754 format */ 
static double unpack754(uint64_t i, unsigned bits, unsigned expbits)
{
	if (i == 0) return 0.0;

	const uint64_t expset = ((1uLL << expbits) - 1uLL) << (bits - expbits - 1);
	if ((i & expset) == expset) { /* NaN or +/-Infinity */
		if (i & ((1uLL << (bits - expbits - 1)) - 1uLL)) /* Non zero Mantissa means NaN */
			return NAN;
		return i & (1uLL << (bits - 1)) ? -INFINITY : INFINITY;
	}

	/* pull the significand */
	const unsigned significandbits = bits - expbits - 1; /* - 1 for sign bit */
	double result = (i & ((1LL << significandbits) - 1)); /* mask */
	result /= (1LL << significandbits);  /* convert back to float */
	result += 1.0f;                        /* add the one back on */

	/* deal with the exponent */
	const unsigned bias = (1 << (expbits - 1)) - 1;
	long long shift = ((i >> significandbits) & ((1LL << expbits) - 1)) - bias;
	while (shift > 0) { result *= 2.0; shift--; }
	while (shift < 0) { result /= 2.0; shift++; }
	
	return (i >> (bits - 1)) & 1? -result: result; /* sign it, and return */
}

static inline float    unpack754_32(uint32_t i) { return unpack754(i, 32, 8); }
static inline double   unpack754_64(uint64_t i) { return unpack754(i, 64, 11); }

#include <stdio.h>

void fpu32(float x) {
	uint32_t xp = pack754_32(x);
	float xu = unpack754_32(xp);
	printf("%f %lx %f\n", x, (unsigned long)xp, xu);
}

void fpu64(float x) {
	uint64_t xp = pack754_64(x);
	double xu = unpack754_64(xp);
	printf("%f %llx %f\n", x, (unsigned long long)xp, xu);
}

int main(void) {
	fpu32(3.25);
	fpu32(NAN);
	fpu32(INFINITY);
	fpu32(-INFINITY);
	fpu32(-0.0);

	fpu64(-3.25);
	fpu64(NAN);
	fpu64(INFINITY);
	fpu64(-INFINITY);
	fpu64(-0.0);
	return 0;
}


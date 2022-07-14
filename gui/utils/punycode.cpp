#include <cstdint>

enum ErrorType
{
    NoError = 0,
    IllegalArgumentError = 1,
    IndexOutOfBoundsError = 2,
    InvalidCharFoundError = 3,
    InternalError = 4,
    BufferOverflowError = 5,
    StringNotTerminatedError = 6
};

/* Punycode parameters for Bootstring */
static constexpr int32_t BASE = 36;
static constexpr int32_t TMIN = 1;
static constexpr int32_t TMAX = 26;
static constexpr int32_t SKEW = 38;
static constexpr int32_t DAMP = 700;
static constexpr int32_t INITIAL_BIAS = 72;
static constexpr int32_t INITIAL_N = 0x80;
static constexpr int32_t DELIMITER = 0x2d;

static constexpr bool  isBasic(char32_t _c) { return _c < 0x80; }

/**
 * Is this code point a surrogate (U+d800..U+dfff)?
 * @param c 32-bit code point
 * @return TRUE or FALSE
 */
static constexpr bool isSurrogate(char32_t _c) { return (((_c) & 0xfffff800) == 0xd800); }

/**
 * Does this code unit alone encode a code point (BMP, not a surrogate)?
 * @param c 16-bit code unit
 * @return TRUE or FALSE
 */
static constexpr bool isSingle(char32_t _c) { return !isSurrogate(_c); }

/**
 * Is this code unit a lead surrogate (U+d800..U+dbff)?
 * @param c 16-bit code unit
 * @return TRUE or FALSE
 */
static constexpr bool isLead(char32_t _c) { return (((_c) & 0xfffffc00) == 0xd800); }

/**
 * Is this code unit a trail surrogate (U+dc00..U+dfff)?
 * @param c 16-bit code unit
 * @return TRUE or FALSE
 */
static constexpr bool isTrail(char32_t _c) { return (((_c) & 0xfffffc00) == 0xdc00); }

/**
 * Get a supplementary code point value (U+10000..U+10ffff)
 * from its lead and trail surrogates.
 * The result is undefined if the input values are not
 * lead and trail surrogates.
 *
 * @param lead lead surrogate (U+d800..U+dbff)
 * @param trail trail surrogate (U+dc00..U+dfff)
 * @return supplementary code point (U+10000..U+10ffff)
 */
static constexpr char32_t getSupplementary(char32_t _lead, char32_t _trail)
{
    // Helper constant for supplementary
    constexpr uint32_t _SURROGATE_OFFSET = ((0xd800 << 10UL) + 0xdc00 - 0x10000);
    return (((char32_t)(_lead) << 10UL) + (char32_t)(_trail) -_SURROGATE_OFFSET);
}



/**
 * digitToBasic() returns the basic code point whose value
 * (when used for representing integers) is d, which must be in the
 * range 0 to BASE-1. The lowercase form is used unless the uppercase flag is
 * nonzero, in which case the uppercase form is used.
 */
static constexpr char encodeDigit(int32_t _digit, bool _uppercase)
{
    /*  0..25 map to ASCII a..z or A..Z */
    /* 26..35 map to ASCII 0..9         */
    if (_digit < 26)
    {
        if (_uppercase)
            return (char)(0x41 + _digit);
        else
            return (char)(0x61 + _digit);
    }
    else
    {
        return (char)((0x30 - 26) + _digit);
    }
}

/*
 * The following code omits the {parts} of the pseudo-algorithm in the spec
 * that are not used with the Punycode parameter set.
 */
 /* Bias adaptation function. */
static int32_t adaptBias(int32_t _delta, int32_t _length, bool _firstTime)
{
    int32_t count;
    if (_firstTime)
        _delta /= DAMP;
    else
        _delta /= 2;

    _delta += _delta / _length;
    for (count = 0; _delta > ((BASE - TMIN) * TMAX) / 2; count += BASE)
        _delta /= (BASE - TMIN);

    return count + (((BASE - TMIN + 1) * _delta) / (_delta + SKEW));
}

extern "C" int32_t punyencode(const char16_t* _src, int32_t _srcLength, char16_t* _dest, int32_t _destCapacity, int32_t * _pErrorCode)
{
    static constexpr int32_t MAX_CP_COUNT = 200;
    int32_t cpBuffer[MAX_CP_COUNT];
    int32_t n, delta, handledCPCount, basicLength, destLength, bias, j, m, q, k, t, h, srcCPCount;
    char32_t c, c2;

    /* argument checking */
    if (_pErrorCode == nullptr || (*_pErrorCode > NoError))
        return 0;

    if (_src == nullptr || _srcLength < -1 || (_dest == nullptr && _destCapacity != 0))
    {
        *_pErrorCode = IllegalArgumentError;
        return 0;
    }

    h = 0;

    /*
     * Handle the basic code points and
     * convert extended ones to UTF-32 in cpBuffer (caseFlag in sign bit):
     */
    srcCPCount = destLength = 0;
    if (_srcLength == -1)
    {
        /* NUL-terminated input */
        for (j = 0; /* no condition */; ++j)
        {
            if ((c = _src[j]) == 0)
                break;

            if (srcCPCount == MAX_CP_COUNT)
            {
                /* too many input code points */
                *_pErrorCode = IndexOutOfBoundsError;
                return 0;
            }

            if (isBasic(c))
            {
                cpBuffer[srcCPCount++] = 0;
                if (destLength < _destCapacity)
                    _dest[destLength] = (char)c;

                ++destLength;
            }
            else
            {
                h += (c >= 0xfe00);
                n = 0;
                if (isSingle(c))
                {
                    n |= c;
                }
                else if (isLead(c) && isTrail(c2 = _src[j + 1]))
                {
                    ++j;
                    n |= (int32_t)getSupplementary(c, c2);
                }
                else
                {
                    /* error: unmatched surrogate */
                    *_pErrorCode = InvalidCharFoundError;
                    return 0;
                }
                cpBuffer[srcCPCount++] = n;
            }
        }
    }
    else
    {
        /* length-specified input */
        for (j = 0; j < _srcLength; ++j)
        {
            if (srcCPCount == MAX_CP_COUNT)
            {
                /* too many input code points */
                *_pErrorCode = IndexOutOfBoundsError;
                return 0;
            }
            c = _src[j];
            if (isBasic(c))
            {
                cpBuffer[srcCPCount++] = 0;
                if (destLength < _destCapacity)
                    _dest[destLength] = (char)c;

                ++destLength;
            }
            else
            {
                h += (c >= 0xfe00);
                n = 0;
                if (isSingle(c))
                {
                    n |= c;
                }
                else if (isLead(c) && (j + 1) < _srcLength && isTrail(c2 = _src[j + 1]))
                {
                    ++j;
                    n |= (int32_t)getSupplementary(c, c2);
                }
                else
                {
                    /* error: unmatched surrogate */
                    *_pErrorCode = InvalidCharFoundError;
                    return 0;
                }
                cpBuffer[srcCPCount++] = n;
            }
        }
    }
    /* Finish the basic string - if it is not empty - with a delimiter. */
    basicLength = destLength;
    if (basicLength > 0)
    {
        if (destLength < _destCapacity)
            _dest[destLength] = DELIMITER;

        ++destLength;
    }

    /*
     * handledCPCount is the number of code points that have been handled
     * basicLength is the number of basic code points
     * destLength is the number of chars that have been output
     */
     /* Initialize the state: */
    n = INITIAL_N;
    delta = 0;
    bias = INITIAL_BIAS;
    /* Main encoding loop: */
    for (handledCPCount = basicLength; handledCPCount < srcCPCount - h; /* no op */)
    {
        /*
         * All non-basic code points < n have been handled already.
         * Find the next larger one:
         */
        for (m = 0x7fffffff, j = 0; j < srcCPCount; ++j)
        {
            q = cpBuffer[j] & 0x7fffffff; /* remove case flag from the sign bit */
            if (n <= q && q < m)
                m = q;
        }
        /*
         * Increase delta enough to advance the decoder's
         * <n,i> state to <m,0>, but guard against overflow:
         */
        if (m - n > (0x7fffffff - MAX_CP_COUNT - delta) / (handledCPCount + 1))
        {
            *_pErrorCode = InternalError;
            return 0;
        }
        delta += (m - n) * (handledCPCount + 1);
        n = m;
        /* Encode a sequence of same code points n */
        for (j = 0; j < srcCPCount; ++j)
        {
            q = cpBuffer[j] & 0x7fffffff; /* remove case flag from the sign bit */
            if (q < n)
            {
                ++delta;
            }
            else if (q == n)
            {
                /* Represent delta as a generalized variable-length integer: */
                for (q = delta, k = BASE; /* no condition */; k += BASE)
                {
                    t = k - bias;
                    if (t < TMIN)
                        t = TMIN;
                    else if (k >= (bias + TMAX))
                        t = TMAX;

                    if (q < t)
                        break;

                    if (destLength < _destCapacity)
                        _dest[destLength] = encodeDigit(t + (q - t) % (BASE - t), 0);

                    ++destLength;
                    q = (q - t) / (BASE - t);
                }

                if (destLength < _destCapacity)
                    _dest[destLength] = encodeDigit(q, (bool)(cpBuffer[j] < 0));

                ++destLength;
                bias = adaptBias(delta, handledCPCount + 1, (bool)(handledCPCount == basicLength));
                delta = 0;
                ++handledCPCount;
            }
        }
        ++delta;
        ++n;
    }

    /*
     * NUL-terminate a string no matter what its type.
     * Set warning and error codes accordingly.
     */
    if (_pErrorCode != nullptr && (*_pErrorCode <= NoError))
    {
        if (destLength < _destCapacity)
        {
            /* NUL-terminate the string, the NUL fits */
            _dest[destLength] = 0;
            /* unset the not-terminated warning but leave all others */
            if (*_pErrorCode == StringNotTerminatedError)
                *_pErrorCode = NoError;
        }
        else if (destLength == _destCapacity)
        {
            /* unable to NUL-terminate, but the string itself fit - set a warning code */
            *_pErrorCode = StringNotTerminatedError;
        }
        else /* length>_destCapacity */
        {
            /* even the string itself did not fit - set an error code */
            *_pErrorCode = BufferOverflowError;
        }
    }
    return destLength;
}



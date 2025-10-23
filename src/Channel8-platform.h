/**
 *
 *  Copyright (C) 2025 Roman Pauer
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of
 *  this software and associated documentation files (the "Software"), to deal in
 *  the Software without restriction, including without limitation the rights to
 *  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is furnished to do
 *  so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

// FM-tone-generator uses function sysctlbyname to check whether to use simd intrinsics (in Channel8.mm)
// the function is only defined on BSD platforms
// the function's use doesn't make sense, because even if the result is negative the compiler can generate simd instructions in the code
// this implementation always returns positive result

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__ALTIVEC__) || defined(__SSE4_1__)

#include <stddef.h>

static int sysctlbyname(const char *name, void *oldp, size_t *oldlenp, const void *newp, size_t newlen)
{
    *(int *)oldp = 1;
    return 0;
}

#endif


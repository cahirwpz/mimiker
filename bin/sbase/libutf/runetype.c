/* MIT/X Consortium Copyright (c) 2012 Connor Lane Smith <cls@lubutu.com>
 *                            (c) 2015 Laslo Hunhold <dev@frign.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "../utf.h"

int
rune1cmp(const void *v1, const void *v2)
{
	Rune r1 = *(Rune *)v1, r2 = *(Rune *)v2;

	return r1 - r2;
}

int
rune2cmp(const void *v1, const void *v2)
{
	Rune r = *(Rune *)v1, *p = (Rune *)v2;

	if(r >= p[0] && r <= p[1])
		return 0;
	else
		return r - p[0];
}

/*
 * $Id$
 */

#include <stdlib.h>

#include "debug.h"
#include "image.h"
#include "scale.h"

Image *scalePoint(Image *src, int scale, int n);
Image *scale2xBilinear(Image *src, int scale, int n);
Image *scale2xSaI(Image *src, int scale, int N);
Image *scaleScale2x(Image *src, int scale, int N);

Scaler scalerGet(FilterType filter) {
    switch (filter) {
    case SCL_POINT:
        return &scalePoint;
        
    case SCL_2xBi:
        return &scale2xBilinear;

    case SCL_2xSaI:
        return &scale2xSaI;

    case SCL_Scale2x:
        return &scaleScale2x;

    default:
        return NULL;
    }
}

/**
 * Returns true if the given scaler can scale by 3 (as well as by 2).
 */
int scaler3x(FilterType filter) {
    return filter == SCL_Scale2x;
}

/**
 * A simple row and column duplicating scaler.
 */
Image *scalePoint(Image *src, int scale, int n) {
    int x, y, i, j;
    Image *dest;

    dest = imageNew(src->w * scale, src->h * scale, src->scale * scale, src->indexed, IMTYPE_HW);
    if (!dest)
        return NULL;

    if (dest->indexed)
        imageSetPaletteFromImage(dest, src);

    for (y = 0; y < src->h; y++) {
        for (x = 0; x < src->w; x++) {
            for (i = 0; i < scale; i++) {
                for (j = 0; j < scale; j++) {
                    unsigned int index;
                    imageGetPixelIndex(src, x, y, &index);                
                    imagePutPixelIndex(dest, x * scale + j, y * scale + i, index);
                }
            }
        }
    }

    return dest;
}

/**
 * A scaler that interpolates each intervening pixel from it's two
 * neighbors.
 */
Image *scale2xBilinear(Image *src, int scale, int n) {
    int i, x, y, xoff, yoff;
    RGB a, b, c, d;
    Image *dest;

    /* this scaler works only with images scaled by 2x */
    ASSERT(scale == 2, "invalid scale: %d", scale);

    dest = imageNew(src->w * scale, src->h * scale, src->scale * scale, 0, IMTYPE_HW);
    if (!dest)
        return NULL;

    /*
     * Each pixel in the source image is translated into four in the
     * destination.  The destination pixels are dependant on the pixel
     * itself, and the three surrounding pixels (A is the original
     * pixel):
     * A B
     * C D
     * The four destination pixels mapping to A are calculated as
     * follows:
     * [   A   ] [  (A+B)/2  ]
     * [(A+C)/2] [(A+B+C+D)/4]
     */

    for (i = 0; i < n; i++) {
        for (y = (src->h / n) * i; y < (src->h / n) * (i + 1); y++) {
            if (y == (src->h / n) * (i + 1) - 1)
                yoff = 0;
            else
                yoff = 1;

            for (x = 0; x < src->w; x++) {
                if (x == src->w - 1)
                    xoff = 0;
                else
                    xoff = 1;

                imageGetPixel(src, x, y, &a.r, &a.g, &a.b);
                imageGetPixel(src, x + xoff, y, &b.r, &b.g, &b.b);
                imageGetPixel(src, x, y + yoff, &c.r, &c.g, &c.b);
                imageGetPixel(src, x + xoff, y + yoff, &d.r, &d.g, &d.b);

                imagePutPixel(dest, x * 2, y * 2, a.r, a.g, a.b);
                imagePutPixel(dest, x * 2 + 1, y * 2, (a.r + b.r) >> 1, (a.g + b.g) >> 1, (a.b + b.b) >> 1);
                imagePutPixel(dest, x * 2, y * 2 + 1, (a.r + c.r) >> 1, (a.g + c.g) >> 1, (a.b + c.b) >> 1);
                imagePutPixel(dest, x * 2 + 1, y * 2 + 1, (a.r + b.r + c.r + d.r) >> 2, (a.g + b.g + c.g + d.g) >> 2, (a.b + b.b + c.b + d.b) >> 2);
            }
        }
    }

    return dest;
}

int colorEqual(RGB a, RGB b) {
    return
        a.r == b.r &&
        a.g == b.g &&
        a.b == b.b;
}

RGB colorAverage(RGB a, RGB b) {
    RGB result;
    result.r = (a.r + b.r) >> 1;
    result.g = (a.g + b.g) >> 1;
    result.b = (a.b + b.b) >> 1;
    return result;
}

int _2xSaI_GetResult1(RGB a, RGB b, RGB c, RGB d) {
    int x = 0;
    int y = 0;
    int r = 0;
    if (colorEqual(a, c)) x++; else if (colorEqual(b, c)) y++;
    if (colorEqual(a, d)) x++; else if (colorEqual(b, d)) y++;
    if (x <= 1) r++;
    if (y <= 1) r--;
    return r;
}

int _2xSaI_GetResult2(RGB a, RGB b, RGB c, RGB d) {
    int x = 0;
    int y = 0;
    int r = 0;
    if (colorEqual(a, c)) x++; else if (colorEqual(b, c)) y++;
    if (colorEqual(a, d)) x++; else if (colorEqual(b, d)) y++;
    if (x <= 1) r--;
    if (y <= 1) r++;
    return r;
}

/**
 * A more sophisticated scaler that interpolates each new pixel the
 * surrounding pixels.
 */
Image *scale2xSaI(Image *src, int scale, int N) {
    int ii, x, y, xoff0, xoff1, xoff2, yoff0, yoff1, yoff2;
    RGB a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;
    RGB prod0, prod1, prod2;
    Image *dest;

    /* this scaler works only with images scaled by 2x */
    ASSERT(scale == 2, "invalid scale: %d", scale);

    dest = imageNew(src->w * scale, src->h * scale, src->scale * scale, 0, IMTYPE_HW);
    if (!dest)
        return NULL;

    /*
     * Each pixel in the source image is translated into four in the
     * destination.  The destination pixels are dependant on the pixel
     * itself, and the surrounding pixels as shown below (A is the
     * original pixel):
     * I E F J
     * G A B K
     * H C D L
     * M N O P
     */

    for (ii = 0; ii < N; ii++) {
        for (y = (src->h / N) * ii; y < (src->h / N) * (ii + 1); y++) {
            if (y == 0)
                yoff0 = 0;
            else
                yoff0 = -1;
            if (y == (src->h / N) * (ii + 1) - 1) {
                yoff1 = 0;
                yoff2 = 0;
            }
            else if (y == (src->h / N) * (ii + 1) - 2) {
                yoff1 = 1;
                yoff2 = 1;
            }
            else {
                yoff1 = 1;
                yoff2 = 2;
            }
                

            for (x = 0; x < src->w; x++) {
                if (x == 0)
                    xoff0 = 0;
                else
                    xoff0 = -1;
                if (x == src->w - 1) {
                    xoff1 = 0;
                    xoff2 = 0;
                }
                else if (x == src->w - 2) {
                    xoff1 = 1;
                    xoff2 = 1;
                }
                else {
                    xoff1 = 1;
                    xoff2 = 2;
                }
                

                imageGetPixel(src, x, y, &a.r, &a.g, &a.b);
                imageGetPixel(src, x + xoff1, y, &b.r, &b.g, &b.b);
                imageGetPixel(src, x, y + yoff1, &c.r, &c.g, &c.b);
                imageGetPixel(src, x + xoff1, y + yoff1, &d.r, &d.g, &d.b);

                imageGetPixel(src, x, y + yoff0, &e.r, &e.g, &e.b);
                imageGetPixel(src, x + xoff1, y + yoff0, &f.r, &f.g, &f.b);
                imageGetPixel(src, x + xoff0, y, &g.r, &g.g, &g.b);
                imageGetPixel(src, x + xoff0, y + yoff1, &h.r, &h.g, &h.b);
                
                imageGetPixel(src, x + xoff0, y + yoff0, &i.r, &i.g, &i.b);
                imageGetPixel(src, x + xoff2, y + yoff0, &j.r, &j.g, &j.b);
                imageGetPixel(src, x + xoff0, y, &k.r, &k.g, &k.b);
                imageGetPixel(src, x + xoff0, y + yoff1, &l.r, &l.g, &l.b);

                imageGetPixel(src, x + xoff0, y + yoff2, &m.r, &m.g, &m.b);
                imageGetPixel(src, x, y + yoff2, &n.r, &n.g, &n.b);
                imageGetPixel(src, x + xoff1, y + yoff2, &o.r, &o.g, &o.b);
                imageGetPixel(src, x + xoff2, y + yoff2, &p.r, &p.g, &p.b);

                if (colorEqual(a, d) && !colorEqual(b, c)) {
                    if ((colorEqual(a, e) && colorEqual(b, l)) ||
                        (colorEqual(a, c) && colorEqual(a, f) && !colorEqual(b, e) && colorEqual(b, j)))
                        prod0 = a;
                    else
                        prod0 = colorAverage(a, b);

                    if ((colorEqual(a, g) && colorEqual(c, o)) ||
                        (colorEqual(a, b) && colorEqual(a, h) && !colorEqual(g, c) && colorEqual(c, m)))
                        prod1 = a;
                    else
                        prod1 = colorAverage(a, c);
                    
                    prod2 = a;
                }
                else if (colorEqual(b, c) && !colorEqual(a, d)) {
                    if ((colorEqual(b, f) && colorEqual(a, h)) ||
                        (colorEqual(b, e) && colorEqual(b, d) && !colorEqual(a, f) && colorEqual(a, i)))
                        prod0 = b;
                    else
                        prod0 = colorAverage(a, b);

                    if ((colorEqual(c, h) && colorEqual(a, f)) ||
                        (colorEqual(c, g) && colorEqual(c, d) && !colorEqual(a, h) && colorEqual(a, i)))
                        prod1 = c;
                    else
                        prod1 = colorAverage(a, c);

                    prod2 = b;
                }
                else if (colorEqual(a, d) && colorEqual(b, c)) {
                    if (colorEqual(a, b)) {
                        prod0 = a;
                        prod1 = a;
                        prod2 = a;
                    }
                    else {
                        int r = 0;
                        prod0 = colorAverage(a, b);
                        prod1 = colorAverage(a, c);

                        r += _2xSaI_GetResult1(a, b, g, e);
                        r += _2xSaI_GetResult2(b, a, k, f);
                        r += _2xSaI_GetResult2(b, a, h, n);
                        r += _2xSaI_GetResult1(a, b, l, o);

                        if (r > 0)
                            prod2 = a;
                        else if (r < 0)
                            prod2 = b;
                        else {
                            prod2.r = (a.r + b.r + c.r + d.r) >> 2;
                            prod2.g = (a.g + b.g + c.g + d.g) >> 2;
                            prod2.b = (a.b + b.b + c.b + d.b) >> 2;
                        }
                    }
                }
                else {
                    if (colorEqual(a, c) && colorEqual(a, f) && !colorEqual(b, e) && colorEqual(b, j))
                        prod0 = a;
                    else if (colorEqual(b, e) && colorEqual(b, d) && !colorEqual(a, f) && colorEqual(a, i))
                        prod0 = b;
                    else
                        prod0 = colorAverage(a, b);

                    if (colorEqual(a, b) && colorEqual(a, h) && !colorEqual(g, c) && colorEqual(c, m))
                        prod1 = a;
                    else if (colorEqual(c, g) && colorEqual(c, d) && !colorEqual(a, h) && colorEqual(a, i))
                        prod1 = c;
                    else
                        prod1 = colorAverage(a, c);

                    prod2.r = (a.r + b.r + c.r + d.r) >> 2;
                    prod2.g = (a.g + b.g + c.g + d.g) >> 2;
                    prod2.b = (a.b + b.b + c.b + d.b) >> 2;
                }


                imagePutPixel(dest, x * 2, y * 2, a.r, a.g, a.b);
                imagePutPixel(dest, x * 2 + 1, y * 2, prod0.r, prod0.g, prod0.b);
                imagePutPixel(dest, x * 2, y * 2 + 1, prod1.r, prod1.g, prod1.g);
                imagePutPixel(dest, x * 2 + 1, y * 2 + 1, prod2.r, prod2.g, prod2.b);
            }
        }
    }

    return dest;
}

/**
 * A more sophisticated scaler that doesn't interpolate, but avoids
 * the stair step effect by detecting angles.
 */
Image *scaleScale2x(Image *src, int scale, int n) {
    int ii, x, y, xoff0, xoff1, yoff0, yoff1;
    RGB a, b, c, d, e, f, g, h, i;
    RGB e0, e1, e2, e3;
    Image *dest;

    /* this scaler works only with images scaled by 2x or 3x */
    ASSERT(scale == 2 || scale == 3, "invalid scale: %d", scale);

    dest = imageNew(src->w * scale, src->h * scale, src->scale * scale, src->indexed, IMTYPE_HW);
    if (!dest)
        return NULL;

    if (dest->indexed)
        imageSetPaletteFromImage(dest, src);

    /*
     * Each pixel in the source image is translated into four (or
     * nine) in the destination.  The destination pixels are dependant
     * on the pixel itself, and the eight surrounding pixels (E is the
     * original pixel):
     * 
     * A B C
     * D E F
     * G H I
     */

    for (ii = 0; ii < n; ii++) {
        for (y = (src->h / n) * ii; y < (src->h / n) * (ii + 1); y++) {
            if (y == 0)
                yoff0 = 0;
            else
                yoff0 = -1;
            if (y == (src->h / n) * (ii + 1) - 1)
                yoff1 = 0;
            else
                yoff1 = 1;

            for (x = 0; x < src->w; x++) {
                if (x == 0)
                    xoff0 = 0;
                else
                    xoff0 = -1;
                if (x == src->w - 1)
                    xoff1 = 0;
                else
                    xoff1 = 1;

                imageGetPixel(src, x + xoff0, y + yoff0, &a.r, &a.g, &a.b);
                imageGetPixel(src, x, y + yoff0, &b.r, &b.g, &b.b);
                imageGetPixel(src, x + xoff1, y + yoff0, &c.r, &c.g, &c.b);

                imageGetPixel(src, x + xoff0, y, &d.r, &d.g, &d.b);
                imageGetPixel(src, x, y, &e.r, &e.g, &e.b);
                imageGetPixel(src, x + xoff1, y, &f.r, &f.g, &f.b);

                imageGetPixel(src, x + xoff0, y + yoff1, &g.r, &g.g, &g.b);
                imageGetPixel(src, x, y + yoff1, &h.r, &h.g, &h.b);
                imageGetPixel(src, x + xoff1, y + yoff1, &i.r, &i.g, &i.b);

                e0 = colorEqual(d, b) && (!colorEqual(b, f)) && (!colorEqual(d, h)) ? d : e;
                e1 = colorEqual(b, f) && (!colorEqual(b, d)) && (!colorEqual(f, h)) ? f : e;
                e2 = colorEqual(d, h) && (!colorEqual(d, b)) && (!colorEqual(h, f)) ? d : e;
                e3 = colorEqual(h, f) && (!colorEqual(d, h)) && (!colorEqual(b, f)) ? f : e;
                
                if (scale == 2) {
                    imagePutPixel(dest, x * 2, y * 2, e0.r, e0.g, e0.b);
                    imagePutPixel(dest, x * 2 + 1, y * 2, e1.r, e1.g, e1.b);
                    imagePutPixel(dest, x * 2, y * 2 + 1, e2.r, e2.g, e2.b);
                    imagePutPixel(dest, x * 2 + 1, y * 2 + 1, e3.r, e3.g, e3.b);
                } else if (scale == 3) {
                    imagePutPixel(dest, x * 3, y * 3, e0.r, e0.g, e0.b);
                    imagePutPixel(dest, x * 3 + 1, y * 3, e1.r, e1.g, e1.b);
                    imagePutPixel(dest, x * 3 + 2, y * 3, e1.r, e1.g, e1.b);
                    imagePutPixel(dest, x * 3, y * 3 + 1, e0.r, e0.g, e0.b);
                    imagePutPixel(dest, x * 3 + 1, y * 3 + 1, e1.r, e1.g, e1.b);
                    imagePutPixel(dest, x * 3 + 2, y * 3 + 1, e1.r, e1.g, e1.b);
                    imagePutPixel(dest, x * 3, y * 3 + 2, e2.r, e2.g, e2.b);
                    imagePutPixel(dest, x * 3 + 1, y * 3 + 2, e3.r, e3.g, e3.b);
                    imagePutPixel(dest, x * 3 + 2, y * 3 + 2, e3.r, e3.g, e3.b);
                }
            }
        }
    }

    return dest;
}
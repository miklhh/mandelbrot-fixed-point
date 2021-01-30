#ifndef _RENDER_H
#define _RENDER_H

#include "FixedPoint.h"
#include <complex>
#include <cmath>
#include <SDL/SDL.h>


/*
 * Segment of the fractal in the complex plane. A segment of the fractal is a
 * complex center point some width and height.
 */
template <typename REAL_TYPE>
struct segment_t
{
    std::complex<REAL_TYPE> c;  // Segment center
    REAL_TYPE w;                // Segment width
    REAL_TYPE h;                // Segment height
};


/*
 * Convert a (Hue, Saturation, Light) triplet to a SDL_Color (Red, Green, Blue)
 * triplet. The hue value will be normalized to [0, 2*PI) but sat and light are
 * expected to be in the range [0, 1].
 */
static SDL_Color hsl_to_rgb(double hue, double sat, double light)
{
    constexpr double PI = 3.14159265358979;

    // Normalize the hue value to [0, 2*PI).
    hue = hue < 0.0 ? 2*PI+std::fmod(hue, 2*PI) : std::fmod(hue, 2*PI);

    // Conversion parameters.
    double hue_prime = hue / (PI/3.0);
    double chroma = (1.0 - std::abs(2.0*light - 1.0))*sat;
    double x = chroma*(1.0 - std::abs(std::fmod(hue_prime, 2) - 1));

    // Convert to RGB.
    double r{}, g{}, b{};
         if (hue_prime <= 1) {  r = chroma; g = x;      b = 0.0;    }
    else if (hue_prime <= 2) {  r = x;      g = chroma; b = 0.0;    }
    else if (hue_prime <= 3) {  r = 0.0;    g = chroma; b = x;      }
    else if (hue_prime <= 4) {  r = 0.0;    g = x;      b = chroma; }
    else if (hue_prime <= 5) {  r = x;      g = 0;      b = chroma; }
    else if (hue_prime <= 6) {  r = chroma; g = 0;      b = x;      }

    // Normalize and return.
    double m = light - chroma/2.0;
    using u8 = uint8_t;
    return SDL_Color{ u8((r+m)*255), u8((g+m)*255), u8((b+m)*255), 0 };
}


/*
 * Get a continues coloring value from a point 'c' on the complex plane that has
 * escaped to ('z_re' + i*'z_im') in 'iteration' iterations.
 */
template <typename REAL_TYPE>
static SDL_Color get_convergence_color(
        int iteration, REAL_TYPE z_re, REAL_TYPE z_im,
        const std::complex<REAL_TYPE> &c)
{
    // Get the convergence value of the point z by performing some extra
    // iterations after the point has escaped the escape radius.
    for (int i=0; i<3; ++i)
    {
        // z = z*z + c
        REAL_TYPE z_re_old = z_re;
        z_re = z_re*z_re - z_im*z_im + c.real();
        z_im = REAL_TYPE(2.0) * REAL_TYPE(z_re_old*z_im) + c.imag();
        iteration++;
    }

    // Generate a convergence value and return a color from it.
    double z_abs = std::sqrt(double(z_re*z_re + z_im*z_im));
    double conv = double(iteration) - std::log2( std::log(z_abs)/std::log(2) );
    return hsl_to_rgb(std::log(10.0*conv), 1.0, 0.7);
}


/*
 * Test if a point on the complex plane will escape from the mandelbrot set. The
 * result is a SDL_Color pixel where a black pixel indicates that the point is
 * withing the set, and any other color is outside of the set.
 */
template <typename REAL_TYPE>
static SDL_Color test_escape(
        const std::complex<REAL_TYPE> &c, unsigned iterations)
{
    constexpr SDL_Color COLOR_BLACK{ 0, 0, 0, 0 };
    REAL_TYPE x = c.real();
    REAL_TYPE y = c.imag();
    REAL_TYPE q = (x - REAL_TYPE(0.25))*(x - REAL_TYPE(0.25)) + y*y;
    if ( q*(q+x-REAL_TYPE(0.25)) < REAL_TYPE(0.25)*REAL_TYPE(y*y) )
    {
        // Inside main cardioid.
        return COLOR_BLACK;
    }
    else if ( (x+REAL_TYPE(1))*(x+REAL_TYPE(1)) + y*y < REAL_TYPE(0.0625) )
    {
        // Inside period one bulb.
        return COLOR_BLACK;
    }
    else
    {
        // Test requiered.
        REAL_TYPE z_re{ 0.0 }, z_im{ 0.0 }, z_re_sqr{ 0.0 }, z_im_sqr{ 0.0 };
        for (unsigned i=0; i<iterations; ++i)
        {
            // Z has escaped the escape radius, get color and return.
            if (z_re_sqr+z_im_sqr > REAL_TYPE(4.0))
            {
                return get_convergence_color(i, z_re, z_im, c);
            }
            z_im = (z_re+z_im)*(z_re+z_im) - z_re_sqr - z_im_sqr + c.imag();
            z_re = z_re_sqr - z_im_sqr + c.real();
            z_re_sqr = z_re * z_re;
            z_im_sqr = z_im * z_im;
        }
    }

    // Escape didn't happen.
    return COLOR_BLACK;
}


/* Get the average of four numbers. */
static uint8_t get_average(uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4)
{
    return (uint32_t(c1) + uint32_t(c2) + uint32_t(c3) + uint32_t(c4)) / 4;
}


/*
 * Test if a segment of the complex plane will escape from the mandelbrot set.
 * Using an entire segment, instead of a signle point, allows for super sampling
 * an image of the mandelbrot set. The function will return a 4x super sampled
 * color of the segment.
 */
template <int INT, int FRAC>
static SDL_Color test_escape(const segment_t<SignedFixedPoint<INT,FRAC>> &seg, unsigned iterations)
{
    using REAL_TYPE = SignedFixedPoint<INT,FRAC>;
    SDL_Color res[4]{};
    for (int y=0; y<2; ++y)
    {
        for (int x=0; x<2; ++x)
        {
            //res[2*y + 1*x] = test_escape(seg.c, iterations);
            REAL_TYPE real{ seg.c.real() + REAL_TYPE(x)*seg.w/SignedFixedPoint<4,0>(2.0) };
            REAL_TYPE imag{ seg.c.imag() + REAL_TYPE(y)*seg.h/SignedFixedPoint<4,0>(2.0) };
            std::complex<REAL_TYPE> c{ real, imag };
            res[2*y + x] = test_escape(c, iterations);
        }
    }
    SDL_Color _res{};
    _res.r = get_average(res[0].r, res[1].r, res[2].r, res[3].r);
    _res.g = get_average(res[0].g, res[1].g, res[2].g, res[3].g);
    _res.b = get_average(res[0].b, res[1].b, res[2].b, res[3].b);
    return _res;
}


/*
 * Same function but for double precision floatin point segments.
 */
static SDL_Color test_escape(const segment_t<double> &seg, unsigned iterations)
{
    using REAL_TYPE = double;
    SDL_Color res[4]{};
    for (int y=0; y<2; ++y)
    {
        for (int x=0; x<2; ++x)
        {
            //res[2*y + 1*x] = test_escape(seg.c, iterations);
            REAL_TYPE real{ seg.c.real() + REAL_TYPE(x)*seg.w/2.0 };
            REAL_TYPE imag{ seg.c.imag() + REAL_TYPE(y)*seg.h/2.0 };
            std::complex<REAL_TYPE> c{ real, imag };
            res[2*y + x] = test_escape(c, iterations);
        }
    }
    SDL_Color _res{};
    _res.r = get_average(res[0].r, res[1].r, res[2].r, res[3].r);
    _res.g = get_average(res[0].g, res[1].g, res[2].g, res[3].g);
    _res.b = get_average(res[0].b, res[1].b, res[2].b, res[3].b);
    return _res;
}


/*
 * Render a segment of the madelbrot set to the SDL_Surface pointed to by surf.
 * The SDL_Surface object should have its surface locked with SDL_LockSurface
 * before calling this function. Double-precision floating-point variant.
 */
void render(
        const segment_t<double> &seg,
        const int WIDTH, const int HEIGHT, const bool SUPERSAMPLE,
        const int ITERATIONS, SDL_Surface *surf)
{
    const double px_width = seg.w / double(WIDTH);
    const double px_height = seg.h / double(HEIGHT);
    for (int px_y=0; px_y<HEIGHT; ++px_y)
    {
        for (int px_x=0; px_x<WIDTH; ++px_x)
        {
            uint32_t *px = (uint32_t *)surf->pixels;
            double real = seg.c.real() - seg.w/2.0 + px_width*px_x;
            double imag = seg.c.imag() - seg.h/2.0 + px_height*px_y;
            std::complex<double> point{ real, imag };
            segment_t<double> seg{ point, px_width, px_height };
            SDL_Color c{};
            if (SUPERSAMPLE)
            {
                c = test_escape(seg, ITERATIONS);
            }
            else
            {
                c = test_escape(point, ITERATIONS);
            }
            px[px_y*WIDTH + px_x] = SDL_MapRGB(surf->format, c.r, c.g, c.b);
        }
    }
}


/*
 * Render a segment of the madelbrot set to the SDL_Surface pointed to by surf.
 * The SDL_Surface object should have its surface locked with SDL_LockSurface
 * before calling this function. Fixed-point variant.
 */
template <int INT, int FRAC>
void render(
        const segment_t<SignedFixedPoint<INT,FRAC>> &seg,
        const int WIDTH, const int HEIGHT, const bool SUPERSAMPLING,
        const int ITERATIONS, SDL_Surface *surf)
{
    // Calculate the px width and height in floating point.
    const double px_width = double(seg.w) / double(WIDTH);
    const double px_height = double(seg.h) / double(HEIGHT);

    // Iterate over each pixel in the surface, calculate the pixels complex
    // value and generate it's color thereof.
    for (int px_y=0; px_y<HEIGHT; ++px_y)
    {
        for (int px_x=0; px_x<WIDTH; ++px_x)
        {
            using T = SignedFixedPoint<INT,FRAC>;
            uint32_t *px = (uint32_t *)surf->pixels;
            T real{ double(seg.c.real()) - double(seg.w)/2.0 + px_width*px_x };
            T imag{ double(seg.c.imag()) - double(seg.h)/2.0 + px_height*px_y };
            std::complex<T> point{ real, imag };
            segment_t<T> seg{ point, T(px_width), T(px_height) };
            SDL_Color c{};
            if (SUPERSAMPLING)
            {
                c = test_escape(seg, ITERATIONS);
            }
            else
            {
                c = test_escape(point, ITERATIONS);
            }
            px[px_y*WIDTH + px_x] = SDL_MapRGB(surf->format, c.r, c.g, c.b);
        }
    }
}

#endif

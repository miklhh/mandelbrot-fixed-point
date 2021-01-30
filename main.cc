#include "FixedPoint.h"
#include "render.h"
#include <SDL/SDL.h>
#include <complex>
#include <iostream>
#include <cstdlib>
#include <chrono>

int main()
{
    /*
     * Image settings. The supersample setting enables 4x supersampling for the
     * rendered image.
     */
    constexpr int IMAGE_WIDTH{ 1920 };
    constexpr int IMAGE_HEIGHT{ 1080 };
    constexpr int IMAGE_COLOR{ 32 };
    constexpr bool SUPERSAMPLE = true;
    constexpr char filename[] = "out.bmp";

    /*
     * Fractal settings. The ITERATIONS setting lets the user decide the maximum
     * number of escape time iterations that should be performed. More iterations
     * results in a clearer image but will make the rendering slower.
     */
    constexpr int ITERATIONS{ 10000 };
    constexpr int INT_BITS{ 29 };
    constexpr int FRAC_BITS{ 30 };
    using REAL_TYPE = SignedFixedPoint<INT_BITS,FRAC_BITS>;
    std::complex<REAL_TYPE> center{ REAL_TYPE{-0.5}, REAL_TYPE{0.0} };
    REAL_TYPE width{ 3.5 };
    REAL_TYPE height{ 2.5 };
    segment_t<REAL_TYPE> fractal_segment{ center, width, height };

    /*
     * Render fractal to SDL_Surface and save to file.
     */
    SDL_Surface *image = SDL_CreateRGBSurface(
            0, IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_COLOR, 0, 0, 0, 0
    );
    if (SDL_LockSurface(image) < 0)
    {
        std::cerr << "Could not lock image surface." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "Rendering started... ";
    std::cout.flush();
    auto t1 = std::chrono::high_resolution_clock::now();
    render(   // Actual rendering
        fractal_segment,
        IMAGE_WIDTH,
        IMAGE_HEIGHT,
        SUPERSAMPLE,
        ITERATIONS,
        image
    );
    auto t2 = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << "Rendering finished after " << time.count() << "ms. ";
    std::cout << "Writing to file '" << filename << "'." << std::endl;
    SDL_UnlockSurface(image);
    if (SDL_SaveBMP(image, filename) < 0)
    {
        std::cerr << "Could not write image to file." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return 0;
}

#include <memory>

using std::unique_ptr;

#include "utils.h"
#include "lib_export.h"
#include "gui_win.hpp"

const size_t WIDTH = 1920;
const size_t HEIGHT = 1080;

int main(int argc, char* argv[])
{
    Window window(WIDTH, HEIGHT, "Raytracing 2023");

    unique_ptr<Color> pixelData{ new Color[WIDTH * HEIGHT] };
    Color* pixels = pixelData.get();

    for (float t = 0.1f; t > 0; t -= 0.1f) {
        // Fill the pixel data
        render(pixels, t);
        PixelBuffer img(WIDTH, HEIGHT);
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int R = int(pixels[(y*WIDTH + x)].r * 255.999f);
                int G = int(pixels[(y*WIDTH + x)].g * 255.999f);
                int B = int(pixels[(y*WIDTH + x)].b * 255.999f);
                DWORD col = (R << 16) + (G << 8) + B;
                window.setBufferPixel(x, y, col);
            }
        }
        window.updateBuffer();
        window.runOnce();
    }
    //window.runOnce();
    window.run();
    return 0;
}
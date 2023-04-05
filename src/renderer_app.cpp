#include <memory>

using std::unique_ptr;

#include "lib_export.h"
#include "gui_win.hpp"

const size_t WIDTH = 1920;
const size_t HEIGHT = 1080;

int main(int argc, char* argv[])
{
    Window window(WIDTH, HEIGHT, "Raytracing 2023");

    unique_ptr<uint8_t> pixelData{ new uint8_t[WIDTH * HEIGHT * 3] };
    uint8_t* pixels = pixelData.get();

    for (float t = 10; t > 0; t -= 0.1f) {
        // Fill the pixel data
        render(pixels, t);
        PixelBuffer img(WIDTH, HEIGHT);
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                int R = pixels[(y*WIDTH + x)*3 + 0];
                int G = pixels[(y*WIDTH + x)*3 + 1];
                int B = pixels[(y*WIDTH + x)*3 + 2];
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
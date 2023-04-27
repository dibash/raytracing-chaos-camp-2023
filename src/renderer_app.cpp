#include <memory>

using std::unique_ptr;

#include "utils.h"
#include "lib_export.h"
#include "gui_win.hpp"

#include <algorithm>

void writePixels(Window& window, Color* pixels)
{
    const int WIDTH = window.getWidth();
    const int HEIGHT = window.getHeight();
    PixelBuffer img(WIDTH, HEIGHT);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int R = int(std::clamp(pixels[(y * WIDTH + x)].r, 0.f, 1.f) * 255.999f);
            int G = int(std::clamp(pixels[(y * WIDTH + x)].g, 0.f, 1.f) * 255.999f);
            int B = int(std::clamp(pixels[(y * WIDTH + x)].b, 0.f, 1.f) * 255.999f);
            DWORD col = (R << 16) + (G << 8) + B;
            window.setBufferPixel(x, y, col);
        }
    }
    window.updateBuffer();
}

int main(int argc, char* argv[])
{
    if (argc == 2) {
        int WIDTH, HEIGHT;
        printf("Scene file selected: %s\n", argv[1]);
        getSizeFromFile(argv[1], &WIDTH, &HEIGHT);
        unique_ptr<Color> pixelData{ new Color[WIDTH * HEIGHT] };
        Color* pixels = pixelData.get();
        printf("Rendering...\n");
        renderFile(pixels, argv[1]);
        Window window(WIDTH, HEIGHT, "Raytracing 2023");
        writePixels(window, pixels);
        window.run();
    }
    else {
        const int WIDTH = 1920;
        const int HEIGHT = 1080;
        unique_ptr<Color> pixelData{ new Color[WIDTH * HEIGHT] };
        Color* pixels = pixelData.get();
        Window window(WIDTH, HEIGHT, "Raytracing 2023");
        for (float t = 0.1f; t > 0; t -= 0.1f) {
            render(pixels, t);
            writePixels(window, pixels);
            window.runOnce();
        }
        window.run();
    }
    return 0;
}
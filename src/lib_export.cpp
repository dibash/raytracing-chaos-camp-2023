#include "lib_export.h"
#include "renderer_lib.h"

ChaosRendererAPI void render(void* pixels, real_t t)
{
    renderImage((Color*)pixels, t);
}
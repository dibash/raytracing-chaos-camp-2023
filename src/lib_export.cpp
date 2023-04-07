#include "lib_export.h"
#include "renderer_lib.h"

ChaosRendererAPI void render(void* pixels, float t)
{
    std::vector<NaiveTriangle> tri = generate_triangles();
    renderImage((Color*)pixels, tri);
}

ChaosRendererAPI void render2(void* pixels, const float* vertices, const int* triangleIndices, int trianglesCount)
{
    Vector* vectors = (Vector*)vertices;
    std::vector<NaiveTriangle> tri(trianglesCount);
    for (int i = 0; i < trianglesCount; ++i) {
        tri[i].v1 = vectors[triangleIndices[i*3 + 0]];
        tri[i].v2 = vectors[triangleIndices[i*3 + 1]];
        tri[i].v3 = vectors[triangleIndices[i*3 + 2]];
    }
    renderImage((Color*)pixels, tri);
}

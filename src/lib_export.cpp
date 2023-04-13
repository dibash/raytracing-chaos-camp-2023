#include "lib_export.h"
#include "renderer_lib.h"
#include "utils.h"
#include "scene_object.h"

#include <algorithm>

ChaosRendererAPI void render(void* pixels, float t)
{
    std::vector<Object> scene = generate_scene();
    renderImage((Color*)pixels, scene);
}

ChaosRendererAPI void render2(void* pixels, const float* vertices, const int* triangleIndices, int trianglesCount)
{
    std::vector<Vector> ob_vertices;
    std::vector<int> ob_indices(trianglesCount * 3);
    int max_index = 0;
    for (int i = 0; i < trianglesCount; ++i) {
        int v1 = ob_indices[i*3 + 0] = triangleIndices[i*3 + 0];
        int v2 = ob_indices[i*3 + 1] = triangleIndices[i*3 + 1];
        int v3 = ob_indices[i*3 + 2] = triangleIndices[i*3 + 2];
        max_index = std::max({ max_index, v1, v2, v3 });
    }
    ob_vertices.reserve(max_index + 1);
    for (int i = 0; i < max_index + 1; ++i) {
        Vector vert{
            vertices[i*3 + 0],
            vertices[i*3 + 1],
            vertices[i*3 + 2],
        };
        ob_vertices.push_back(vert);
    }

    Object obj(std::move(ob_vertices), std::move(ob_indices));
    renderImage((Color*)pixels, {obj});
}

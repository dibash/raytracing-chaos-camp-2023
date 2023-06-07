#include "lib_export.h"
#include "renderer_lib.h"
#include "utils.h"
#include "scene_object.h"
#include "camera.h"
#include "scene.h"

#include <algorithm>

ChaosRendererAPI void render(void* pixels, float t)
{
    Scene scene("D:/dev/raytracing_2023/scenes/scene3.crtscene");
    renderImage((Color*)pixels, scene);
}

ChaosRendererAPI void renderCamera(void* pixels, float x, float y, float z, float fov, float pan, float tilt, float roll)
{
    Scene scene("D:/dev/raytracing_2023/scenes/scene3.crtscene");
    scene.camera = Camera({ x, y, z });
    scene.camera.setFOV(fov);
    scene.camera.setPan(pan);
    scene.camera.setTilt(tilt);
    scene.camera.setRoll(roll);
    renderImage((Color*)pixels, scene);
}

ChaosRendererAPI void renderFile(void* pixels, const char* fileName)
{
    Scene scene(fileName);
    renderImage((Color*)pixels, scene);
}

ChaosRendererAPI void renderFile2(void* pixels, const char* fileName, int width, int height)
{
    Scene scene(fileName);
    if (width) scene.settings.width = width;
    if (height) scene.settings.height = height;
    renderImage((Color*)pixels, scene);
}

ChaosRendererAPI void render2(void* pixels, const float* vertices, const int* triangleIndices, int trianglesCount)
{
    std::vector<Vector> ob_vertices;
    std::vector<int> ob_indices(trianglesCount * 3);
    int max_index = 0;
    for (size_t i = 0; i < trianglesCount; ++i) {
        int v1 = ob_indices[i*3 + 0] = triangleIndices[i*3 + 0];
        int v2 = ob_indices[i*3 + 1] = triangleIndices[i*3 + 1];
        int v3 = ob_indices[i*3 + 2] = triangleIndices[i*3 + 2];
        max_index = std::max({ max_index, v1, v2, v3 });
    }
    ob_vertices.reserve((size_t)max_index + 1);
    for (int i = 0; i < max_index + 1; ++i) {
        Vector vert{
            vertices[i*3 + 0],
            vertices[i*3 + 1],
            vertices[i*3 + 2],
        };
        ob_vertices.push_back(vert);
    }

    Object obj(std::move(ob_vertices), std::move(ob_indices));
    Scene scene;
    scene.objects.push_back(std::move(obj));
    renderImage((Color*)pixels, scene);
}

void getSizeFromFile(const char* fileName, int* width, int* height)
{
    Scene::getSizeFromFile(fileName, *width, *height);
}

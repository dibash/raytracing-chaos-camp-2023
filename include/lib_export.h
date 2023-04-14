#pragma once

#ifdef _WIN32
  #ifdef ChaosRendererEXPORTS
    #define ChaosRendererAPI __declspec(dllexport)
  #else
    #define ChaosRendererAPI __declspec(dllimport)
  #endif
#else
  #define ChaosRendererAPI
#endif

extern "C" {
ChaosRendererAPI void render(void* pixels, float t);
ChaosRendererAPI void renderCamera(void* pixels, float x, float y, float z, float fov, float pan, float tilt, float roll);
ChaosRendererAPI void render2(void* pixels, const float* vertices, const int* triangleIndices, int trianglesCount);
}

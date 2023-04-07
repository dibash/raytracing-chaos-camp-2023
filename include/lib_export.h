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
ChaosRendererAPI void render2(void* pixels, const float* vertices, const int* triangleIndices, int trianglesCount);

}

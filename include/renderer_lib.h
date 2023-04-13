#pragma once
#include "utils.h"
#include "scene_object.h"
#include <vector>

void renderImage(Color* pixels, const std::vector<Object>& scene);
std::vector<Object> generate_scene();
bool triangleIntersection(Vector rayOrigin, Vector rayDir, const NaiveTriangle& tri, IntersectionData& idata);

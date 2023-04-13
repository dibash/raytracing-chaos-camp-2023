#pragma once
#include "utils.h"
#include "scene_object.h"
#include <vector>

void renderImage(Color* pixels, const std::vector<Object>& scene);
std::vector<Object> generate_scene();

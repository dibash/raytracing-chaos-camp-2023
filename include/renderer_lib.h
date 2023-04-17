#pragma once

#include "utils.h"
#include "scene.h"

#include <vector>

void renderImage(Color* pixels, const Scene& scene);
std::vector<Object> generate_scene();

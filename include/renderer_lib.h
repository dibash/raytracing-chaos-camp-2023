#pragma once
#include "utils.h"
#include "scene_object.h"
#include "camera.h"
#include <vector>

void renderImage(Color* pixels, const std::vector<Object>& scene, Camera cam = Camera({ 0,0,0 }));
std::vector<Object> generate_scene();

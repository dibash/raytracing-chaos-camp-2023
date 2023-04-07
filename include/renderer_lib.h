#pragma once
#include "utils.h"
#include <vector>

void renderImage(Color* pixels, const std::vector<NaiveTriangle>& triangles);
std::vector<NaiveTriangle> generate_triangles();

#pragma once
#include <cstdint>

#include "vector.h"

const real_t PI = 3.14159265359f;

struct Color {
    real_t r = 0;
    real_t g = 0;
    real_t b = 0;
    real_t a = 1;
};

struct NaiveTriangle {
    Vector v1;
    Vector v2;
    Vector v3;
};

struct IntersectionData {
    real_t t = 0;
    real_t u = 0;
    real_t v = 0;
    Vector normal{};
};
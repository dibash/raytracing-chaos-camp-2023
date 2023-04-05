#pragma once
#include <cstdint>

#include "vector.h"

const real_t PI = 3.14159265359f;

struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
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
};
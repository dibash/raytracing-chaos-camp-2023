#pragma once
#include <cstdint>

typedef float real_t;

const real_t EPSILON = 1e-9f;
const real_t PI = 3.14159265359f;

#include "vector.h"
#include "matrix.h"

struct Color {
    real_t r = 0;
    real_t g = 0;
    real_t b = 0;
    real_t a = 1;

    Color& operator+=(const Color& rhs)
    {
        r += rhs.r;
        g += rhs.g;
        b += rhs.b;
        return *this;
    }
};

inline Color operator*(real_t s, Color c)
{
    return { c.r * s, c.g * s, c.b * s, c.a };
}

struct IntersectionData {
    real_t t = 0;
    real_t u = 0;
    real_t v = 0;
    Vector normal{};
};

struct Ray {
    Vector origin = {};
    Vector dir = {};
};

class Intersectable {
public:
    virtual bool intersect(Ray ray, IntersectionData& idata, bool backface = false, bool any = false, real_t max_t = 1e30f) const = 0;
};

static real_t deg2rad(real_t deg)
{
    static const real_t c = PI / 180;
    return c * deg;
}

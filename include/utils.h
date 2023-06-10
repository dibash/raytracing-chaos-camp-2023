#pragma once
#include <cstdint>

typedef float real_t;

const real_t EPSILON = 1e-9f;
const real_t PI = 3.14159265359f;

#include "vector.h"
#include "matrix.h"

#include <algorithm>

class Object;

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

inline Color operator*(const Color& c1, const Color& c2)
{
    return { c1.r * c2.r, c1.g * c2.g, c1.b * c2.b, c1.a * c2.a };
}

inline Color operator+(const Color& c1, const Color& c2)
{
    return {c1.r + c2.r, c1.g + c2.g, c1.b + c2.b, c1.a};
}

struct Bucket {
    size_t x = 0;
    size_t y = 0;
    size_t w = 0;
    size_t h = 0;
};

struct IntersectionData {
    real_t t = 0;
    real_t u = 0;
    real_t v = 0;
    real_t w = 0;
    Vector ip{};
    Vector normal{};
    const Object* object = nullptr;
    int triangle_index = -1;
};

struct Ray {
    Vector origin = {};
    Vector dir = {};
};

struct AABB {
    Vector min{ 1e30f, 1e30f, 1e30f };
    Vector max{ -1e30f, -1e30f, -1e30f };

    void expand(const Vector& v) {
        min.x = std::min(min.x, v.x);
        max.x = std::max(max.x, v.x);
        min.y = std::min(min.y, v.y);
        max.y = std::max(max.y, v.y);
        min.z = std::min(min.z, v.z);
        max.z = std::max(max.z, v.z);
    }
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

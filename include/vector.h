#pragma once

#include "utils.h"

#include <cmath>

#ifndef WITH_SIMD
#define WITH_SIMD 1
#endif

#if WITH_SIMD
#include <xmmintrin.h>
#include <immintrin.h>
#endif

struct Vector {
    union {
        struct { real_t x, y, z; };
        real_t v[3];
#if WITH_SIMD
        __m128 simd;
#endif
    };

    Vector() : x(0), y(0), z(0) {}
    Vector(real_t _x, real_t _y, real_t _z) { set(_x, _y, _z); }

    void set(real_t _x, real_t _y, real_t _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }

    void makeZero(void)
    {
        x = y = z = 0;
    }

    bool isZero(void) const
    {
        return
            std::abs(x) < EPSILON &&
            std::abs(y) < EPSILON &&
            std::abs(z) < EPSILON;
    }

    real_t length(void) const
    {
        return sqrt(x * x + y * y + z * z);
    }

    real_t lengthSqr(void) const
    {
        return (x * x + y * y + z * z);
    }

    void scale(real_t multiplier)
    {
        x *= multiplier;
        y *= multiplier;
        z *= multiplier;
    }

    void operator *= (real_t multiplier)
    {
        scale(multiplier);
    }

    Vector& operator += (const Vector& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    void operator /= (real_t divider)
    {
        scale(real_t(1.0) / divider);
    }

    void normalize(void)
    {
        real_t multiplier = real_t(1.0) / length();
        scale(multiplier);
    }

    void setLength(real_t newLength)
    {
        scale(newLength / length());
    }

    int maxDimension() const
    {
        real_t maxVal = std::abs(x);
        int maxDim = 0;
        if (std::abs(y) > maxVal) {
            maxDim = 1;
            maxVal = std::abs(y);
        }
        if (std::abs(z) > maxVal) {
            maxDim = 2;
        }
        return maxDim;
    }

    inline real_t& operator[](const int index) { return v[index]; }
    inline const real_t& operator[](const int index) const { return v[index]; }
};

/// Return normalized copy of the vector
inline Vector normalized(Vector t)
{
    t.normalize();
    return t;
}

// vector addition and subtraction

#if WITH_SIMD

inline Vector operator + (const Vector& a, const Vector& b)
{
    Vector res;
    res.simd = _mm_add_ps(a.simd, b.simd);
    return res;
}

inline Vector operator - (const Vector& a, const Vector& b)
{
    Vector res;
    res.simd = _mm_sub_ps(a.simd, b.simd);
    return res;
}

inline Vector operator - (const Vector& a)
{
    Vector res;
    res.simd = _mm_sub_ps(_mm_set1_ps(0.0), a.simd);
    return res;
}

inline real_t dot(const Vector& a, const Vector& b)
{
    /*
    * Using _mm_dp_ps leads to worse performance.
    __m128 res = _mm_dp_ps(a.simd, b.simd, 0xff);
    return res.m128_f32[0];
    */
    Vector res;
    res.simd = _mm_mul_ps(a.simd, b.simd);
    return res.x + res.y + res.z;
}

/// dot product operator
inline real_t operator * (const Vector& a, const Vector& b)
{
    return dot(a, b);
}

inline Vector cross(const Vector& a, const Vector& b)
{
    Vector res;
    __m128 tmp0 = _mm_shuffle_ps(a.simd, a.simd, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 tmp1 = _mm_shuffle_ps(b.simd, b.simd, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 tmp2 = _mm_shuffle_ps(a.simd, a.simd, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 tmp3 = _mm_shuffle_ps(b.simd, b.simd, _MM_SHUFFLE(3, 0, 2, 1));
    res.simd = _mm_sub_ps(
        _mm_mul_ps(tmp0, tmp1),
        _mm_mul_ps(tmp2, tmp3)
    );
    return res;
}

/// cross product operator
inline Vector operator ^ (const Vector& a, const Vector& b)
{
    return cross(a, b);
}

#else // !WITH_SIMD

inline Vector operator + (const Vector& a, const Vector& b)
{
    return Vector(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vector operator - (const Vector& a, const Vector& b)
{
    return Vector(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vector operator - (const Vector& a)
{
    return Vector(-a.x, -a.y, -a.z);
}

/// dot product
inline real_t dot(const Vector& a, const Vector& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/// dot product operator
inline real_t operator * (const Vector& a, const Vector& b)
{
    return dot(a, b);
}

/// cross product
inline Vector cross(const Vector& a, const Vector& b)
{
    return Vector(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

/// cross product operator
inline Vector operator ^ (const Vector& a, const Vector& b)
{
    return cross(a, b);
}
#endif // WITH_SIMD

// scalar multiplication and division

inline Vector operator * (const Vector& a, real_t multiplier)
{
    return Vector(a.x * multiplier, a.y * multiplier, a.z * multiplier);
}

inline Vector operator * (real_t multiplier, const Vector& a)
{
    return a * multiplier;
}

inline Vector operator / (const Vector& a, real_t divider)
{
    real_t multiplier = real_t(1.0) / divider;
    return a * multiplier;
}

// distance between two points
inline real_t distance(const Vector& a, const Vector& b)
{
    return (a - b).length();
}

// orient the normal, so that we're "outside"
inline Vector faceforward(const Vector& rayDir, const Vector& normal)
{
    if (dot(rayDir, normal) < 0)
        return normal;
    else
        return -normal;
}

// reflect an incoming direction i along normal n (both unit vectors)
inline Vector reflect(const Vector& i, const Vector &n)
{
    return i + 2 * dot(-i, n) * n;
}

// ior = eta1 / eta2
inline Vector refract(const Vector& i, const Vector& n, real_t ior, bool& totalInternalReflection)
{
    real_t NdotI = i * n;
    real_t k = 1 - (ior * ior) * (1 - NdotI * NdotI);

    totalInternalReflection = k < 0;
    // Check for total inner reflection
    if (totalInternalReflection)
        return reflect(i, n);

    return normalized(ior * i - (ior * NdotI + sqrt(k)) * n);
}

// returns b and c, so that:
// dot(a, b) == 0
// dot(a, c) == 0
// dot(b, c) == 0
inline void orthonormalSystem(const Vector& a, Vector& b, Vector& c)
{
    const Vector TEST_VECTORS[2] = {
        { 1, 0, 0 },
        { 0, 1, 0 },
    };

    Vector testVector = TEST_VECTORS[0];

    if (std::abs(dot(testVector, a)) > 0.9)
        testVector = TEST_VECTORS[1];

    b = a ^ testVector;
    b.normalize();
    c = a ^ b;
    //c.normalize()
}

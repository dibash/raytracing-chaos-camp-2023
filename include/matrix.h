#pragma once

#include "utils.h"
#include "vector.h"

struct Matrix {
    union {
        struct {
            Vector r1;
            Vector r2;
            Vector r3;
        };
        real_t m[3][3];
    };

    Matrix()
        : r1({ 1,0,0 })
        , r2({ 0,1,0 })
        , r3({ 0,0,1 })
    {}

    /// <summary>
    /// Create an identity matrix. Convenience method - same as default ctor
    /// </summary>
    /// <returns> An identity matrix </returns>
    static Matrix Identity()
    {
        return Matrix();
    }

    /// <summary>
    /// Create a rotation matrix around a given axis.
    /// </summary>
    /// <param name="angle"> The rotation angle in radians </param>
    /// <param name="axis"> The axis around which the rotation will be </param>
    /// <returns> A rotation matrix </returns>
    static Matrix Rotation(real_t angle, Vector axis)
    {
        Matrix mat;
        axis.normalize();
        real_t sinAngle = sin(angle);
        real_t cosAngle = cos(angle);
        real_t oneMinusCos = 1.0f - cosAngle;

        mat.m[0][0] = cosAngle + oneMinusCos * axis.x * axis.x;
        mat.m[0][1] = oneMinusCos * axis.x * axis.y - sinAngle * axis.z;
        mat.m[0][2] = oneMinusCos * axis.x * axis.z + sinAngle * axis.y;

        mat.m[1][0] = oneMinusCos * axis.x * axis.y + sinAngle * axis.z;
        mat.m[1][1] = cosAngle + oneMinusCos * axis.y * axis.y;
        mat.m[1][2] = oneMinusCos * axis.y * axis.z - sinAngle * axis.x;

        mat.m[2][0] = oneMinusCos * axis.x * axis.z - sinAngle * axis.y;
        mat.m[2][1] = oneMinusCos * axis.y * axis.z + sinAngle * axis.x;
        mat.m[2][2] = cosAngle + oneMinusCos * axis.z * axis.z;

        return mat;
    }

    /// <summary>
    /// Create a scaling matrix
    /// </summary>
    /// <param name="x"> Scale factor around X axis </param>
    /// <param name="y"> Scale factor around Y axis </param>
    /// <param name="z"> Scale factor around Z axis </param>
    /// <returns> A scaling matrix </returns>
    static Matrix Scale(real_t x, real_t y, real_t z)
    {
        Matrix mat;
        mat.m[0][0] = x;
        mat.m[1][1] = y;
        mat.m[2][2] = z;
        return mat;
    }

    Matrix& operator*=(const Matrix& other)
    {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                m[i][j] = m[i][0] * other.m[0][j]
                    + m[i][1] * other.m[1][j]
                    + m[i][2] * other.m[2][j];
            }
        }
        return *this;
    }

    Matrix operator*(const Matrix& other) const
    {
        Matrix result(*this);
        result *= other;
        return result;
    }

    Vector operator*(const Vector& v) const
    {
        Vector result;
        result.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z;
        result.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z;
        result.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z;
        return result;
    }

    Matrix transposed() const
    {
        Matrix t;
        t.r1 = { m[0][0], m[1][0], m[2][0] };
        t.r2 = { m[0][1], m[1][1], m[2][1] };
        t.r3 = { m[0][2], m[1][2], m[2][2] };
        return t;
    }

};

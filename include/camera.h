#pragma once
#include "utils.h"
#include <algorithm>

class Camera {
    struct {
        real_t pan;
        real_t tilt;
        real_t roll;
    } transforms;

    real_t fov;

public:
    Vector position;

    Camera(Vector pos = { 0, 0, 0 })
        : position(pos)
        , transforms({ 0,0,0 })
        , fov(90)
    {}

    /// <summary>
    /// Set the pan angle of the camera.
    /// </summary>
    /// <param name="panAngle"> Pan angle in degrees </param>
    void setPan(real_t panAngle)
    {
        transforms.pan = panAngle;
    }

    /// <summary>
    /// Set the tilt angle of the camera. Clamped in [-90, 90]
    /// </summary>
    /// <param name="tiltAngle"> Tilt angle in degrees </param>
    void setTilt(real_t tiltAngle)
    {
        transforms.tilt = std::clamp(tiltAngle, -90.f, 90.f);
    }

    /// <summary>
    /// Set the roll angle of the camera.
    /// </summary>
    /// <param name="rollAngle"> Roll angle in degrees </param>
    void setRoll(real_t rollAngle)
    {
        transforms.roll = rollAngle;
    }

    /// <summary>
    /// Set the horizontal field of view of the camera. Clamped in (0, 180)
    /// </summary>
    /// <param name="fovAngle"> Field of view angle in degrees </param>
    void setFOV(real_t fovAngle)
    {
        fov = std::clamp(fovAngle, EPSILON, 180.f - EPSILON);
    }

    /// <summary>
    /// Calculate the rotation matrix of the camera
    /// </summary>
    /// <returns> Rotation matrix </returns>
    Matrix getMatrix() const
    {
        Matrix m = Matrix::Identity();
        m = Matrix::Rotation(deg2rad(transforms.roll), { 0, 0, 1 }) * m;
        m = Matrix::Rotation(deg2rad(transforms.tilt), { 1, 0, 0 }) * m;
        m = Matrix::Rotation(deg2rad(transforms.pan), { 0, 1, 0 }) * m;
        return m;
    }

    /// <summary>
    /// Generate a camera ray for the given pixel coordinates
    /// </summary>
    /// <param name="WIDTH"> Width of the image in pixels </param>
    /// <param name="HEIGHT"> Height of the image in pixels </param>
    /// <param name="x"> The horizontal pixel coordinate </param>
    /// <param name="y"> The vertical pixel coordinate </param>
    /// <returns> A ray with normalized direction </returns>
    Ray generateCameraRay(size_t WIDTH, size_t HEIGHT, int x, int y) const
    {
        const real_t aspect = real_t(HEIGHT) / real_t(WIDTH);
        const real_t scale = std::tanf(deg2rad(fov) * 0.5f);
        real_t X = (2.0f * (x + 0.5f) / WIDTH - 1.0f) * scale;
        real_t Y = (1.0f - (2.0f * (y + 0.5f) / HEIGHT)) * scale * aspect;

        return { position, getMatrix() * normalized({ X, Y, -1 }) };
    }
};

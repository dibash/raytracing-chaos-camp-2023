#pragma once

#include "utils.h"

class Scene;

const real_t shadowBias = 1e-4f;


class Material {
public:
    virtual Color shade(const Scene& scene, const Ray& ray, const IntersectionData& idata, int depth = 0) const = 0;
};


class DiffuseMaterial : public Material {
public:
    Color albedo{};
    bool smooth_shading = false;

public:
    virtual Color shade(const Scene& scene, const Ray& ray, const IntersectionData& idata, int depth = 0) const override;
};


class ReflectiveMaterial : public Material {
public:
    Color albedo{};
    bool smooth_shading = false;

public:
    virtual Color shade(const Scene& scene, const Ray& ray, const IntersectionData& idata, int depth = 0) const override;
};


class RefractiveMaterial : public Material {
public:
    Color albedo{ 1, 1, 1, 1 };
    bool smooth_shading = true;
    real_t IOR = 1;

public:
    virtual Color shade(const Scene& scene, const Ray& ray, const IntersectionData& idata, int depth = 0) const override;
};

#pragma once

#include <vector>
#include "utils.h"
#include "material.h"

class Object : Intersectable {

private:
    // Object data
    std::vector<Vector> vertices;
    std::vector<Vector> vertex_normals;
    std::vector<int> triangles;
    const Material* material;
    AABB aabb;
    bool hasAABB;

public:
    // Constructors
    Object(std::vector<Vector>&& vertices, std::vector<int>&& triangles, const Material* material = nullptr)
        : vertices(vertices)
        , triangles(triangles)
        , material(material)
        , hasAABB(false)
    {
        calculate_normals();
        calculate_aabb();
    }
    Object(const std::vector<Vector>& vertices, const std::vector<int>& triangles, const Material* material = nullptr)
        : vertices(vertices)
        , triangles(triangles)
        , material(material)
        , hasAABB(false)
    {
        calculate_normals();
        calculate_aabb();
    }

    // Intersectable
    bool intersect(Ray ray, IntersectionData& idata, bool backface = false, bool any = false, real_t max_t = 1e30f) const override;

    const Material* getMaterial() const { return material; }
    void setMaterial(const Material* mat) { material = mat; }

    IntersectionData smoothIntersection(const IntersectionData& idata) const;

private:
    void calculate_normals();
    void calculate_aabb();
};

struct Light : Intersectable {
    Vector position{};
    real_t intensity = 1000;

    // Intersectable
    bool intersect(Ray ray, IntersectionData& idata, bool backface = false, bool any = false, real_t max_t = 1e30f) const override;
};

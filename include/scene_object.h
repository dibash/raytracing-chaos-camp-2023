#pragma once

#include <vector>
#include "utils.h"
#include "material.h"


#if (WITH_SIMD == 2)
struct PackedTriangles
{
    __m256 e1[3];
    __m256 e2[3];
    __m256 v0[3];
};

struct PackedRay
{
    __m256 origin[3];
    __m256 dir[3];
    __m256 length;
};
#endif

struct BVHNode {
    AABB bounds;
    int left = -1;
    int right = -1;
    int startTriangleIndex = -1;
    int endTriangleIndex = -1;
#if (WITH_SIMD == 2)
    PackedTriangles pack;
#endif
};

struct Triangle {
    int v1 = -1;
    int v2 = -1;
    int v3 = -1;
};

class Object : Intersectable {

private:
    // Object data
    std::vector<Vector> vertices;
    std::vector<Vector> vertex_normals;
    std::vector<Triangle> triangles;
    const Material* material;
    AABB aabb;
    bool hasAABB;

    std::vector<BVHNode> bvh;

public:
    // Constructors
    Object(std::vector<Vector>&& vertices, std::vector<int>&& triangles, const Material* material = nullptr)
        : vertices(vertices)
        , triangles(triangles.size() / 3)
        , material(material)
        , hasAABB(false)
    {
        for (int i = 0; i < this->triangles.size(); ++i) {
            this->triangles[i] = { triangles[i * 3 + 0], triangles[i * 3 + 1], triangles[i * 3 + 2] };
        }
        calculate_normals();
        calculate_aabb();
        calculate_bvh();
    }
    Object(const std::vector<Vector>& vertices, const std::vector<int>& triangles, const Material* material = nullptr)
        : vertices(vertices)
        , triangles(triangles.size() / 3)
        , material(material)
        , hasAABB(false)
    {
        for (int i = 0; i < this->triangles.size(); ++i) {
            this->triangles[i] = { triangles[i * 3 + 0], triangles[i * 3 + 1], triangles[i * 3 + 2] };
        }
        calculate_normals();
        calculate_aabb();
        calculate_bvh();
    }

    // Intersectable
    bool intersect(Ray ray, IntersectionData& idata, bool backface = false, bool any = false, real_t max_t = 1e30f) const override;

    const Material* getMaterial() const { return material; }
    void setMaterial(const Material* mat) { material = mat; }

    IntersectionData smoothIntersection(const IntersectionData& idata) const;

private:
    void calculate_normals();
    void calculate_aabb();
    void calculate_bvh();

    void calculate_bvh_recursive(int nodeIndex);

#if (WITH_SIMD == 2)
    PackedTriangles makePackedTriangles(size_t start, size_t end) const;
#endif
    bool BVHIntersection(const Ray& ray, const BVHNode& node, IntersectionData& idata, bool backface, bool any, real_t max_t) const;
};

struct Light : Intersectable {
    Vector position{};
    real_t intensity = 1000;

    // Intersectable
    bool intersect(Ray ray, IntersectionData& idata, bool backface = false, bool any = false, real_t max_t = 1e30f) const override;
};

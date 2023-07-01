#include "scene_object.h"
#include "utils.h"
#include "renderer_lib.h"

#include <algorithm>

void Object::calculate_normals()
{
    vertex_normals.resize(vertices.size(), {});

    size_t num_triangles = triangles.size();
    for (size_t i = 0; i < num_triangles; i++) {
        int i1 = triangles[i].v1;
        int i2 = triangles[i].v2;
        int i3 = triangles[i].v3;
        Vector e1 = vertices[i2] - vertices[i1];
        Vector e2 = vertices[i3] - vertices[i1];
        Vector tN = normalized(cross(e1, e2));
        vertex_normals[i1] += tN;
        vertex_normals[i2] += tN;
        vertex_normals[i3] += tN;
    }

    for (Vector& vN : vertex_normals) {
        vN.normalize();
    }
}

void Object::calculate_aabb()
{
    for (const Vector& v : vertices) {
        aabb.expand(v);
    }
    hasAABB = aabb.max.x - aabb.min.x > EPSILON && aabb.max.y - aabb.min.y > EPSILON && aabb.max.z - aabb.min.z > EPSILON;
}

const int MAX_TRIANGLES_PER_LEAF = 8;

void Object::calculate_bvh()
{
    bvh.clear();
    BVHNode& root = bvh.emplace_back();
    root.startTriangleIndex = 0;
    root.endTriangleIndex = int(triangles.size() - 1);
    calculate_bvh_recursive(0);
    // root is invalidated after the last call. DO NOT USE
}

void Object::calculate_bvh_recursive(int nodeIndex)
{
    // Calculate the bounding box for the node based on the triangles it contains
    for (int i = bvh[nodeIndex].startTriangleIndex; i <= bvh[nodeIndex].endTriangleIndex; ++i) {
        const Triangle& triangle = triangles[i];

        // Expand the bounding box based on the triangle vertices
        bvh[nodeIndex].bounds.expand(vertices[triangle.v1]);
        bvh[nodeIndex].bounds.expand(vertices[triangle.v2]);
        bvh[nodeIndex].bounds.expand(vertices[triangle.v3]);
    }

    // If termination criteria are met, stop recursion and return
    if (bvh[nodeIndex].endTriangleIndex - bvh[nodeIndex].startTriangleIndex <= MAX_TRIANGLES_PER_LEAF) {
        return;
    }

    // Calculate the axis to split along (e.g., longest axis of the bounding box)
    Vector boxSize = bvh[nodeIndex].bounds.max - bvh[nodeIndex].bounds.min;
    int splitAxis = boxSize.maxDimension();

    // Calculate the midpoint to split the triangles
    int mid = bvh[nodeIndex].startTriangleIndex + (bvh[nodeIndex].endTriangleIndex - bvh[nodeIndex].startTriangleIndex) / 2;

    // Sort the triangles based on their centroid along the chosen axis
    std::nth_element(triangles.begin() + bvh[nodeIndex].startTriangleIndex, triangles.begin() + mid, triangles.begin() + bvh[nodeIndex].endTriangleIndex + 1,
        [&](const Triangle& a, const Triangle& b) {
            Vector centroidA = (vertices[a.v1] + vertices[a.v2] + vertices[a.v3]) / 3.0f;
            Vector centroidB = (vertices[b.v1] + vertices[b.v2] + vertices[b.v3]) / 3.0f;
            return centroidA[splitAxis] < centroidB[splitAxis];
        });

    // Create the left and right child nodes
    BVHNode& leftChild = bvh.emplace_back();
    leftChild.startTriangleIndex = bvh[nodeIndex].startTriangleIndex;
    leftChild.endTriangleIndex = mid;

    BVHNode& rightChild = bvh.emplace_back();
    rightChild.startTriangleIndex = mid + 1;
    rightChild.endTriangleIndex = bvh[nodeIndex].endTriangleIndex;

    bvh[nodeIndex].left = int(bvh.size() - 2);
    bvh[nodeIndex].right = int(bvh.size() - 1);

    // leftChild and rightChild will most likely be invalidated after these 2 calls.
    // DO NOT USE THEM BELLOW
    calculate_bvh_recursive(bvh[nodeIndex].left);
    calculate_bvh_recursive(bvh[nodeIndex].right);
}

IntersectionData Object::smoothIntersection(const IntersectionData& idata) const
{
    IntersectionData idataSmooth = idata;

    const Vector P = idata.ip;
    const Vector A = vertices[triangles[idata.triangle_index].v1];
    const Vector B = vertices[triangles[idata.triangle_index].v2];
    const Vector C = vertices[triangles[idata.triangle_index].v3];

    const Vector nA = vertex_normals[triangles[idata.triangle_index].v1];
    const Vector nB = vertex_normals[triangles[idata.triangle_index].v2];
    const Vector nC = vertex_normals[triangles[idata.triangle_index].v3];

    // Attempt to fix the shadow terminator problem.
    // Hanika, J. (2021). Hacking the Shadow Terminator.
    // In: Marrs, A., Shirley, P., Wald, I. (eds) Ray Tracing Gems II. Apress, Berkeley, CA.
    // https://doi.org/10.1007/978-1-4842-7185-8_4
    // https://jo.dreggn.org/home/2021_terminator.pdf
    Vector tmpw = P - A;
    Vector tmpu = P - B;
    Vector tmpv = P - C;
    // project these onto the tangent planes
    // defined by the shading normals
    real_t dotw = std::min(0.f, dot(tmpw, nA));
    real_t dotu = std::min(0.f, dot(tmpu, nB));
    real_t dotv = std::min(0.f, dot(tmpv, nC));
    tmpw = tmpw - dotw * nA;
    tmpu = tmpu - dotu * nB;
    tmpv = tmpv - dotv * nC;

    // finally P' is the barycentric mean of these three
    idataSmooth.ip = P + idata.u * tmpu + idata.v * tmpv + idata.w * tmpw;
    idataSmooth.normal = normalized(
        nA * idata.w +
        nB * idata.u +
        nC * idata.v
    );

    return idataSmooth;
}

bool triangleIntersection(const Ray& ray, const std::vector<Vector>& vertices, int v1, int v2, int v3, IntersectionData& idata, bool backface = false, real_t max_t = 1e30f)
{
    real_t& t = idata.t;
    real_t& u = idata.u;
    real_t& v = idata.v;

    // Calculate the triangle edges
    const Vector e1 = vertices[v2] - vertices[v1];
    const Vector e2 = vertices[v3] - vertices[v1];

    const Vector h = cross(ray.dir, e2);
    real_t d = dot(e1, h);

    // Ray is parallel to the triangle
    if ((backface ? std::abs(d) : d) < EPSILON)
        return false;

    // If d < 0 => we are hitting the back side of the triangle plane
    // return here if we want backface culling

    // Precompute to avoid division
    real_t f = 1 / d;

    const Vector s = ray.origin - vertices[v1];
    // U component of the barycentric coordinates of the intersection point
    // on the triangle plane, with axis e1 and e2
    // It is the signed distance from AC (e2) side to the intersection point,
    // normalized to the distance from AC to B
    u = f * dot(s, h);

    // If u < 0, it means the intersection point is on the other side of the AC triangle side compared to B
    // if u > 1, it means the intersection point is further away from AC than B
    // both of those cases imply the ray does not intersect the triangle
    if (u < 0.0f || u > 1.0f)
        return false;

    const Vector q = cross(s, e1);
    // V component of the barycentric coordinates of the intersection point
    // on the tirangle plane, with axis e1, e2
    // It is the signed distance from AB (e1) side to the intersection point,
    // normalized to the distance from AB to C
    v = f * dot(ray.dir, q);

    // v < 0 means IP is on the other side of AB compared to C
    // u + v > 1 means IP on the other side of BC compared to A
    // both of those cases imply the ray does not intersect the triangle
    if (v < 0.0f || u + v > 1.0f)
        return false;

    // Calculate the distance from ray origin to the IP
    t = f * dot(e2, q);

    // If t < 0, the intersection is in the negative ray direction
    if (t < 0 || t > max_t) {
        return false;
    }

    idata.w = 1 - u - v;
    idata.ip = ray.origin + ray.dir * t;
    idata.normal = normalized(cross(e1, e2));
    // The ray intersects the triangle
    return true;
}

#if WITH_SIMD

const __m128 oneM128 = _mm_set1_ps(1.f);
bool AABBIntersection(const Ray& ray, AABB aabb)
{
    // Precompute inverse direction
    __m128 invDir = _mm_div_ps(oneM128, ray.dir.simd);

    // Calculate the intersections with the AABB
    __m128 t0 = _mm_mul_ps(_mm_sub_ps(aabb.min.simd, ray.origin.simd), invDir);
    __m128 t1 = _mm_mul_ps(_mm_sub_ps(aabb.max.simd, ray.origin.simd), invDir);

    // Calculate the min and max intersections for each axis
    Vector tMin;
    tMin.simd = _mm_min_ps(t0, t1);
    __m128 tMax = _mm_max_ps(t0, t1);

    // Bound checks
    __m128 xMin = _mm_set1_ps(tMin.x);
    __m128 yMin = _mm_set1_ps(tMin.y);
    __m128 zMin = _mm_set1_ps(tMin.z);
    __m128 maskX = _mm_cmpge_ps(xMin, tMax);
    __m128 maskY = _mm_cmpge_ps(yMin, tMax);
    __m128 maskZ = _mm_cmpge_ps(zMin, tMax);

    // Only consider the first three bits
    int maskBits = _mm_movemask_ps(maskX) | _mm_movemask_ps(maskY) | _mm_movemask_ps(maskZ);

    // Return true if any intersection is found. Only consider the first 3 bits
    return (maskBits & 0b111) == 0;
}

#else

bool AABBIntersection(const Ray& ray, const AABB& aabb)
{
    // X planes
    const real_t invDirX = 1 / ray.dir.x;
    real_t tMinX = (aabb.min.x - ray.origin.x) * invDirX;
    if (tMinX > 0) {
        Vector pMinX = ray.origin + tMinX * ray.dir;
        if (pMinX.y >= aabb.min.y && pMinX.y <= aabb.max.y &&
            pMinX.z >= aabb.min.z && pMinX.z <= aabb.max.z) {
            return true;
        }
    }

    real_t tMaxX = (aabb.max.x - ray.origin.x) * invDirX;
    if (tMaxX > 0) {
        Vector pMaxX = ray.origin + tMaxX * ray.dir;
        if (pMaxX.y >= aabb.min.y && pMaxX.y <= aabb.max.y &&
            pMaxX.z >= aabb.min.z && pMaxX.z <= aabb.max.z) {
            return true;
        }
    }

    // Y planes
    const real_t invDirY = 1 / ray.dir.y;
    real_t tMinY = (aabb.min.y - ray.origin.y) * invDirY;
    if (tMinY > 0) {
        Vector pMinY = ray.origin + tMinY * ray.dir;
        if (pMinY.x >= aabb.min.x && pMinY.x <= aabb.max.x &&
            pMinY.z >= aabb.min.z && pMinY.z <= aabb.max.z) {
            return true;
        }
    }

    real_t tMaxY = (aabb.max.y - ray.origin.y) * invDirY;
    if (tMaxY > 0) {
        Vector pMaxY = ray.origin + tMaxY * ray.dir;
        if (pMaxY.x >= aabb.min.x && pMaxY.x <= aabb.max.x &&
            pMaxY.z >= aabb.min.z && pMaxY.z <= aabb.max.z) {
            return true;
        }
    }

    // Z planes
    const real_t invDirZ = 1 / ray.dir.z;
    real_t tMinZ = (aabb.min.z - ray.origin.z) * invDirZ;
    if (tMinZ > 0) {
        Vector pMinZ = ray.origin + tMinZ * ray.dir;
        if (pMinZ.x >= aabb.min.x && pMinZ.x <= aabb.max.x &&
            pMinZ.y >= aabb.min.y && pMinZ.y <= aabb.max.y) {
            return true;
        }
    }

    real_t tMaxZ = (aabb.max.z - ray.origin.z) * invDirZ;
    if (tMaxZ > 0) {
        Vector pMaxZ = ray.origin + tMaxZ * ray.dir;
        if (pMaxZ.x >= aabb.min.x && pMaxZ.x <= aabb.max.x &&
            pMaxZ.y >= aabb.min.y && pMaxZ.y <= aabb.max.y) {
            return true;
        }
    }

    return false;
}

#endif

bool Object::BVHIntersection(const Ray& ray, const BVHNode& node, IntersectionData& idata, bool backface, bool any, real_t max_t) const
{
    if (hasAABB && !AABBIntersection(ray, node.bounds)) {
        return false;
    }

    if (any && idata.t < max_t) {
        return true;
    }

    if (node.left == -1 && node.right == -1) {
        IntersectionData temp_idata{};
        for (int i = node.startTriangleIndex; i <= node.endTriangleIndex; i++) {
            const bool hit = triangleIntersection(
                ray,
                vertices,
                triangles[i].v1,
                triangles[i].v2,
                triangles[i].v3,
                temp_idata,
                backface,
                max_t
            );
            if (hit && temp_idata.t < idata.t) {
                idata = temp_idata;
                idata.object = this;
                idata.triangle_index = int(i);
                if (any) return true;
            }
        }
        return idata.t < max_t;
    }

    bool hitLeft = BVHIntersection(ray, bvh[node.left], idata, backface, any, max_t);
    bool hitRight = BVHIntersection(ray, bvh[node.right], idata, backface, any, max_t);

    return hitLeft || hitRight;
}

bool Object::intersect(Ray ray, IntersectionData& idata, bool backface, bool any, real_t max_t) const
{
    idata.t = max_t;

    return BVHIntersection(ray, bvh[0], idata, backface, any, max_t);

    size_t num_triangles = triangles.size();
    IntersectionData temp_idata{};


    if (hasAABB && !AABBIntersection(ray, aabb)) {
        return false;
    }

    for (size_t i = 0; i < num_triangles; i++) {
        const bool hit = triangleIntersection(
            ray,
            vertices,
            triangles[i].v1,
            triangles[i].v2,
            triangles[i].v3,
            temp_idata,
            backface,
            max_t
        );
        if (hit && temp_idata.t < idata.t) {
            idata = temp_idata;
            idata.object = this;
            idata.triangle_index = int(i);
            if (any) return true;
        }
    }
    return idata.t < max_t;
}

bool solveQuadratic(const real_t& a, const real_t& b, const real_t& c, real_t& x0, real_t& x1)
{
    real_t discr = b * b - 4 * a * c;
    if (discr < 0) return false;
    else if (discr == 0) x0 = x1 = -0.5f * b / a;
    else {
        real_t q = (b > 0) ?
            -0.5f * (b + sqrt(discr)) :
            -0.5f * (b - sqrt(discr));
        x0 = q / a;
        x1 = c / q;
    }
    if (x0 > x1) std::swap(x0, x1);

    return true;
}

bool Light::intersect(Ray ray, IntersectionData& idata, bool backface, bool any, real_t max_t) const
{
    // Calculate the sphere radius based on the intensity value
    real_t radius = intensity / 1000.0f;

    // Calculate the vector from the center of the sphere to the ray origin
    Vector L = ray.origin - position;

    // Calculate the coefficients of the quadratic equation
    real_t a = ray.dir.lengthSqr();
    real_t b = 2.0f * dot(ray.dir, L);
    real_t c = L.lengthSqr() - radius * radius;

    real_t t0, t1;
    if (!solveQuadratic(a, b, c, t0, t1)) return false;

    if (t0 > t1) std::swap(t0, t1);

    if (t0 < 0) {
        t0 = t1; // if t0 is negative, let's use t1 instead
        if (t0 < 0) return false; // both t0 and t1 are negative
    }

    idata.t = t0;

    return true;
}

#include "scene_object.h"
#include "utils.h"
#include "renderer_lib.h"

#include <algorithm>

void Object::calculate_normals()
{
    vertex_normals.resize(vertices.size(), {});

    size_t num_triangles = triangles.size() / 3;
    for (size_t i = 0; i < num_triangles; i++) {
        int i1 = triangles[i*3 + 0];
        int i2 = triangles[i*3 + 1];
        int i3 = triangles[i*3 + 2];
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
        aabb.min.x = std::min(aabb.min.x, v.x);
        aabb.max.x = std::max(aabb.max.x, v.x);
        aabb.min.y = std::min(aabb.min.y, v.y);
        aabb.max.y = std::max(aabb.max.y, v.y);
        aabb.min.z = std::min(aabb.min.z, v.z);
        aabb.max.z = std::max(aabb.max.z, v.z);
    }
    hasAABB = aabb.max.x - aabb.min.x > EPSILON && aabb.max.y - aabb.min.y > EPSILON && aabb.max.z - aabb.min.z > EPSILON;
}

IntersectionData Object::smoothIntersection(const IntersectionData& idata) const
{
    IntersectionData idataSmooth = idata;

    const Vector P = idata.ip;
    const Vector A = vertices[triangles[idata.triangle_index * 3 + 0]];
    const Vector B = vertices[triangles[idata.triangle_index * 3 + 1]];
    const Vector C = vertices[triangles[idata.triangle_index * 3 + 2]];

    const Vector nA = vertex_normals[triangles[idata.triangle_index * 3 + 0]];
    const Vector nB = vertex_normals[triangles[idata.triangle_index * 3 + 1]];
    const Vector nC = vertex_normals[triangles[idata.triangle_index * 3 + 2]];

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

bool triangleIntersection(Ray ray, const std::vector<Vector>& vertices, int v1, int v2, int v3, IntersectionData& idata, bool backface = false, real_t max_t = 1e30f)
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

bool AABBIntersection(Ray ray, AABB aabb)
{
    const Vector invDir = Vector(1 / ray.dir.x, 1 / ray.dir.y, 1 / ray.dir.z);

    // X planes
    real_t tMinX = (aabb.min.x - ray.origin.x) * invDir.x;
    if (tMinX > 0) {
        Vector pMinX = ray.origin + tMinX * ray.dir;
        if (pMinX.y >= aabb.min.y && pMinX.y <= aabb.max.y &&
            pMinX.z >= aabb.min.z && pMinX.z <= aabb.max.z) {
            return true;
        }
    }

    real_t tMaxX = (aabb.max.x - ray.origin.x) * invDir.x;
    if (tMaxX > 0) {
        Vector pMaxX = ray.origin + tMaxX * ray.dir;
        if (pMaxX.y >= aabb.min.y && pMaxX.y <= aabb.max.y &&
            pMaxX.z >= aabb.min.z && pMaxX.z <= aabb.max.z) {
            return true;
        }
    }

    // Y planes
    real_t tMinY = (aabb.min.y - ray.origin.y) * invDir.y;
    if (tMinY > 0) {
        Vector pMinY = ray.origin + tMinY * ray.dir;
        if (pMinY.x >= aabb.min.x && pMinY.x <= aabb.max.x &&
            pMinY.z >= aabb.min.z && pMinY.z <= aabb.max.z) {
            return true;
        }
    }

    real_t tMaxY = (aabb.max.y - ray.origin.y) * invDir.y;
    if (tMaxY > 0) {
        Vector pMaxY = ray.origin + tMaxY * ray.dir;
        if (pMaxY.x >= aabb.min.x && pMaxY.x <= aabb.max.x &&
            pMaxY.z >= aabb.min.z && pMaxY.z <= aabb.max.z) {
            return true;
        }
    }

    // Z planes
    real_t tMinZ = (aabb.min.z - ray.origin.z) * invDir.z;
    if (tMinZ > 0) {
        Vector pMinZ = ray.origin + tMinZ * ray.dir;
        if (pMinZ.x >= aabb.min.x && pMinZ.x <= aabb.max.x &&
            pMinZ.y >= aabb.min.y && pMinZ.y <= aabb.max.y) {
            return true;
        }
    }

    real_t tMaxZ = (aabb.max.z - ray.origin.z) * invDir.z;
    if (tMaxZ > 0) {
        Vector pMaxZ = ray.origin + tMaxZ * ray.dir;
        if (pMaxZ.x >= aabb.min.x && pMaxZ.x <= aabb.max.x &&
            pMaxZ.y >= aabb.min.y && pMaxZ.y <= aabb.max.y) {
            return true;
        }
    }

    return false;
}

bool Object::intersect(Ray ray, IntersectionData& idata, bool backface, bool any, real_t max_t) const
{
    size_t num_triangles = triangles.size() / 3;
    IntersectionData temp_idata{};

    idata.t = max_t;

    if (hasAABB && !AABBIntersection(ray, aabb)) {
        return false;
    }

    for (size_t i = 0; i < num_triangles; i++) {
        const bool hit = triangleIntersection(
            ray,
            vertices,
            triangles[i*3+0],
            triangles[i*3+1],
            triangles[i*3+2],
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

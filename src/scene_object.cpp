#include "scene_object.h"
#include "utils.h"
#include "renderer_lib.h"

bool triangleIntersection(Ray ray, const std::vector<Vector>& vertices, int v1, int v2, int v3, IntersectionData& idata, bool backface = false)
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
    if (t < 0) {
        return false;
    }

    idata.normal = normalized(cross(e1, e2));
    // The ray intersects the triangle
    return true;
}

bool Object::intersect(Ray ray, IntersectionData& idata, bool backface, bool any) const
{
    size_t num_triangles = triangles.size() / 3;
    IntersectionData temp_idata{};

    idata.t = 1e30f;
    for (size_t i = 0; i < num_triangles; i++) {
        const bool hit = triangleIntersection(
            ray,
            vertices,
            triangles[i*3+0],
            triangles[i*3+1],
            triangles[i*3+2],
            temp_idata,
            backface
        );
        if (hit && temp_idata.t < idata.t) {
            idata = temp_idata;
            if (any) return true;
        }
    }
    return idata.t < 1e30f;
}
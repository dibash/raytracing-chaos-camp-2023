#include "scene_object.h"
#include "utils.h"
#include "renderer_lib.h"

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

    idata.normal = normalized(cross(e1, e2));
    // The ray intersects the triangle
    return true;
}

bool Object::intersect(Ray ray, IntersectionData& idata, bool backface, bool any, real_t max_t) const
{
    size_t num_triangles = triangles.size() / 3;
    IntersectionData temp_idata{};

    idata.t = max_t;
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

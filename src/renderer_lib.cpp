#include "renderer_lib.h"
#include "vector.h"
#include "utils.h"
#include "scene_object.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <execution>


const size_t WIDTH = 1920;
const size_t HEIGHT = 1080;


bool triangleIntersection(Vector rayOrigin, Vector rayDir, const NaiveTriangle& tri, IntersectionData& idata)
{
    real_t& t = idata.t;
    real_t& u = idata.u;
    real_t& v = idata.v;

    // Calculate the triangle edges
    const Vector e1 = tri.v2 - tri.v1;
    const Vector e2 = tri.v3 - tri.v1;

    const Vector h = cross(rayDir, e2);
    real_t d = dot(e1, h);

    // Ray is parallel to the triangle
    if (std::abs(d) < EPSILON)
        return false;

    // If d < 0 => we are hitting the back side of the triangle plane
    // return here if we want backface culling

    // Precompute to avoid division
    real_t f = 1 / d;

    const Vector s = rayOrigin - tri.v1;
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
    v = f * dot(rayDir, q);

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

Color shade(Vector rayDirection, const IntersectionData &idata)
{
    const real_t theta = dot(-rayDirection, idata.normal);
    const real_t val = theta / 3 * 2 + 1.0f / 3;
    return { val * 0.5f, val * 0.3f, val * 0.9f };
}

Ray generateCameraRay(Vector origin, int x, int y, real_t scale)
{
    const real_t aspect = real_t(WIDTH) / real_t(HEIGHT);
    real_t X = (2.0f * (x + 0.5f) / WIDTH - 1.0f) * aspect * scale;
    real_t Y = (1.0f - (2.0f * (y + 0.5f) / HEIGHT)) * scale;

    return { origin, normalized({ X, Y, -1 }) };
}

void renderImage(Color* pixels, const std::vector<Object>& scene)
{
    const Vector cameraPosition{ 0, 0, 0 };
    const Vector cameraForward = normalized({ 0, 0, -1 });

    const real_t fov = 90; // field of view in degrees
    const real_t scale = std::tanf((fov * 0.5) * PI / 180);

    std::vector<int> height(HEIGHT);
    std::iota(height.begin(), height.end(), 0);

    std::for_each(
        std::execution::par,
        height.begin(),
        height.end(),
        [&](int y) {
            IntersectionData idata;
            IntersectionData closest_idata;
            for (int x = 0; x < WIDTH; x++) {
                Ray ray = generateCameraRay(cameraPosition, x, y, scale);
                closest_idata.t = 1e30f;
                for (size_t i = 0; i < scene.size(); i++) {
                    bool intersection = scene[i].intersect(ray, idata);
                    if (intersection && idata.t < closest_idata.t) {
                        closest_idata = idata;
                    }
                }
                if (closest_idata.t < 1e30f) {
                    pixels[y * WIDTH + x] = shade(ray.dir, closest_idata);
                    //pixels[y * WIDTH + x] = { idata.u, idata.v, 0.1f };
                    //pixels[y * WIDTH + x] = { idata.normal.x, idata.normal.y, idata.normal.z };
                }
                else {
                    pixels[y * WIDTH + x] = {
                        std::abs(ray.dir.x),
                        std::abs(ray.dir.y),
                        std::abs(ray.dir.z / 2.0f),
                    };
                }
            }
        }
    );
}

std::vector<Object> generate_scene()
{
    std::vector<Object> objects;

    Object simple_triangle({
        Vector(-1.75f, -1.75f, -3),
        Vector(1.75f, -1.75f, -3),
        Vector(0, 1.75f, -3)},
        { 0, 1, 2 }
    );

    Object another_triangle({
        Vector(2, 2, -3),
        Vector(1, 2, -3),
        Vector(1.5f, 0, -3)},
        { 0, 1, 2 }
    );

    // Half cube
    Object half_cube({
        // front side
        Vector(-0.139214f, -0.3f, -1.57511f),
        Vector(0.024891f, -0.3f, -1.46079f),
        Vector(0.024891f, -0.1f, -1.46079f),
        Vector(-0.139214f, -0.1f, -1.57511f),
        // back side
        Vector(-0.024890f, -0.3f, -1.83921f),
        Vector(0.139214f, -0.3f, -1.72489f),
        Vector(0.139214f, -0.1f, -1.72489f),
        Vector(-0.024890f, -0.1f, -1.83921f)},
        {
        0, 1, 2,
        0, 2, 3,
        1, 5, 6,
        1, 6, 2,
        3, 2, 6,
        3, 6, 7
    });

    Object prism ({
        Vector(-1.4299746f, -0.75f, -1.82386f),
        Vector(-1.1699746f, -0.75f, -1.56386f),
        Vector(-1.4299746f, -0.75f, -1.30386f),
        Vector(-1.6899746f, -0.75f, -1.56386f),
        Vector(-1.4299746f, -0.25f, -1.56386f)
    },{
        0, 4, 1,
        1, 4, 2,
        2, 3, 0,
        2, 4, 3,
        2, 0, 1,
        3, 4, 0,
    });

    objects.push_back(simple_triangle);
    objects.push_back(another_triangle);
    objects.push_back(half_cube);
    objects.push_back(prism);

    return objects;
}

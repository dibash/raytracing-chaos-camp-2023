#include "vector.h"
#include "utils.h"

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

Vector generateCameraRay(int x, int y, real_t scale)
{
    const real_t aspect = real_t(WIDTH) / real_t(HEIGHT);
    real_t X = (2.0f * (x + 0.5f) / WIDTH - 1.0f) * aspect * scale;
    real_t Y = (1.0f - (2.0f * (y + 0.5f) / HEIGHT)) * scale;

    return normalized({ X, Y, -1 });
}

void renderImage(Color* pixels, const std::vector<NaiveTriangle>& triangles)
{
    const std::vector<NaiveTriangle>& tri = triangles;
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
                Vector rayDirection = generateCameraRay(x, y, scale);
                closest_idata.t = 1e30f;
                for (size_t i = 0; i < tri.size(); i++) {
                    bool intersection = triangleIntersection(cameraPosition, rayDirection, tri[i], idata);
                    if (intersection && idata.t < closest_idata.t) {
                        closest_idata = idata;
                    }
                }
                if (closest_idata.t < 1e30f) {
                    pixels[y * WIDTH + x] = shade(rayDirection, closest_idata);
                    //pixels[y * WIDTH + x] = { idata.u, idata.v, 0.1f };
                    //pixels[y * WIDTH + x] = { idata.normal.x, idata.normal.y, idata.normal.z };
                }
                else {
                    pixels[y * WIDTH + x] = {
                        std::abs(rayDirection.x),
                        std::abs(rayDirection.y),
                        std::abs(rayDirection.z / 2.0f),
                    };
                }
            }
        }
    );
}

std::vector<NaiveTriangle> generate_triangles()
{
    std::vector<NaiveTriangle> allTriangles;

    // Task 1
    allTriangles.push_back({
        Vector(-1.75f, -1.75f, -3),
        Vector(1.75f, -1.75f, -3),
        Vector(0, 1.75f, -3)
        });

    // Task 2
    allTriangles.push_back({
        Vector(2, 2, -3),
        Vector(1, 2, -3),
        Vector(1.5f, 0, -3)
        });

    // Half cube
    const Vector cube_vertices[] = {
        // front side
        Vector{-0.139214f, -0.3f, -1.57511f},
        Vector{ 0.024891f, -0.3f, -1.46079f},
        Vector{ 0.024891f, -0.1f, -1.46079f},
        Vector{-0.139214f, -0.1f, -1.57511f},
        // back side
        Vector{-0.024890f, -0.3f, -1.83921f},
        Vector{ 0.139214f, -0.3f, -1.72489f},
        Vector{ 0.139214f, -0.1f, -1.72489f},
        Vector{-0.024890f, -0.1f, -1.83921f},
    };
    std::vector<NaiveTriangle> cube = {
        // front face
        {cube_vertices[0], cube_vertices[1], cube_vertices[2]},
        {cube_vertices[0], cube_vertices[2], cube_vertices[3]},
        // side face
        {cube_vertices[1], cube_vertices[5], cube_vertices[6]},
        {cube_vertices[1], cube_vertices[6], cube_vertices[2]},
        // top face
        {cube_vertices[3], cube_vertices[2], cube_vertices[6]},
        {cube_vertices[3], cube_vertices[6], cube_vertices[7]},
    };
    allTriangles.insert(allTriangles.end(), cube.cbegin(), cube.cend());


    const Vector prism_vertices[] = {
        Vector(-1.4299746f, -0.75f, -1.82386f),
        Vector(-1.1699746f, -0.75f, -1.56386f),
        Vector(-1.4299746f, -0.75f, -1.30386f),
        Vector(-1.6899746f, -0.75f, -1.56386f),
        Vector(-1.4299746f, -0.25f, -1.56386f)
    };
    std::vector<NaiveTriangle> prism = {
        {prism_vertices[0], prism_vertices[4], prism_vertices[1]},
        {prism_vertices[1], prism_vertices[4], prism_vertices[2]},
        {prism_vertices[2], prism_vertices[3], prism_vertices[0]},
        {prism_vertices[2], prism_vertices[4], prism_vertices[3]},
        {prism_vertices[2], prism_vertices[0], prism_vertices[1]},
        {prism_vertices[3], prism_vertices[4], prism_vertices[0]},
    };
    allTriangles.insert(allTriangles.end(), prism.cbegin(), prism.cend());

    return allTriangles;
}

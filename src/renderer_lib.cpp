#include "vector.h"
#include "utils.h"

#include <vector>
#include <cmath>
#include <memory>
#include <algorithm>
#include <execution>

using std::unique_ptr;
using std::array;

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

    // The ray intersects the triangle
    return true;
}

std::vector<NaiveTriangle> generate_triangles();

void renderImage(Color* pixels, real_t t)
{
    const std::vector<NaiveTriangle> tri = generate_triangles();

    const Vector cameraPosition{ 0, 0, t };
    const Vector cameraForward = normalized({ 0, 0, -1 });

    const real_t fov = 90; // field of view in degrees
    const real_t scale = std::tanf((fov * 0.5) * PI / 180);

    /*
    std::for_each(
        std::execution::par,
        foo.begin(),
        foo.end(),
        [](auto&& item)
        {
            //do stuff with item
        });*/



    std::vector<int> height(HEIGHT);
    std::iota(height.begin(), height.end(), 0);

    std::for_each(
        std::execution::par,
        height.begin(),
        height.end(),
        [&](auto&& y) {

            IntersectionData idata;

            //for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                // find center of pixel
                real_t pixelCenterX = x + 0.5f;
                real_t pixelCenterY = y + 0.5f;

                // convert raster coordinates to NDC space
                real_t ndcX = pixelCenterX / WIDTH;
                real_t ndcY = pixelCenterY / HEIGHT;

                // convert NDC coordinates to Screen space
                real_t screenX = (2.0f * ndcX) - 1.0f;
                real_t screenY = 1.0f - (2.0f * ndcY);

                // consider aspect ratio
                screenX *= real_t(WIDTH) / real_t(HEIGHT);

                real_t scaledX = screenX * scale;
                real_t scaledY = screenY * scale;


                // create ray direction vector
                Vector rayDirection{ scaledX, scaledY, cameraForward.z };

                // normalize ray direction
                rayDirection = normalized(rayDirection);

                //pixels[y][x] = { 24, 24, 24 };

                pixels[y*WIDTH + x] = {
                    uint8_t(std::abs(rayDirection.x) * 255),
                    uint8_t(std::abs(rayDirection.y) * 255),
                    uint8_t(std::abs(rayDirection.z / 2.0f) * 255),
                };

                real_t closest_t = 1e30f;
                for (size_t i = 0; i < tri.size(); i++) {
                    bool intersection = triangleIntersection(cameraPosition, rayDirection, tri[i], idata);
                    if (intersection && idata.t < closest_t) {
                        //pixels[y][x] = { uint8_t(idata.u * 240), uint8_t(idata.v * 240), 30 };
                        const Vector e1 = tri[i].v2 - tri[i].v1;
                        const Vector e2 = tri[i].v3 - tri[i].v1;
                        const Vector N = normalized(cross(e1, e2));
                        //pixels[y][x] = { uint8_t(N.x * 240), uint8_t(N.y * 240), uint8_t(N.z * 240) };
                        const real_t theta = dot(-rayDirection, N);
                        const real_t val = theta / 3 * 2 + 1.0f / 3;
                        pixels[y*WIDTH + x] = { uint8_t(val * 120), uint8_t(val * 80), uint8_t(val * 240) };
                        closest_t = idata.t;
                    }
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

    // Task 4
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

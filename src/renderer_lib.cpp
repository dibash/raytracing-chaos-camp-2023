﻿#include "renderer_lib.h"
#include "vector.h"
#include "utils.h"
#include "scene_object.h"
#include "camera.h"
#include "scene.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <execution>


Color shade(const Scene& scene, const Ray& ray, const IntersectionData &idata)
{
    if (idata.u == -1 && idata.v == -1) {
        // "Temporary" workaround for rendering lights
        return { 1, 1, .9f, 1 };
    }
    const Color albedo = { 0.5f, 0.2f, 0.9f };
    const real_t shadowBias = 0.0001f;
    const Vector ip = ray.origin + ray.dir * idata.t + idata.normal * shadowBias;
    IntersectionData idata2;
    Color finalColor = { 0,0,0,1 };

    if (scene.lights.empty()) {
        const real_t theta = dot(-ray.dir, idata.normal);
        const real_t val = theta / 3 * 2 + 1.0f / 3;
        return { val * 0.5f, val * 0.3f, val * 0.9f };
    }

    for (const Light& l : scene.lights) {
        const Vector lightDir = l.position - ip;
        const Ray shadowRay = { ip, normalized(lightDir) };
        bool shadow = scene.intersect(shadowRay, idata2, true, true);
        if (!shadow) {
            const real_t cosLaw = std::max(.0f, dot(shadowRay.dir, idata.normal));
            const real_t rSqr = lightDir.lengthSqr();
            const real_t area = 4 * PI * rSqr;
            const Color contribution = (l.intensity / area * cosLaw) * albedo;
            finalColor += contribution;
        }
    }
    return finalColor;
}

void renderImage(Color* pixels, const Scene& scene)
{
    const size_t WIDTH = scene.settings.width;
    const size_t HEIGHT = scene.settings.height;
    std::vector<int> height(HEIGHT);
    std::iota(height.begin(), height.end(), 0);

    std::for_each(
        std::execution::par,
        height.begin(),
        height.end(),
        [&](int y) {
            IntersectionData idata;
            for (int x = 0; x < WIDTH; x++) {
                Ray ray = scene.camera.generateCameraRay(WIDTH, HEIGHT, x, y);
                const bool intersection = scene.intersect(ray, idata);
                if (intersection) {
                    pixels[y * WIDTH + x] = shade(scene, ray, idata);
                    //pixels[y * WIDTH + x] = { idata.u, idata.v, 0.1f };
                    //pixels[y * WIDTH + x] = { idata.normal.x, idata.normal.y, idata.normal.z };
                }
                else {
                    pixels[y * WIDTH + x] = scene.settings.background;
                    /*
                    Vector absDir{ std::abs(ray.dir.x), std::abs(ray.dir.y), std::abs(ray.dir.z) };
                    real_t axis = std::max({ absDir.x, absDir.y, absDir.z });
                    pixels[y * WIDTH + x] = {
                        std::abs(absDir.x - axis) < EPSILON ? absDir.x : 0,
                        std::abs(absDir.y - axis) < EPSILON ? absDir.y : 0,
                        std::abs(absDir.z - axis) < EPSILON ? absDir.z : 0,
                    };
                    pixels[y * WIDTH + x] = {
                        std::abs(ray.dir.x),
                        std::abs(ray.dir.y),
                        std::abs(ray.dir.z / 2.0f),
                    };
                    */
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

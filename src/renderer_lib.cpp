#include "renderer_lib.h"
#include "vector.h"
#include "utils.h"
#include "scene_object.h"
#include "camera.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <execution>


const size_t WIDTH = 1920;
const size_t HEIGHT = 1080;


Color shade(Vector rayDirection, const IntersectionData &idata)
{
    const real_t theta = dot(-rayDirection, idata.normal);
    const real_t val = theta / 3 * 2 + 1.0f / 3;
    return { val * 0.5f, val * 0.3f, val * 0.9f };
}

void renderImage(Color* pixels, const std::vector<Object>& scene, Camera cam)
{
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
                Ray ray = cam.generateCameraRay(WIDTH, HEIGHT, x, y);
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

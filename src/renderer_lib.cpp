#include "renderer_lib.h"
#include "vector.h"
#include "utils.h"
#include "scene_object.h"
#include "camera.h"
#include "scene.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <execution>

std::vector<Bucket> generate_buckets(const Scene& scene)
{
    const size_t full_buckets_h = scene.settings.width / scene.settings.bucketSize;
    const size_t partial_buckets_h_width = scene.settings.width % scene.settings.bucketSize;
    const size_t full_buckets_v = scene.settings.height / scene.settings.bucketSize;
    const size_t partial_buckets_v_height = scene.settings.height % scene.settings.bucketSize;

    std::vector<Bucket> buckets;
    buckets.reserve((full_buckets_v + (partial_buckets_v_height != 0)) * (full_buckets_h + (partial_buckets_h_width != 0)));

    for (size_t row = 0; row < full_buckets_v; ++row) {
        for (size_t col = 0; col < full_buckets_h; ++col) {
            // full buckets
            buckets.push_back({
                col * scene.settings.bucketSize,
                row * scene.settings.bucketSize,
                scene.settings.bucketSize,
                scene.settings.bucketSize
            });
        }
        // Add bucket from the last column
        if (partial_buckets_h_width) {
            buckets.push_back({
                full_buckets_h * scene.settings.bucketSize,
                row * scene.settings.bucketSize,
                partial_buckets_h_width,
                scene.settings.bucketSize
            });
        }
    }

    // Generate partial buckets in the last row
    if (partial_buckets_v_height) {
        const size_t last_row_y = full_buckets_v * scene.settings.bucketSize;
        for (size_t col = 0; col < full_buckets_h; ++col) {
            buckets.push_back({
                col * scene.settings.bucketSize,
                last_row_y,
                scene.settings.bucketSize,
                partial_buckets_v_height
            });
        }
        // Add bucket in the corner
        if (partial_buckets_h_width) {
            buckets.push_back({
                full_buckets_h * scene.settings.bucketSize,
                last_row_y,
                partial_buckets_h_width,
                partial_buckets_v_height
                });
        }
    }

    return buckets;
}

void renderBucket(Color* pixels, const Bucket& bucket, const Scene& scene)
{
    const size_t WIDTH = scene.settings.width;
    const size_t HEIGHT = scene.settings.height;
    IntersectionData idata;
    for (size_t y = bucket.y; y < bucket.y + bucket.h; y++) {
        for (size_t x = bucket.x; x < bucket.x + bucket.w; x++) {
            Ray ray = scene.camera.generateCameraRay(WIDTH, HEIGHT, x, y);
            const bool intersection = scene.intersect(ray, idata);
            if (intersection) {
                pixels[y * WIDTH + x] = scene.shade(ray, idata);
            }
            else {
                pixels[y * WIDTH + x] = scene.settings.background;
            }
        }
    }
}


void renderImage(Color* pixels, const Scene& scene)
{
    const size_t WIDTH = scene.settings.width;
    const size_t HEIGHT = scene.settings.height;

#if 1 // Buckets

    std::vector<Bucket> buckets = generate_buckets(scene);

    std::for_each(
        std::execution::par,
        buckets.begin(),
        buckets.end(),
        [&](const Bucket& bucket) {
            renderBucket(pixels, bucket, scene);
        }
    );
#else // Scanline

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
                    pixels[y * WIDTH + x] = scene.shade(ray, idata);
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
#endif
}

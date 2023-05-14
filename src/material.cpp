#include "material.h"
#include "scene.h"
#include "scene_object.h"

const int MAX_DEPTH = 4;

Color DiffuseMaterial::shade(const Scene& scene, const Ray& ray, const IntersectionData& idata, int depth) const
{
    const Vector ip = ray.origin + ray.dir * idata.t + idata.normal * shadowBias;
    IntersectionData idata2;
    Color finalColor = { 0,0,0,1 };

    for (const Light& l : scene.lights) {
        const Vector lightDir = l.position - ip;
        const Ray shadowRay = { ip, normalized(lightDir) };
        bool shadow = scene.intersect(shadowRay, idata2, true, true, lightDir.length());
        if (!shadow) {
            const real_t cosLaw = std::max(.0f, dot(shadowRay.dir, idata.normal));
            const real_t rSqr = lightDir.lengthSqr();
            const real_t area = 4 * PI * rSqr;
            const Color contribution = (l.intensity / area * cosLaw) * albedo;
            finalColor += contribution;
        }
    }

    if (scene.lights.empty()) {
        const real_t theta = dot(-ray.dir, idata.normal);
        const real_t val = theta / 3 * 2 + 1.0f / 3;
        return val * albedo;
    }

    return finalColor;
}

Color ReflectiveMaterial::shade(const Scene& scene, const Ray& ray, const IntersectionData& idata, int depth) const {
    const Vector ip = ray.origin + ray.dir * idata.t + idata.normal * shadowBias;
    const Vector reflectedDir = reflect(ray.dir, idata.normal);
    const Ray reflectedRay = { ip, reflectedDir };
    IntersectionData idata2;

    // Compute the reflected color recursively
    Color reflectedColor = scene.settings.background;
    if (depth < MAX_DEPTH) {
        bool hit = scene.intersect(reflectedRay, idata2);
        if (hit) {
            reflectedColor = idata2.object->getMaterial()->shade(scene, reflectedRay, idata2);
        }
    }

    return reflectedColor * albedo;
}

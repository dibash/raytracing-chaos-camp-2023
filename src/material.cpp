#include "material.h"
#include "scene.h"
#include "scene_object.h"

Color DiffuseMaterial::shade(const Scene& scene, const Ray& ray, const IntersectionData& idata) const
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

Color ReflectiveMaterial::shade(const Scene& scene, const Ray& ray, const IntersectionData& idata) const
{
    return albedo;
}

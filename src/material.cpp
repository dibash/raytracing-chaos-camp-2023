#include "material.h"
#include "scene.h"
#include "scene_object.h"

#include <random>

const int MAX_DEPTH = 8;

Color ConstantMaterial::shade(const Scene& scene, const Ray& ray, const IntersectionData& idata, int depth) const
{
    IntersectionData idataSmooth = smooth_shading ?
        idata.object->smoothIntersection(idata) :
        idata;

    const real_t theta = dot(-ray.dir, idataSmooth.normal);
    const real_t val = theta / 3 * 2 + 1.0f / 3;
    return val * albedo;
}

const int GI_RAYS = 128;
const int GI_DEPTH = 1;
thread_local std::random_device rd;
thread_local std::mt19937 gen(rd());
thread_local std::uniform_real_distribution<float> dis(0.0f, 1.0f);

Vector randomUnitVector()
{
    real_t theta = 2.0f * PI * dis(gen);
    real_t z = 2.0f * dis(gen) - 1.0f;
    real_t r = std::sqrt(1.0f - z * z);

    return { r * std::cos(theta), r * std::sin(theta), z };
}

Ray generateGIRay(const Ray& incomingRay, const IntersectionData& idata)
{
    Vector reflectedDirection = reflect(incomingRay.dir, idata.normal);
    Vector newDirection = reflectedDirection + randomUnitVector();
    return { idata.ip, newDirection, incomingRay.giDepth + 1 };
}

Color DiffuseMaterial::shade(const Scene& scene, const Ray& ray, const IntersectionData& idata, int depth) const
{
    IntersectionData idataSmooth = smooth_shading ?
        idata.object->smoothIntersection(idata) :
        idata;

    Vector ip = idataSmooth.ip + idataSmooth.normal * shadowBias;

    IntersectionData idata2;
    Color finalColor = { 0,0,0,1 };

    for (const Light& l : scene.lights) {
        const Vector lightDir = l.position - ip;
        const Ray shadowRay = { ip, normalized(lightDir) };
        bool shadow = scene.intersect(shadowRay, idata2, true, true, lightDir.length());
        if (!shadow) {
            const real_t cosLaw = std::max(.0f, dot(shadowRay.dir, idataSmooth.normal));
            const real_t rSqr = lightDir.lengthSqr();
            const real_t area = 4 * PI * rSqr;
            const Color contribution = (l.intensity / area * cosLaw) * albedo;
            finalColor += contribution;
        }
    }

    Color giColor = { 0,0,0,1 };
    int giTraced = 0;
    if (ray.giDepth < GI_DEPTH) {
        for (int i = 0; i < GI_RAYS; ++i) {
            IntersectionData idataGI;
            const Ray giRay = generateGIRay(ray, idataSmooth);
            bool intersect = scene.intersect(giRay, idataGI);
            if (intersect && idataGI.object && idataGI.object->getMaterial()) {
                giColor += idataGI.object->getMaterial()->shade(scene, giRay, idataGI, depth + 1);
            }
            giTraced++;
        }
    }

    if (scene.lights.empty()) {
        const real_t theta = dot(-ray.dir, idataSmooth.normal);
        const real_t val = theta / 3 * 2 + 1.0f / 3;
        return val * albedo;
    }

    return (1.0f / (giTraced + 1)) * (finalColor + giColor);
}

Color ReflectiveMaterial::shade(const Scene& scene, const Ray& ray, const IntersectionData& idata, int depth) const
{
    IntersectionData idataSmooth = smooth_shading ?
        idata.object->smoothIntersection(idata) :
        idata;

    Vector ip = idataSmooth.ip + idataSmooth.normal * shadowBias;

    const Vector reflectedDir = reflect(ray.dir, idataSmooth.normal);
    const Ray reflectedRay = { ip, reflectedDir, ray.giDepth };
    IntersectionData idata2;

    // Compute the reflected color recursively
    Color reflectedColor = scene.settings.background;
    if (depth < MAX_DEPTH) {
        bool hit = scene.intersect(reflectedRay, idata2);
        if (hit && idata2.object) {
            reflectedColor = idata2.object->getMaterial()->shade(scene, reflectedRay, idata2, depth + 1);
        }
    }

    return reflectedColor * albedo;
}

Color RefractiveMaterial::shade(const Scene& scene, const Ray& ray, const IntersectionData& idata, int depth) const
{
    IntersectionData idataSmooth = smooth_shading ?
        idata.object->smoothIntersection(idata) :
        idata;

    const bool inside = dot(ray.dir, idata.normal) > 0;
    const Vector ipIn = idata.ip - idataSmooth.normal * shadowBias;
    const Vector ipOut = idataSmooth.ip + idataSmooth.normal * shadowBias;
    const Vector normal = inside ? -idataSmooth.normal : idataSmooth.normal;
    const real_t ior = inside ? this->IOR : 1 / this->IOR;

    Color reflectedColor;
    const Vector reflectedDir = normalized(reflect(ray.dir, normal));
    const Ray reflectedRay = { inside ? ipIn : ipOut, reflectedDir, ray.giDepth };

    // No point in tracing reflections too deep inside
    if (depth < 2) {
        IntersectionData idata2;
        bool hit = scene.intersect(reflectedRay, idata2, true);
        if (hit && idata2.object) {
            reflectedColor = idata2.object->getMaterial()->shade(scene, reflectedRay, idata2, depth + 1);
        }
        if (!hit) {
            reflectedColor = scene.settings.background;
        }
    }

    Color refractedColor;
    bool totalInternalReflection = false;
    const Vector refractedDir = normalized(refract(ray.dir, normal, ior, totalInternalReflection));
    const Vector refractedRayStart = (inside && !totalInternalReflection) ? ipOut : ipIn;
    const Ray refractedRay = { refractedRayStart, refractedDir, ray.giDepth };

    if (depth < MAX_DEPTH) {
        IntersectionData idata3;
        bool hit = scene.intersect(refractedRay, idata3, true);
        if (hit && idata3.object) {
            refractedColor = idata3.object->getMaterial()->shade(scene, refractedRay, idata3, depth + 1);
        }
        if (!hit) {
            refractedColor = scene.settings.background;
        }
    }

    const real_t fresnel = 0.5f * std::powf((1 + dot(ray.dir, normal)), 5);
    Color r = (fresnel * reflectedColor) + (1 - fresnel) * refractedColor;
    return r * albedo;
}

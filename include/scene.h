#pragma once

#include "utils.h"
#include "scene_object.h"
#include "camera.h"

#include <vector>
#include <string>

struct SceneSettings {
    size_t width = 1920;
    size_t height = 1080;
    Color background{ 0.2f, 0.2f, 0.2f };
    size_t bucketSize = 24;
};

class Scene : Intersectable {

public:
    SceneSettings settings;
    Camera camera;
    std::vector<Object> objects;
    std::vector<Material*> materials;
    std::vector<Light> lights;

public:
    Scene() {}
    Scene(const SceneSettings& settings)
        : settings(settings)
    {}
    Scene(const std::string& fileName)
    {
        load(fileName);
    }

    void addObject(const Object& object);
    void load(const std::string& fileName);

    Color shade(const Ray& ray, const IntersectionData& idata) const;

    // Intersectable
    bool intersect(Ray ray, IntersectionData& idata, bool backface = false, bool any = false, real_t max_t = 1e30f) const override;

    static void getSizeFromFile(const std::string& fileName, int& width, int& height);
};

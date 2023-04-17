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
};

class Scene : Intersectable {

public:
    SceneSettings settings;
    Camera camera;
    std::vector<Object> objects;

public:
    Scene() {}
    Scene(const SceneSettings& settings)
        : settings(settings)
    {}
    Scene(const std::string& fileName) { load(fileName); }

    void addObject(const Object& object);
    void load(const std::string& fileName);

    // Intersectable
    bool intersect(Ray ray, IntersectionData& idata) const override;
};

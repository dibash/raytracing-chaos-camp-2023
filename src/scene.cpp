#include "scene.h"


void Scene::addObject(const Object& object)
{
    objects.push_back(object);
}

bool Scene::intersect(Ray ray, IntersectionData& idata) const
{
    IntersectionData temp_idata;
    idata.t = 1e30f;
    for (size_t i = 0; i < objects.size(); i++) {
        bool intersection = objects[i].intersect(ray, temp_idata);
        if (intersection && temp_idata.t < idata.t) {
            idata = temp_idata;
        }
    }
    return idata.t < 1e30f;
}

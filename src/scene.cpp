#include "scene.h"

// Disable warnings from rapidjson
#pragma warning(push)
#pragma warning(disable: 26439 26812 26451 26495 4996 4267)
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#pragma warning(pop)

#include <fstream>
#include <iostream>

void Scene::addObject(const Object& object)
{
    objects.push_back(object);
}

bool Scene::intersect(Ray ray, IntersectionData& idata, bool backface, bool any, real_t max_t) const
{
    IntersectionData temp_idata;
    idata.t = max_t;
    /*
    if (!any) { // "Temporary" workaround for rendering lights
        for (const Light& l : lights) {
            bool intersection = l.intersect(ray, temp_idata, backface, any, max_t);
            if (intersection && temp_idata.t < idata.t) {
                idata = temp_idata;
                idata.u = -1;
                idata.v = -1;
            }
        }
    }
    */

    for (const Object& o : objects) {
        bool intersection = o.intersect(ray, temp_idata, backface, any, max_t);
        if (intersection && temp_idata.t < idata.t) {
            idata = temp_idata;
            if (any) return true;
        }
    }
    return idata.t < max_t;
}

Color Scene::shade(const Ray& ray, const IntersectionData& idata) const
{
    if (idata.u == -1 && idata.v == -1) {
        // "Temporary" workaround for rendering lights
        return { 1, 1, .9f, 1 };
    }
    Color finalColor{ 1, 0, 1, 1 };
    const Material* material = idata.object ? idata.object->getMaterial() : nullptr;
    if (material) {
        finalColor = material->shade(*this, ray, idata);
    }
    return finalColor;
}

rapidjson::Document getJsonDocument(const std::string& fileName)
{
    using namespace rapidjson;

    std::ifstream ifs(fileName);
    if (!ifs.is_open()) {
        std::cerr << "File doesn't exist or is not readable\n";
        Document doc;
        doc.Parse("");
        return doc;
    }

    IStreamWrapper isw(ifs);
    Document doc;
    doc.ParseStream(isw);
    if (doc.HasParseError()) {
        std::cerr << "Error  :" << doc.GetParseError() << '\n';
        std::cerr << "Offset :" << doc.GetErrorOffset() << '\n';
        assert(false);
    }

    assert(doc.IsObject());
    return doc;
}

Color loadColor(const rapidjson::Value::ConstArray& arr)
{
    assert(arr.Size() == 3 || arr.Size() == 4);
    Color c{
        arr[0].GetFloat(),
        arr[1].GetFloat(),
        arr[2].GetFloat(),
    };
    if (arr.Size() > 3) {
        c.a = arr[3].GetFloat();
    }
    return c;
}

Vector loadVector(const rapidjson::Value::ConstArray& arr)
{
    assert(arr.Size() == 3);
    Vector v{
        arr[0].GetFloat(),
        arr[1].GetFloat(),
        arr[2].GetFloat(),
    };
    return v;
}

Matrix loadMatrix(const rapidjson::Value::ConstArray& arr)
{
    assert(arr.Size() == 9);
    Vector r1 = { arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat() };
    Vector r2 = { arr[3].GetFloat(), arr[4].GetFloat(), arr[5].GetFloat() };
    Vector r3 = { arr[6].GetFloat(), arr[7].GetFloat(), arr[8].GetFloat() };
    return { r1, r2, r3 };
}

Light loadLight(const rapidjson::Value& lightVal)
{
    using namespace rapidjson;

    Light light;

    if (!lightVal.IsNull() && lightVal.IsObject()) {
        const Value& intensityVal = lightVal.FindMember("intensity")->value;
        if (!intensityVal.IsNull() && intensityVal.IsNumber()) {
            light.intensity = intensityVal.GetFloat();
        }
        const Value& positionVal = lightVal.FindMember("position")->value;
        if (!positionVal.IsNull() && positionVal.IsArray()) {
            light.position = loadVector(positionVal.GetArray());
        }
    }
    return light;
}

Object loadObject(const rapidjson::Value& objectVal)
{
    using namespace rapidjson;

    std::vector<Vector> verts;
    std::vector<int> triangles;

    const Value& verticesVal = objectVal.FindMember("vertices")->value;
    const Value& trianglesVal = objectVal.FindMember("triangles")->value;
    if (!verticesVal.IsNull() && verticesVal.IsArray()) {
        verts.reserve(verticesVal.Size());
        for (SizeType i = 0; i < verticesVal.Size() / 3; ++i) {
            assert(verticesVal[i * 3 + 0].IsNumber() && verticesVal[i * 3 + 1].IsNumber() && verticesVal[i * 3 + 2].IsNumber());
            verts.push_back({
                verticesVal[i * 3 + 0].GetFloat(),
                verticesVal[i * 3 + 1].GetFloat(),
                verticesVal[i * 3 + 2].GetFloat()
                });
        }
    }
    if (!trianglesVal.IsNull() && trianglesVal.IsArray()) {
        triangles.reserve(trianglesVal.Size());
        for (const Value& v : trianglesVal.GetArray()) {
            assert(v.IsInt());
            triangles.push_back(v.GetInt());
        }
    }

    Object obj(verts, triangles);
    return obj;
}

Material* loadMaterial(const rapidjson::Value& materialVal)
{
    using namespace rapidjson;

    Material* material = nullptr;

    if (!materialVal.IsNull() && materialVal.IsObject()) {
        const Value& typeVal = materialVal.FindMember("type")->value;
        if (!typeVal.IsNull() && typeVal.IsString()) {
            std::string typeStr = typeVal.GetString();
            if (typeStr == "constant") {
                ConstantMaterial* constantMaterial = new ConstantMaterial;
                material = constantMaterial;
                const Value& albedoVal = materialVal.FindMember("albedo")->value;
                if (!albedoVal.IsNull() && albedoVal.IsArray()) {
                    constantMaterial->albedo = loadColor(albedoVal.GetArray());
                }
                const Value& smoothShadingVal = materialVal.FindMember("smooth_shading")->value;
                if (!smoothShadingVal.IsNull() && smoothShadingVal.IsBool()) {
                    constantMaterial->smooth_shading = smoothShadingVal.GetBool();
                }
            }
            else if (typeStr == "diffuse") {
                DiffuseMaterial* diffuseMaterial = new DiffuseMaterial;
                material = diffuseMaterial;
                const Value& albedoVal = materialVal.FindMember("albedo")->value;
                if (!albedoVal.IsNull() && albedoVal.IsArray()) {
                    diffuseMaterial->albedo = loadColor(albedoVal.GetArray());
                }
                const Value& smoothShadingVal = materialVal.FindMember("smooth_shading")->value;
                if (!smoothShadingVal.IsNull() && smoothShadingVal.IsBool()) {
                    diffuseMaterial->smooth_shading = smoothShadingVal.GetBool();
                }
            }
            else if (typeStr == "reflective") {
                ReflectiveMaterial* reflectiveMaterial = new ReflectiveMaterial;
                material = reflectiveMaterial;
                const Value& albedoVal = materialVal.FindMember("albedo")->value;
                if (!albedoVal.IsNull() && albedoVal.IsArray()) {
                    reflectiveMaterial->albedo = loadColor(albedoVal.GetArray());
                }
                const Value& smoothShadingVal = materialVal.FindMember("smooth_shading")->value;
                if (!smoothShadingVal.IsNull() && smoothShadingVal.IsBool()) {
                    reflectiveMaterial->smooth_shading = smoothShadingVal.GetBool();
                }
            }
            else if (typeStr == "refractive") {
                RefractiveMaterial* refractiveMaterial = new RefractiveMaterial;
                material = refractiveMaterial;
                const Value& albedoVal = materialVal.FindMember("albedo")->value;
                if (!albedoVal.IsNull() && albedoVal.IsArray()) {
                    refractiveMaterial->albedo = loadColor(albedoVal.GetArray());
                }
                const Value& smoothShadingVal = materialVal.FindMember("smooth_shading")->value;
                if (!smoothShadingVal.IsNull() && smoothShadingVal.IsBool()) {
                    refractiveMaterial->smooth_shading = smoothShadingVal.GetBool();
                }
                const Value& iorVal = materialVal.FindMember("ior")->value;
                if (!iorVal.IsNull() && iorVal.IsNumber()) {
                    refractiveMaterial->IOR = iorVal.GetFloat();
                }
            }
            else {
                std::cerr << "Unknown material type: " << typeStr << '\n';
                return nullptr;
            }
        }
    }
    return material;
}

Camera loadCamera(const rapidjson::Value& cameraVal)
{
    using namespace rapidjson;
    Camera camera;

    if (!cameraVal.IsNull() && cameraVal.IsObject()) {
        const Value& matrixVal = cameraVal.FindMember("matrix")->value;
        if (!matrixVal.IsNull() && matrixVal.IsArray()) {
            Matrix mat = loadMatrix(matrixVal.GetArray());
            camera.setOriginalMatrix(mat);
        }
        const Value& positionVal = cameraVal.FindMember("position")->value;
        if (!positionVal.IsNull() && positionVal.IsArray()) {
            camera.position = loadVector(positionVal.GetArray());
        }
    }
    return camera;
}

SceneSettings loadSettings(const rapidjson::Value& settingsVal)
{
    using namespace rapidjson;
    SceneSettings settings;

    if (!settingsVal.IsNull() && settingsVal.IsObject()) {
        const Value& bgColorVal = settingsVal.FindMember("background_color")->value;
        if (!bgColorVal.IsNull() && bgColorVal.IsArray()) {
            settings.background = loadColor(bgColorVal.GetArray());
        }
        const Value& imageSettingsVal = settingsVal.FindMember("image_settings")->value;
        if (!imageSettingsVal.IsNull() && imageSettingsVal.IsObject()) {
            const Value& imageWidthVal = imageSettingsVal.FindMember("width")->value;
            const Value& imageHeightVal = imageSettingsVal.FindMember("height")->value;
            if (!imageWidthVal.IsNull() && imageWidthVal.IsInt() && !imageHeightVal.IsNull() && imageHeightVal.IsInt()) {
                settings.width = imageWidthVal.GetInt();
                settings.height = imageHeightVal.GetInt();
            }
        }
    }
    return settings;
}

void Scene::load(const std::string& fileName)
{
    using namespace rapidjson;
    Document doc = getJsonDocument(fileName);

    if (doc.HasParseError()) {
        if (doc.GetParseError() == kParseErrorDocumentEmpty) {
            std::cerr << "Error: Document is empty\n";
            return;
        }
    }

    const Value& settingsVal = doc.FindMember("settings")->value;
    settings = loadSettings(settingsVal);

    const Value& cameraVal = doc.FindMember("camera")->value;
    camera = loadCamera(cameraVal);

    const Value& lightsVal = doc.FindMember("lights")->value;
    if (!lightsVal.IsNull() && lightsVal.IsArray()) {
        for (const Value& v : lightsVal.GetArray()) {
            lights.push_back(loadLight(v));
        }
    }

    const Value& materialsVal = doc.FindMember("materials")->value;
    if (!materialsVal.IsNull() && materialsVal.IsArray()) {
        for (const Value& v : materialsVal.GetArray()) {
            Material* mat = loadMaterial(v);
            if (mat) materials.push_back(mat);
        }
    }

    const Value& objectsVal = doc.FindMember("objects")->value;
    if (!objectsVal.IsNull() && objectsVal.IsArray()) {
        for (const Value& v : objectsVal.GetArray()) {
            Object o = loadObject(v);
            // TODO: fix this ugly
            int materialIndex = -1;
            const Value& materialIndexVal = v.FindMember("material_index")->value;
            if (!materialIndexVal.IsNull() && materialIndexVal.IsUint()) {
                materialIndex = materialIndexVal.GetUint();
            }
            if (materialIndex >= 0 && materialIndex < materials.size())
                o.setMaterial(materials[materialIndex]);
            addObject(o);
        }
    }
}

void Scene::getSizeFromFile(const std::string& fileName, int& width, int& height)
{
    using namespace rapidjson;
    Document doc = getJsonDocument(fileName);
    if (doc.HasParseError()) {
        if (doc.GetParseError() == kParseErrorDocumentEmpty) {
            std::cerr << "Error: Document is empty\n";
            return;
        }
    }
    const Value& settingsVal = doc.FindMember("settings")->value;
    SceneSettings settings = loadSettings(settingsVal);
    width = (int)settings.width;
    height = (int)settings.height;
}

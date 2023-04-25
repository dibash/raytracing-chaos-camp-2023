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
            assert(verticesVal[i * 3 + 0].IsFloat() && verticesVal[i * 3 + 1].IsFloat() && verticesVal[i * 3 + 2].IsFloat());
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

    const Value& objectsVal = doc.FindMember("objects")->value;
    if (!objectsVal.IsNull() && objectsVal.IsArray()) {
        for (const Value& v : objectsVal.GetArray()) {
            addObject(loadObject(v));
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

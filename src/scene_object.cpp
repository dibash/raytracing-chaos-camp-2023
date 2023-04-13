#include "scene_object.h"
#include "utils.h"
#include "renderer_lib.h"

bool Object::intersect(Ray ray, IntersectionData& idata) const
{
	size_t num_triangles = triangles.size() / 3;
	IntersectionData temp_idata{};

	idata.t = 1e30f;
	for (size_t i = 0; i < num_triangles; i++) {
		NaiveTriangle tri{
			vertices[triangles[i*3+0]],
			vertices[triangles[i*3+1]],
			vertices[triangles[i*3+2]]
		};
		const bool hit = triangleIntersection(ray.origin, ray.dir, tri, temp_idata);
		if (hit && temp_idata.t < idata.t) {
			idata = temp_idata;
		}
	}
	return idata.t < 1e30f;
}
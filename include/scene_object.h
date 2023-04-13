#pragma once

#include <vector>
#include "utils.h"

class Object : Intersectable {

private:
	// Object data
	std::vector<Vector> vertices;
	std::vector<int> triangles;

public:
	// Constructors
	Object(std::vector<Vector>&& vertices, std::vector<int>&& triangles)
		: vertices(vertices)
		, triangles(triangles)
	{}
	Object(const std::vector<Vector>& vertices, const std::vector<int>& triangles)
		: vertices(vertices)
		, triangles(triangles)
	{}

	// Intersectable
	bool intersect(Ray ray, IntersectionData& idata) const override;
};
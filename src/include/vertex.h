#pragma once

#include <array>

#include <glm/glm.hpp>

// general vertex
struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
};

// vertex : position with w
struct VertexP
{
	glm::vec4 position;
	glm::vec3 color;
};

template<typename VertexT>
struct TriangleT
{
	std::array<VertexT, 3> vertices;
};

// general triangle
using Triangle = TriangleT<Vertex>;
// triangle in render pipline
using TriangleP = TriangleT<VertexP>;

template<typename VertexT>
VertexT interpolate(const VertexT& A, const VertexT& B, float t) {
	auto pos = A.position + (B.position - A.position) * t;
	auto color = A.color + (B.color - A.color) * t;
	return { pos, color };
}

template<typename VertexT>
void assignData(VertexT* output, const VertexT* input, const size_t n) {
	for (size_t i = 0; i < n; i++) {
		output[i] = input[i];
	}
}
#pragma once

#include <array>

#include <glm/glm.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
};

struct ClipVertex
{
	glm::vec4 clipPos;
	glm::vec3 color;

	static ClipVertex Interpolate(const ClipVertex& A, const ClipVertex& B, float t)
	{
		glm::vec4 clipPos = A.clipPos + (B.clipPos - A.clipPos) * t;
		glm::vec3 color = A.color + (B.color - A.color) * t;
		return { clipPos, color };
	}

	static void assignData(ClipVertex* output, const ClipVertex* input, const size_t n)
	{
		for (size_t i = 0; i < n; i++)
		{
			output[i] = input[i];
		}
	}
};

struct Triangle
{
	std::array<Vertex, 3> vertices;
};

struct ClipTriangle
{
	std::array<ClipVertex, 3> vertices;
};
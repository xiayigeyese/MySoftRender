#pragma once
#include <iostream>
#include <array>

template<typename Vertex>
class Clipper
{
public:
	static constexpr int MAX_OUTPUT_CLIPPED_POINT = 12;
	static constexpr int TRIANGLE_VERTICES_COUNT = 3;

	enum AXIS_FLAG { X, Y, Z};
	enum W_SIGN_FLAG { NEGATIVE, POSITIVE};
	
	template<int AXIS, bool W_SIGN>
	static bool isInBoundary(const Vertex& vertex)
	{
		if constexpr (W_SIGN)
		{
			return vertex.clipPos[AXIS] <= vertex.clipPos.w;
		}
		return vertex.clipPos[AXIS] >= -1 * vertex.clipPos.w;
	}

	template<int AXIS, bool W_SIGN>
	static float computeParamT(const Vertex& A, const Vertex& B)
	{
		float t;
		if constexpr (W_SIGN)
		{
			t = (A.clipPos.w - A.clipPos[AXIS]) / (B.clipPos[AXIS] - B.clipPos.w + A.clipPos.w - A.clipPos[AXIS]);
		}
		else
		{
			t = -1.0f * (A.clipPos.w + A.clipPos[AXIS]) / (B.clipPos[AXIS] - A.clipPos[AXIS] + B.clipPos.w - A.clipPos.w);
		}
		return t;
	}

	template<int AXIS, bool W_SIGN>
	size_t ClipPolygonWithPlane(Vertex* inVertices, size_t inCount, Vertex* outVertices)
	{
		size_t outCount = 0;
		Vertex A = inVertices[0];
		bool isAIn = isInBoundary<AXIS, W_SIGN>(A);
		for (size_t i = 1; i <= inCount; i++)
		{
			Vertex B = inVertices[i % inCount];
			const bool isBIn = isInBoundary<AXIS, W_SIGN>(B);
			if (isAIn)
			{
				outVertices[outCount++] = A;
			}
			if (isAIn ^ isBIn)
			{
				float t = computeParamT<AXIS, W_SIGN>(A, B);
				outVertices[outCount++] = Vertex::Interpolate(A, B, t);
			}
			A = B;
			isAIn = isBIn;
		}
		return outCount;
	}

	bool isInner(const Vertex& A)
	{
		return abs(A.clipPos.x) <= abs(A.clipPos.w) &&
			   abs(A.clipPos.y) <= abs(A.clipPos.w) &&
			   isInBoundary<AXIS_FLAG::Z, W_SIGN_FLAG::POSITIVE>(A) &&
			   isInBoundary<AXIS_FLAG::Z, W_SIGN_FLAG::NEGATIVE>(A);
	}

	size_t ClipTriangle(const Vertex* triangleVertices, Vertex* outputVertices, size_t verticesCount = 3)
	{
		if (TRIANGLE_VERTICES_COUNT != verticesCount)
		{
			std::cout << "error triangle vertices count is not 3" << std::endl;
		}
		
		if (isInner(triangleVertices[0]) && isInner(triangleVertices[1]) && isInner(triangleVertices[2]))
		{
			Vertex::assignData(outputVertices, &triangleVertices[0], 3);
			return  3;
		}
		std::array<Vertex, MAX_OUTPUT_CLIPPED_POINT> inVertices = {}, outVertices = {};
		Vertex::assignData(&inVertices[0], &triangleVertices[0], 3);

		size_t outCount = 0;
		outCount = ClipPolygonWithPlane<AXIS_FLAG::X, W_SIGN_FLAG::POSITIVE>(&inVertices[0],  3, &outVertices[0]);
		outCount = ClipPolygonWithPlane<AXIS_FLAG::X, W_SIGN_FLAG::NEGATIVE>(&outVertices[0], outCount, &inVertices[0]);
		outCount = ClipPolygonWithPlane<AXIS_FLAG::Y, W_SIGN_FLAG::POSITIVE>(&inVertices[0],  outCount, &outVertices[0]);
		outCount = ClipPolygonWithPlane<AXIS_FLAG::Y, W_SIGN_FLAG::NEGATIVE>(&outVertices[0], outCount, &inVertices[0]);
		outCount = ClipPolygonWithPlane<AXIS_FLAG::Z, W_SIGN_FLAG::POSITIVE>(&inVertices[0],  outCount, &outVertices[0]);
		outCount = ClipPolygonWithPlane<AXIS_FLAG::Z, W_SIGN_FLAG::NEGATIVE>(&outVertices[0], outCount, &inVertices[0]);

		Vertex::assignData(outputVertices, &inVertices[0], outCount);

		return outCount;
	}
};
#include <vector>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tgaimage.h>

#include "vertex.h"
#include "clip.h"

using namespace std;
using namespace glm;

struct FrameBuffer
{
	vector<vector<float>> zBuffer;
	vector<vector<vec3>> colorBuffer;
};

array<int, 4> getBBox(const array<vec3, 3>& tri, const int width, const int height)
{
	array<int, 4> bbox = { width, height, 0, 0 };
	for(int i=0;i<3;i++)
	{
		bbox[0] = std::min(bbox[0], static_cast<int>(tri[i].x));
		bbox[1] = std::min(bbox[1], static_cast<int>(tri[i].y));
		bbox[2] = std::max(bbox[2], static_cast<int>(tri[i].x));
		bbox[3] = std::max(bbox[3], static_cast<int>(tri[i].y));
	}
	bbox[0] = std::max(bbox[0], 0);
	bbox[1] = std::max(bbox[1], 0);
	bbox[2] = std::min(bbox[2], width);
	bbox[3] = std::min(bbox[3], height);
	return bbox;
}

vec3 getBarycentricCoord(const array<vec3, 3>& abc, const vec3& p)
{
	vec3 x = vec3(abc[1].x - abc[0].x, abc[2].x - abc[0].x, abc[0].x - p.x);
	vec3 y = vec3(abc[1].y - abc[0].y, abc[2].y - abc[0].y, abc[0].y - p.y);
	vec3 cross_z = cross(x, y);
	if (abs(static_cast<double>(cross_z.z)) < 0.01)
	{
		return vec3(-1, 1, 1);
	}
	return vec3(1 - (cross_z.x + cross_z.y) / cross_z.z, cross_z.x / cross_z.z, cross_z.y / cross_z.z);
}

vector<TriangleP> geometryProcess(const vector<Triangle>& triangles, 
								  const mat4& m, const mat4&v, const mat4& p, 
								  const int width, const int height)
{
	// vertex process1: modelSpace -> clipSpace
	size_t totalTriangles = triangles.size();
	vector<TriangleP> clipTriangles(totalTriangles);
	for(size_t i =0;i< totalTriangles; i++)
	{
		const Triangle& triangle = triangles[i];
		for(size_t j =0; j < triangle.vertices.size();j++)
		{
			// process in vertex shader
			vec3 pos = triangle.vertices[j].position;
			vec4 mvpPos = p * v * m * vec4(pos, 1.0f);

			// copy attribute
			clipTriangles[i].vertices[j].position = mvpPos;
			clipTriangles[i].vertices[j].color = triangle.vertices[j].color;
		}
	}

	// vertex process2: clipping in clipSpace
	vector<TriangleP> clippedTriangles(totalTriangles);
	Clipper<VertexP> clipper;
	size_t clippedCount = 0, totalClippedCount = totalTriangles;
	for(size_t i = 0;i < totalTriangles;i++)
	{
		array<VertexP, Clipper<VertexP>::MAX_OUTPUT_CLIPPED_POINT> clipVertices = {};
		const TriangleP& clipTriangle = clipTriangles[i];

		const size_t verticesCount = clipper.clipTriangle(&clipTriangle.vertices[0], &clipVertices[0]);

		if (verticesCount >= 3 && clippedCount + (verticesCount - 2) > totalClippedCount)
		{
			size_t addCount = clippedCount + (verticesCount - 2) - totalClippedCount + 2 * (totalClippedCount - i);
			clippedTriangles.insert(clippedTriangles.end(), addCount, {});
			totalClippedCount = clippedTriangles.size();
		}
		
		for(size_t j = 1; j + 1 < verticesCount; j++)
		{
			clippedTriangles[clippedCount++].vertices = { clipVertices[0], clipVertices[j], clipVertices[j + 1] };
		}
	}

	// vertex process3: clipSpace -> NDC and NDC -> ScreenSpace
	vector<TriangleP> screenTriangles(clippedCount);
	for(size_t i =0;i<clippedCount;i++)
	{
		const array<VertexP, 3> clippedVertices = clippedTriangles[i].vertices;
		for(size_t j=0; j< clippedVertices.size(); j++)
		{
			// projection division
			vec3 position = vec3(clippedVertices[j].position.x, clippedVertices[j].position.y, clippedVertices[j].position.z) / clippedVertices[j].position.w;
			
			// screen mapping
			position = (position + vec3(1.0, 1.0, 1.0)) * vec3(width, height, 1) / 2.0f;

			screenTriangles[i].vertices[j].position = vec4(position, -clippedVertices[j].position.w);
			screenTriangles[i].vertices[j].color = clippedVertices[j].color;
		}
	}
	return screenTriangles;
}

void rasterize(const vector<TriangleP>& triangles, FrameBuffer& frameBuffer)
{
	size_t height = frameBuffer.zBuffer.size(), width = frameBuffer.zBuffer[0].size();
	for(auto& triangle: triangles)
	{
		array<vec3, 3> triPos = {};
		for (size_t i = 0; i < 3; i++) {
			triPos[i] = vec3(triangle.vertices[i].position.x, triangle.vertices[i].position.y, triangle.vertices[i].position.z);
		}

		// perspective correct parameter
		vec3 pcPV = {
			triangle.vertices[1].position.w * triangle.vertices[2].position.w,
			triangle.vertices[0].position.w * triangle.vertices[2].position.w,
			triangle.vertices[0].position.w * triangle.vertices[1].position.w,
		};
		float pcPZ = triangle.vertices[0].position.w * triangle.vertices[1].position.w * triangle.vertices[2].position.w;

		array<int, 4> triBBox = getBBox(triPos, width-1, height-1);
		for (int y = triBBox[1]; y <= triBBox[3]; y++)
		{
			for (int x = triBBox[0]; x <= triBBox[2]; x++)
			{
				vec3 p = vec3(x + 0.5, y + 0.5, 0);
				vec3 baryC = getBarycentricCoord(triPos, p);
				if (baryC[0] >= 0 && baryC[1] >= 0 && baryC[2] >= 0)
				{
					// perspective projection interplote correct
					vec3 baryCCorrect = (pcPV / dot(pcPV, baryC)) * baryC;
					p.z = pcPZ / dot(pcPV, baryC);

					// depth test
					if (p.z < frameBuffer.zBuffer[y][x])
					{
						frameBuffer.zBuffer[y][x] = p.z;
						frameBuffer.colorBuffer[y][x] = baryCCorrect[0] * triangle.vertices[0].color +
							baryCCorrect[1] * triangle.vertices[1].color +
							baryCCorrect[2] * triangle.vertices[2].color;
					}
				}
			}
		}
	}
	
}

int main()
{
	const int width = 800, height = 600;

	// viewport
	FrameBuffer frameBuffer;
	frameBuffer.zBuffer = vector<vector<float>>(height + 1, vector<float>(width + 1, 2));
	frameBuffer.colorBuffer = vector<vector<vec3>>(height + 1, vector<vec3>(width + 1, vec3(0)));

	// data
	Triangle triangle = {};
	triangle.vertices[0] = { vec3(0, 0, 4.0), vec3(1,0,0) };
	triangle.vertices[1] = { vec3(-1, -0.7, 0), vec3(0,1,0) };
	triangle.vertices[2] = { vec3(1, -0.7, 0), vec3(0,0,1) };

	vector<Triangle> triangles(1, triangle);

	// set mvp matrix
	mat4 model = mat4(1.0);
	mat4 view = lookAt(vec3(0, 0, 3), vec3(0, 0, 0), vec3(0, 1, 0));
	mat4 projection = perspective(radians(60.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
	mat4 mvp = projection * view * model;
	
	// geometry process
	vector<TriangleP> screenTriangles = geometryProcess(triangles, model, view, projection, width, height);

	// rasterization and pix process
	rasterize(screenTriangles, frameBuffer);

	// color
	TGAImage image(width + 1, height + 1, TGAImage::RGB);
	TGAColor tgaColor;
	for(int y = 0; y<= height; y++)
	{
		for(int x = 0;x<=width;x++)
		{
			vec3 color = frameBuffer.colorBuffer[y][x] * 255.0f;
			tgaColor = TGAColor(
				static_cast<uint8_t>(color.r), 
				static_cast<uint8_t>(color.g), 
				static_cast<uint8_t>(color.b), 255);
			image.set(x, y, tgaColor);
		}
	}
	bool flag = image.write_tga_file("output/output.tga");	
	return 0;
}


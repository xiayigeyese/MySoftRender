#include <iostream>
#include <vector>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tgaimage.h>

using namespace std;
using namespace glm;

struct Vertex
{
	vec3 position;
	vec3 color;
};

struct Triangle
{
	array<Vertex, 3> vertices;
};

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
	if (abs(cross_z.z) < 0.01)
	{
		return vec3(-1, 1, 1);
	}
	return vec3(1 - (cross_z.x + cross_z.y) / cross_z.z, cross_z.x / cross_z.z, cross_z.y / cross_z.z);
}

vector<Triangle> geometryProcess(const vector<Triangle>& triangles, mat4& mvp, const int width, const int height)
{
	vector<Triangle> triangles_gp;
	for(auto& triangle: triangles)
	{
		// vertex shader: mvp
		array<vec4, 3> tri;
		for (int i = 0; i < 3; i++)
		{
			vec3 pos = triangle.vertices[i].position;
			tri[i] = mvp * vec4(pos, 1.0f);
		}

		// clip
	}
	return triangles_gp;
}

int main()
{
	const int width = 800, height = 600;

	// viewport
	FrameBuffer frameBuffer;
	frameBuffer.zBuffer = vector<vector<float>>(height + 1, vector<float>(width + 1, 2));
	frameBuffer.colorBuffer = vector<vector<vec3>>(height + 1, vector<vec3>(width+1, vec3(0)));
	
	// data
	Triangle triangle;
	triangle.vertices[0] = { vec3(-2, -0.5, 0), vec3(1,0,0) };
	triangle.vertices[1] = { vec3(0, 0.5, 0), vec3(0,1,0) };
	triangle.vertices[2] = { vec3(2, -0.5, 0), vec3(0,0,1) };

	// set mvp matrix
	mat4 model = mat4(1.0);
	mat4 view = lookAt(vec3(0, 0, 3), vec3(0, 0, 0), vec3(0, 1, 0));
	mat4 projection = perspective(radians(60.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
	mat4 mvp = projection * view * model;

	// vertex shader
	array<vec4, 3> tri_vs;
	for(int i=0;i<3;i++)
	{
		vec3 pos = triangle.vertices[i].position;
		tri_vs[i] = mvp * vec4(pos, 1.0f);
	}

	// clip


	// project divisor
	array<vec3, 3> tri_ndc;
	for(int i=0;i<3;i++)
	{
		tri_ndc[i] = vec3(tri_vs[i].x, tri_vs[i].y, tri_vs[i].z) / tri_vs[i].w;
	}

	// viewport transform
	array<vec3, 3> tri_ss;
	for(int i =0;i<3;i++)
	{
		float x = (tri_ndc[i].x + 1) * width / 2;
		float y = (tri_ndc[i].y + 1) * height / 2;
		float z = (tri_ndc[i].z + 1) / 2;
		tri_ss[i] = vec3(x, y, z);
	}

	// rasterization
	array<int, 4> tri_bbox = getBBox(tri_ss, width, height);
	
	for(int y = tri_bbox[1]; y <= tri_bbox[3];y++)
	{
		for(int x = tri_bbox[0]; x<=tri_bbox[2];x++)
		{
			vec3 p = vec3(x + 0.5, y + 0.5, 0);
			vec3 baryC = getBarycentricCoord(tri_ss, p);
			if (baryC[0] >= 0 && baryC[1]>=0 && baryC[2]>=0)
			{
				p.z = dot(baryC, vec3(tri_ss[0].z, tri_ss[1].z, tri_ss[2].z));
				if (p.z < frameBuffer.zBuffer[y][x])
				{
					frameBuffer.zBuffer[y][x] = p.z;
					vec3 color = triangle.vertices[0].color * baryC[0] + 
								 triangle.vertices[1].color * baryC[1] + 
								 triangle.vertices[2].color * baryC[2];
					frameBuffer.colorBuffer[y][x] = color;
				}
			}
		}
	}

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
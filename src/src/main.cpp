#include <vector>
#include <array>
#include <imgui.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tgaimage.h>

#include "vertex.h"
#include "clip.h"

using namespace std;
using namespace glm;

// ======== openGl ========

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void frameBuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

GLFWwindow * initWindow(const int width, const int height)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, "SoftRender", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return nullptr;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, frameBuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return nullptr;
	}
	return window;
}

unsigned int initShaderProgram()
{
	const char* vertexShaderSource = "#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"layout (location = 1) in vec3 aColor;\n"
		"layout (location = 2) in vec2 aTexCoord;\n"
		"out vec2 TexCoord;\n"
		"void main()\n"
		"{\n"
		"   TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
		"   gl_Position = vec4(aPos.xyz, 1.0);\n"
		"}\0";
	const char* fragmentShaderSource = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"in vec2 TexCoord;\n"
		"uniform sampler2D texture1;\n"
		"void main()\n"
		"{\n"
		"   FragColor = texture(texture1, TexCoord);\n"
		"}\n\0";

	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return shaderProgram;
}

void initBackgroundData(unsigned int &VAO, unsigned int& VBO, unsigned int& EBO)
{
	float vertices[] = {
		 1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top right
		 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
		-1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f  // top left 
	};
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 3,  // first Triangle
		1, 2, 3   // second Triangle
	};
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void initBackgroundTexture(unsigned int &texture)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// ======== imGui ========

void initImGui (GLFWwindow* window)
{
	const char* glsl_version = "#version 330";
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
}


// ======== SoftRender ========

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

void geometryProcess(vector<TriangleP>& screenTriangles,
					const vector<Triangle>& triangles, 
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
	if (screenTriangles.empty())
	{
		screenTriangles = vector<TriangleP>(clippedCount);
	}
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
}

void rasterize(const vector<TriangleP>& triangles, FrameBuffer& frameBuffer)
{
	size_t height = frameBuffer.zBuffer.size(), width = frameBuffer.zBuffer[0].size();
	for(auto& triangle: triangles)
	{
		array<vec3, 3> triPos = {};
		for (size_t i = 0; i < 3; i++) 
		{
			triPos[i] = vec3(triangle.vertices[i].position.x, triangle.vertices[i].position.y, triangle.vertices[i].position.z);
		}

		// perspective correct parameter
		vec3 pcPV = {
			triangle.vertices[1].position.w * triangle.vertices[2].position.w,
			triangle.vertices[0].position.w * triangle.vertices[2].position.w,
			triangle.vertices[0].position.w * triangle.vertices[1].position.w,
		};
		float pcPZ = triangle.vertices[0].position.w * triangle.vertices[1].position.w * triangle.vertices[2].position.w;

		array<int, 4> triBBox = getBBox(triPos, width - 1, height - 1);
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

	// openGL
	GLFWwindow* window = initWindow(width, height);
	unsigned int shaderProgram = initShaderProgram();
	unsigned int VAO, VBO, EBO;
	initBackgroundData(VAO, VBO, EBO);
	unsigned int texture;
	initBackgroundTexture(texture);

	// imGui
	initImGui(window);
	
	// viewport
	FrameBuffer frameBuffer;
	frameBuffer.zBuffer = vector<vector<float>>(height, vector<float>(width, 2));
	frameBuffer.colorBuffer = vector<vector<vec3>>(height, vector<vec3>(width, vec3(0.2f, 0.3f, 0.3f)));

	// data
	Triangle triangle = {};
	triangle.vertices[0] = { vec3(-1.0, -0.5, 0.0), vec3(1,0,0) };
	triangle.vertices[1] = { vec3(0, 1.0, 0), vec3(0,1,0) };
	triangle.vertices[2] = { vec3(1.0, -0.5, 0), vec3(0,0,1) };

	vector<Triangle> triangles(1, triangle);

	vector<unsigned char> imgData(3 * width * height);
	vector<TriangleP> screenTriangles;

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		
		// set mvp matrix
		mat4 model = mat4(1.0);
		mat4 view = lookAt(vec3(0, 0, 3), vec3(0, 0, 0), vec3(0, 1, 0));
		mat4 projection = perspective(radians(60.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
		// geometry process
		geometryProcess(screenTriangles, triangles, model, view, projection, width - 1, height - 1);
		// rasterization and pix process
		rasterize(screenTriangles, frameBuffer);
		for (size_t y = 0; y < height; y++)
		{
			for (size_t x = 0; x < width; x++)
			{
				vec3 color = frameBuffer.colorBuffer[y][x] * 255.0f;
				size_t i = (y * width + x) * 3;
				imgData[i] = static_cast<unsigned char>(color.r);
				imgData[i + 1] = static_cast<unsigned char>(color.g);
				imgData[i + 2] = static_cast<unsigned char>(color.b);
			}
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, &imgData[0]);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		
		ImGui::Begin("triangle");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glBindTexture(GL_TEXTURE_2D, texture);
		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(shaderProgram);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
}


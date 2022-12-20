#include "GLutility.h"
#include <iostream>
#include "scene.h"
#include <stb_image.h>
#include "shader.h"

using namespace std;

static void glfw_error_callback(int error, const char * desc)
{
	fputs(desc, stderr);
}

GL_Utility::GL_Utility(int width, int height, bool fullScreen)
{
	this->width = width;
	this->height = height;
	this->fullScreen = fullScreen;
	this->useCustomResolution = true;
}

bool GL_Utility::setup_window()
{
	if (!glfwInit())
		return false;

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();

	glfwSetErrorCallback(glfw_error_callback);

	window = glfwCreateWindow(width, height, "RayTracing", fullScreen ? monitor : NULL, NULL);
	glfwGetWindowSize(window, &width, &height);

	if (!window) {
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGL()) {
		printf("gladLoadGL failed!\n");
		return false;
	}
	printf("OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);

	return true;
}

void GL_Utility::draw(GLuint quadVAO)
{
	shader.use();
	glBindVertexArray(quadVAO);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	checkGlErrors("Draw a raytraced image");

	return;
}

void GL_Utility::create_shaders(raytDefines& defines)
{
	const std::string vertexShaderSrc = readStringFromFile(ASSETS_DIR "/shaders/vshader.vs");
	std::string fragmentShaderSrc = readStringFromFile(ASSETS_DIR "/shaders/fshader.fs");
	
	replace(fragmentShaderSrc, "{SPHERE_SIZE}", std::to_string(defines.sphere_size));
	replace(fragmentShaderSrc, "{SURFACE_SIZE}", std::to_string(defines.surface_size));
	replace(fragmentShaderSrc, "{BOX_SIZE}", std::to_string(defines.box_size));
	replace(fragmentShaderSrc, "{LIGHT_POINT_SIZE}", std::to_string(defines.light_point_size));
	replace(fragmentShaderSrc, "{LIGHT_DIRECT_SIZE}", std::to_string(defines.light_direct_size));
	replace(fragmentShaderSrc, "{ITERATIONS}", std::to_string(defines.iterations));
	replace(fragmentShaderSrc, "{AMBIENT_COLOR}", to_string(defines.ambient_color));
	replace(fragmentShaderSrc, "{SHADOW_AMBIENT}", to_string(defines.shadow_ambient));

	shader.createShader(vertexShaderSrc.c_str(), fragmentShaderSrc.c_str());

	shader.use();

	checkGlErrors("Shader creation");
}

std::string GL_Utility::to_string(glm::vec3 v)
{
	return std::string().append("vec3(").append(std::to_string(v.x)).append(",").append(std::to_string(v.y)).append(",").append(std::to_string(v.z)).append(")");
}

unsigned int GL_Utility::load_texture(char const* path, GLuint wrapMode)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		cout << "Texture failed to load at path: " << path << endl;
		stbi_image_free(data);
	}

	return textureID;
}

GLuint GL_Utility::load_texture(int texNum, const char* name, const char* uniformName, GLuint wrapMode)
{
	const std::string path = ASSETS_DIR "/textures/" + std::string(name);
	const unsigned int tex = load_texture(path.c_str(), wrapMode);
	shader.setInt(uniformName, texNum);
	textures.push_back(tex);
	return tex;
}

void GL_Utility::init_buffer(GLuint* ubo, const char* name, int bindingPoint, size_t size, void* data) const
{
	glGenBuffers(1, ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, *ubo);
	glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
	GLuint blockIndex = glGetUniformBlockIndex(shader.ID, name);
	if (blockIndex == 0xffffffff)
	{
		fprintf(stderr, "Invalid ubo block name '%s'", name);
		exit(1);
	}
	glUniformBlockBinding(shader.ID, blockIndex, bindingPoint);
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, *ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void GL_Utility::update_buffer(GLuint ubo, size_t size, void* data)
{
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

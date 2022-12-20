#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "shader.h"
#include "utils.h"

using namespace std;

struct raytDefines;

class GL_Utility
{
public:
	GL_Utility(int width, int height, bool fullScreen);

	bool setup_window();
	void create_shaders(raytDefines& defines);

	GLFWwindow* window;

	void draw(GLuint quadVAO);
	GLuint load_texture(int texNum, const char* name, const char* uniformName, GLuint wrapMode = GL_REPEAT);
	void init_buffer(GLuint* ubo, const char* name, int bindingPoint, size_t size, void* data) const;
	static void update_buffer(GLuint ubo, size_t size, void* data);

private:
	Shader shader;
	GLuint fboColor, fboTexColor, fboEdge, fboTexEdge, fboBlend, fboTexBlend;
	vector<GLuint> textures;

	int width;
	int height;

	bool fullScreen = true;
	bool useCustomResolution = false;

	void gen_framebuffer(GLuint* fbo, GLuint* fboTex, GLenum internalFormat, GLenum format) const;
	
	static GLuint load_texture(char const* path, GLuint wrapMode = GL_REPEAT);
	static std::string to_string(glm::vec3 v);
};


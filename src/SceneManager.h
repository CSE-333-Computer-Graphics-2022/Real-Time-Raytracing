#pragma once

#include "GLutility.h"
#include "scene.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

using namespace std;

class Scene_Manager
{
public:
	Scene_Manager(int screen_width, int screen_height, sceneContainer* scene, GL_Utility* wrapper);

	void init();
	void update(float frameRate);

	static raytMaterial createMaterial(glm::vec3 color, int specular, float reflect, float refract = 0.0, glm::vec3 absorb = {}, float diffuse = 0.7, float kd = 0.8, float ks = 0.2);
	static raytSphere createSphere(glm::vec3 center, float radius, raytMaterial material, bool hollow = false);
	static raytBox createBox(glm::vec3 pos, glm::vec3 form, raytMaterial material);
	static raytLightPoint createLightPoint(glm::vec4 position, glm::vec3 color, float intensity, float linear_k = 0.22f, float quadratic_k = 0.2f);
	static raytLightDirect createLightDirect(glm::vec3 direction, glm::vec3 color, float intensity);
	static raytScene createScene(int width, int height);

private:
	sceneContainer* scene;

	int screen_width;
	int screen_height;
	GL_Utility* util;
	
	bool w_pressed = false;
	bool a_pressed = false;
	bool s_pressed = false;
	bool d_pressed = false;
	bool ctrl_pressed = false;
	bool shift_pressed = false;
	bool space_pressed = false;
	bool alt_pressed = false;

	float lastX = 0;
	float lastY = 0;

	// Camera Attributes
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 world_up = glm::vec3(0, 1, 0);
	// Euler Angles
	float yaw = 0;
	float pitch = 0;

	GLuint sceneUbo = 0;
	GLuint sphereUbo = 0;
	GLuint surfaceUbo = 0;
	GLuint boxUbo = 0;
	GLuint lightPointUbo = 0;
	GLuint lightDirectUbo = 0;

	void scene_update(float deltaTime);
	void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void glfw_framebuffer_size_callback(GLFWwindow* wind, int width, int height);
	void glfw_mouse_callback(GLFWwindow* window, double xpos, double ypos);
	void init_buffers();
	void update_buffers() const;
	glm::vec3 get_color(float r, float g, float b);

	template<typename T>
	void init_buffer(GLuint* ubo, const char* name, int bindingPoint, vector<T>& v);
	template<typename T>
	void update_buffer(GLuint ubo, vector<T>& v) const;
};

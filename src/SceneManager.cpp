#include "SceneManager.h"
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <stb_image.h>

using namespace std;

Scene_Manager::Scene_Manager(int screen_width, int screen_height, sceneContainer* scene, GL_Utility* util)
{
	this->screen_width = screen_width;
	this->screen_height = screen_height;
	this->util = util;
	this->scene = scene;
	this->position = scene->scene.camera_pos;
}

void Scene_Manager::init()
{
	glfwSetWindowUserPointer(util->window, this);

	auto keyFunc = [](GLFWwindow* w, int a, int b, int c, int d)
	{
		static_cast<Scene_Manager*>(glfwGetWindowUserPointer(w))->glfw_key_callback(w, a, b, c, d);
	};
	auto mouseFunc = [](GLFWwindow* w, double x, double y)
	{
		static_cast<Scene_Manager*>(glfwGetWindowUserPointer(w))->glfw_mouse_callback(w, x, y);
	};

	glfwSetInputMode(util->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(util->window, mouseFunc);
	glfwSetKeyCallback(util->window, keyFunc);
	glfwSetFramebufferSizeCallback(util->window, glfw_framebuffer_size_callback);

	init_buffers();
}

void Scene_Manager::update(float deltaTime)
{
	scene_update(deltaTime);
	update_buffers();
}

void Scene_Manager::scene_update(float deltaTime)
{
	front.x = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(front);
	right = glm::normalize(glm::cross(-front, world_up));
	scene->scene.quat_camera_rotation = glm::quat(glm::vec3(glm::radians(-pitch), glm::radians(yaw), 0));

	auto speed = deltaTime * 3;
	if (shift_pressed)
		speed *= 3;
	if (alt_pressed)
		speed /= 3;

	glm::vec3 aq = front * speed * 1e3f;

	if (w_pressed)
		position += front * speed;
	if (a_pressed)
		position -= right * speed;
	if (s_pressed)
		position -= front * speed;
	if (d_pressed)
		position += right * speed;
	if (space_pressed)
		position += world_up * speed;
	if (ctrl_pressed)
		position -= world_up * speed;

	scene->scene.camera_pos = position;
}

void Scene_Manager::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_RELEASE) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, GL_TRUE);

		bool pressed = action == GLFW_PRESS;

		if (key == GLFW_KEY_W)
			w_pressed = pressed;
		else if (key == GLFW_KEY_A)
			a_pressed = pressed;
		else if (key == GLFW_KEY_S)
			s_pressed = pressed;
		else if (key == GLFW_KEY_D)
			d_pressed = pressed;
		else if (key == GLFW_KEY_SPACE)
			space_pressed = pressed;
		else if (key == GLFW_KEY_LEFT_CONTROL)
			ctrl_pressed = pressed;
		else if (key == GLFW_KEY_LEFT_SHIFT)
			shift_pressed = pressed;
		else if (key == GLFW_KEY_LEFT_ALT)
			alt_pressed = pressed;
	}
}

void Scene_Manager::glfw_framebuffer_size_callback(GLFWwindow* wind, int width, int height)
{
	glViewport(0, 0, width, height);
}

bool firstMouse = true;

void Scene_Manager::glfw_mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.002f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
}

raytMaterial Scene_Manager::createMaterial(glm::vec3 color, int specular, float reflect, float refract, glm::vec3 absorb, float diffuse, float kd, float ks)
{
	raytMaterial material = {};

	material.color = color;
	material.absorb = absorb;
	material.specular = specular;
	material.reflect = reflect;
	material.refract = refract;
	material.diffuse = diffuse;
	material.kd = kd;
	material.ks = ks;

	return material;
}

raytScene Scene_Manager::createScene(int width, int height)
{
	raytScene scene = {};

	scene.camera_pos = {};
	scene.canvas_height = height;
	scene.canvas_width = width;
	scene.bg_color = { 0, 0, 0 };
	scene.reflect_depth = 5;

	return scene;
}

raytSphere Scene_Manager::createSphere(glm::vec3 center, float radius, raytMaterial material, bool hollow)
{
	raytSphere sphere = {};
	sphere.obj = glm::vec4(center, radius);
	sphere.hollow = hollow;
	sphere.material = material;

	return sphere;
}

raytBox Scene_Manager::createBox(glm::vec3 pos, glm::vec3 form, raytMaterial material)
{
	raytBox box = {};
	box.form = form;
	box.pos = pos;
	box.mat = material;
	return box;
}

raytLightPoint Scene_Manager::createLightPoint(glm::vec4 position, glm::vec3 color, float intensity, float linear_k,
	float quadratic_k)
{
	raytLightPoint light = {};

	light.intensity = intensity;
	light.pos = position;
	light.color = color;
	light.linear_k = linear_k;
	light.quadratic_k = quadratic_k;

	return light;
}

raytLightDirect Scene_Manager::createLightDirect(glm::vec3 direction, glm::vec3 color, float intensity)
{
	raytLightDirect light = {};

	light.intensity = intensity;
	light.direction = direction;
	light.color = color;

	return light;
}

template<typename T>
void Scene_Manager::init_buffer(GLuint* ubo, const char* name, int bindingPoint, vector<T>& v)
{
	util->init_buffer(ubo, name, bindingPoint, sizeof(T) * v.size(), v.data());
}

void Scene_Manager::init_buffers()
{
	util->init_buffer(&sceneUbo, "scene_buf", 0, sizeof(raytScene), nullptr);
	init_buffer(&sphereUbo, "spheres_buf", 1, scene->spheres);
	init_buffer(&surfaceUbo, "surfaces_buf", 3, scene->surfaces);
	init_buffer(&boxUbo, "boxes_buf", 4, scene->boxes);
	init_buffer(&lightPointUbo, "lights_point_buf", 7, scene->lights_point);
	init_buffer(&lightDirectUbo, "lights_direct_buf", 8, scene->lights_direct);
}

template<typename T>
void Scene_Manager::update_buffer(GLuint ubo, vector<T>& v) const
{
	if (!v.empty())
	{
		util->update_buffer(ubo, sizeof(T) * v.size(), v.data());
	}
}

void Scene_Manager::update_buffers() const
{
	util->update_buffer(sceneUbo, sizeof(raytScene), &scene->scene);
	update_buffer(sphereUbo, scene->spheres);
	update_buffer(surfaceUbo, scene->surfaces);
	update_buffer(boxUbo, scene->boxes);
	update_buffer(lightPointUbo, scene->lights_point);
}

glm::vec3 Scene_Manager::get_color(float r, float g, float b)
{
	return glm::vec3(r / 255, g / 255, b / 255);
}
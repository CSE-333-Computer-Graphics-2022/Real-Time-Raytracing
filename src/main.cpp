#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GLutility.h"
#include "SceneManager.h"
#include "Surface.h"

using namespace std;

int screen_width = 1280;
int screen_height = 720;

int earth_spherenum = -1, box_num = -1;

int scene_update_box(sceneContainer& scene, float delta_Time, float time)
{
	if (box_num != -1)
	{
		raytBox* box = &scene.boxes[box_num];
		glm::quat q = glm::angleAxis(delta_Time, glm::vec3(0.5774, 0.5774, 0.5774));
		box->quat_rotation *= q;
	}
	return 0;
}

int scene_update_earth(sceneContainer& scene, float delta_Time, float time)
{
	if (earth_spherenum != -1) {
		raytSphere* earth = &scene.spheres[earth_spherenum];
		float earthSpeed = 0.50;
		earth->obj.x = cos(time * earthSpeed) * 2000;
		earth->obj.z = sin(time * earthSpeed) * 2000;

		earth->quat_rotation *= glm::angleAxis(delta_Time, glm::vec3(0, 1, 0));
	}
	return 0;
}

float quadVertices[] = 
{
	-1.0f, -1.0f, 0.0f, 0.0f,
	 1.0f, -1.0f, 1.0f, 0.0f,
	 1.0f,  1.0f, 1.0f, 1.0f,

	-1.0f, -1.0f, 0.0f, 0.0f,
	 1.0f,  1.0f, 1.0f, 1.0f,
	-1.0f,  1.0f, 0.0f, 1.0f
};

int main()
{
	GL_Utility glutil(screen_width, screen_height, false);
	
	// Setup window
    glutil.setup_window();
    glfwSwapInterval(1); // vsync

	sceneContainer scene = {};

	scene.scene = Scene_Manager::createScene(screen_width, screen_height);
	scene.scene.camera_pos = { 0, 0, -5 };
	scene.shadow_ambient = glm::vec3{ 0.1, 0.1, 0.1 };
	scene.ambient_color = glm::vec3{ 0.25, 0.25, 0.25 };

	// lights
	scene.lights_point.push_back(Scene_Manager::createLightPoint({ 3, 5, 0, 0.1 }, { 1, 1, 1 }, 25.5));
	scene.lights_direct.push_back(Scene_Manager::createLightDirect({ 3, -1, 1 }, { 1, 1, 1 }, 1.5));

	// red sphere
	scene.spheres.push_back(Scene_Manager::createSphere({ 6.7, 0, 3.8 }, 1,
		Scene_Manager::createMaterial({ 1, 0, 0 }, 100, 0.2), true));

	// transparent sphere
	scene.spheres.push_back(Scene_Manager::createSphere({ 0.5, 1, 4 }, 1,
		Scene_Manager::createMaterial({ 0, 0, 0.8 }, 200, 0.1, 1.125, { 1, 0, 2 }, 1), true));


	// earth
	raytSphere earth = Scene_Manager::createSphere({}, 500, Scene_Manager::createMaterial({}, 0, 0.0f));
	earth.textureNum = 1;
	scene.spheres.push_back(earth);
	earth_spherenum = scene.spheres.size() - 1;

	// cylinder
	raytMaterial cylinder_Material = Scene_Manager::createMaterial({ 150 / 255.0f, 255 / 255.0f, 50 / 255.0f }, 200, 0.2);
	raytSurface cylinder = Surfaces::Elliptic_Cylinder(1 / 2.0f, 1 / 2.0f, cylinder_Material);
	cylinder.pos = { -2, 0, 6 };
	cylinder.quat_rotation = glm::quat(glm::vec3(glm::radians(90.f), 0, 0));
	cylinder.yMin = -1;
	cylinder.yMax = 1;
	scene.surfaces.push_back(cylinder);

	// cone
	raytMaterial cone_Material = Scene_Manager::createMaterial({ 210 / 255.0f, 30 / 255.0f, 60 / 255.0f }, 200, 0.2);
	raytSurface cone = Surfaces::Elliptic_Cone(1 / 3.0f, 1 / 3.0f, 1, cone_Material);
	cone.pos = { -6, 4, 6 };
	cone.quat_rotation = glm::quat(glm::vec3(glm::radians(90.f), 0, 0));
	cone.yMin = -1;
	cone.yMax = 4;
	scene.surfaces.push_back(cone);

	// floor
	scene.boxes.push_back(Scene_Manager::createBox({ 0, -1.2, 6 }, { 10, 0.2, 5 },
		Scene_Manager::createMaterial({ 0.9, 0.7, 0 }, 100, 0.15)));

	// box
	raytBox box = Scene_Manager::createBox({ 4.2, 1, 6 }, { 1, 1, 1 },
		Scene_Manager::createMaterial({}, 50, 0.0));
	box.textureNum = 2;
	scene.boxes.push_back(box);
	box_num = scene.boxes.size() - 1;

	raytDefines defines = scene.get_defines();
	glutil.create_shaders(defines);

	Scene_Manager scene_manager(screen_width, screen_height, &scene, &glutil);
	scene_manager.init();

	auto earth_Tex = glutil.load_texture(1, "Earth Texture.jpg", "texture_sphere_1");
	auto box_Tex = glutil.load_texture(2, "container.png", "texture_box");

	float current_Time = glfwGetTime();
	float last_Frame = current_Time;
	float frames_Count = 0;

	// quad VAO
	GLuint quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glBindVertexArray(0);

	while (!glfwWindowShouldClose(glutil.window))
	{
		frames_Count++;
		float new_Time = glfwGetTime();
		float delta_Time = new_Time - current_Time;
		current_Time = new_Time;

		scene_update_box(scene, delta_Time, new_Time);
		scene_update_earth(scene, delta_Time, new_Time);
		scene_manager.update(delta_Time);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, earth_Tex);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, box_Tex);

		glutil.draw(quadVAO);

		glfwSwapBuffers(glutil.window);
		glfwPollEvents();
	}
    glfwDestroyWindow(glutil.window);
    glfwTerminate();   // close window

	return 0;
}

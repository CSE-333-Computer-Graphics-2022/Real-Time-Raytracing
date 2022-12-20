#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace std;

struct raytDefines
{
	int sphere_size;
	int surface_size;
	int box_size;
	int light_point_size;
	int light_direct_size;
	int iterations;
	glm::vec3 ambient_color;
	glm::vec3 shadow_ambient;
};

typedef struct {
	glm::vec3 color; 
	float _p1;

	glm::vec3 absorb;
	float diffuse;

	float reflect;
	float refract;
	int specular;
	float kd;

	float ks;
	float _padding[3];
} raytMaterial;

typedef struct {
	glm::quat quat_camera_rotation;
	glm::vec3 camera_pos; 
	float _p1;
    
    glm::vec3 bg_color; 
	int canvas_width;

	int canvas_height;
	int reflect_depth;
	float _padding[2];
} raytScene;

typedef struct {
	raytMaterial material;
	glm::vec4 obj; // pos + radius
	glm::quat quat_rotation = glm::quat(1, 0, 0, 0);
	int textureNum;
	bool hollow;
	float _padding[2];
} raytSphere;

typedef struct {
	raytMaterial mat;
	glm::quat quat_rotation = glm::quat(1, 0, 0, 0);
	glm::vec3 pos; 
	float _p1;
	glm::vec3 form;
	int textureNum;
} raytBox;

typedef struct {
	raytMaterial mat;
	glm::quat quat_rotation = glm::quat(1, 0, 0, 0);
	float xMin = -FLT_MAX;
	float yMin = -FLT_MAX;
	float zMin = -FLT_MAX;
	float _p0;
	float xMax = FLT_MAX;
	float yMax = FLT_MAX;
	float zMax = FLT_MAX;
	float _p1;
	glm::vec3 pos;
	float a; // x2
	float b; // y2
	float c; // z2
	float d; // z
	float e; // y
	float f; // const

	float _padding[3];
} raytSurface;

typedef enum { sphere, light } primitiveType;

struct raytLightDirect {
	glm::vec3 direction; 
	float _p1;
	glm::vec3 color;

	float intensity;
};

struct raytLightPoint {
	glm::vec4 pos; //pos + radius
	glm::vec3 color;
	float intensity;

	float linear_k;
	float quadratic_k;
	float _padding[2];
};

struct sceneContainer
{
	raytScene scene;
	glm::vec3 ambient_color;
	glm::vec3 shadow_ambient;
	vector<raytSphere> spheres;
	vector<raytSurface> surfaces;
	vector<raytBox> boxes;
	vector<raytLightPoint> lights_point;
	vector<raytLightDirect> lights_direct;

	raytDefines get_defines()
	{
		int sphs = static_cast<int>(spheres.size());
	    int surs = static_cast<int>(surfaces.size());
	    int boxs = static_cast<int>(boxes.size());
	    int lps = static_cast<int>(lights_point.size());
	    int lds = static_cast<int>(lights_direct.size());

		return { sphs, surs, boxs, lps, lds, scene.reflect_depth, ambient_color, shadow_ambient };
	}
};
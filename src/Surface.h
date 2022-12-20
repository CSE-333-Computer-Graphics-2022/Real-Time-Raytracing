#pragma once
#include "scene.h"

class Surfaces {
public:
    static raytSurface Elliptic_Cylinder(float a, float b, raytMaterial material)
	{
		raytSurface surface = {};
		surface.a = powf(a, -2);
		surface.b = powf(b, -2);
		surface.f = -1;
		surface.mat = material;
		return surface;
	} 

	static raytSurface Elliptic_Cone(float a, float b, float c, raytMaterial material)
	{
		raytSurface surface = {};
		surface.a = powf(a, -2);
		surface.b = powf(b, -2);
		surface.c = -powf(c, -2);
		surface.mat = material;
		return surface;
	}
};

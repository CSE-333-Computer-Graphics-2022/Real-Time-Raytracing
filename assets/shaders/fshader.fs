#version 330 core

struct raytMaterial {
	vec3 color;
	vec3 absorb;

	float diffuse;
	float reflection;
	float refraction;
	int specular;
	float kd;
	float ks;
};

struct raytScene {
	vec4 quat_camera_rotation;
	vec3 camera_pos;
	vec3 bg_color;

	int canvas_width;
	int canvas_height;

	int reflect_depth;
};

struct raytLightDirect {
	vec3 direction;
	vec3 color;

	float intensity;
};

struct raytLightPoint {
	vec4 pos; //pos + radius
	vec3 color;
	float intensity;

	float linear_k;
	float quadratic_k;
};

struct raytSphere {
	raytMaterial mat;
	vec4 obj;
	vec4 quat_rotation; // rotate normal
	int textureNum;
	bool hollow;
};

struct raytBox {
	raytMaterial mat;
	vec4 quat_rotation;
	vec3 pos;
	vec3 form;
	int textureNum;
};

struct raytSurface {
	raytMaterial mat;
	vec4 quat_rotation;
	vec3 v_min;
	vec3 v_max;
	vec3 pos;
	float a; // x2
	float b; // y2
	float c; // z2
	float d; // z
	float e; // y
	float f; // const	
};

struct hitRecord {
	raytMaterial mat;
  	vec3 normal;
	float bias_mult;
	float alpha;
};

int Total_Internal_Reflection = 1;
int Do_Fresnel = 1;
int Reflect_Reduce_Iteration = 1;
int Shadow_Enabled = 1;

out vec4 FragColor;

uniform sampler2D texture_sphere_1;

uniform sampler2D texture_box;

#define DBG 0
#define DBG_First_Value 1

#if DBG
bool dbgEd = false;
#endif

layout( std140 ) uniform scene_buf
{
    raytScene scene;
};

#define Light_Point_Size {LIGHT_POINT_SIZE}
layout( std140 ) uniform lights_point_buf
{
	#if Light_Point_Size == 0
	raytLightPoint lights_point[1];
	#else
	raytLightPoint lights_point[Light_Point_Size];
	#endif
};

#define Light_Direct_Size {LIGHT_DIRECT_SIZE}
layout( std140 ) uniform lights_direct_buf
{
	#if Light_Direct_Size == 0
	raytLightDirect lights_direct[1];
	#else
	raytLightDirect lights_direct[Light_Direct_Size];
	#endif
};

#define Sphere_Size {SPHERE_SIZE}
layout( std140 ) uniform spheres_buf
{
	#if Sphere_Size == 0
	raytSphere spheres[1];
	#else
	raytSphere spheres[Sphere_Size];
	#endif
};

#define Surface_Size {SURFACE_SIZE}
layout( std140 ) uniform surfaces_buf
{
	#if Suface_Size == 0
	raytSurface surfaces[1];
	#else
	raytSurface surfaces[Suface_Size];
	#endif
};

#define Box_Size {BOX_SIZE}
layout( std140 ) uniform boxes_buf
{
	#if Box_Size == 0
	raytBox boxes[1];
	#else
	raytBox boxes[Box_Size];
	#endif
};

int swap_xy(inout float x, inout float y)
{
	float temp = x;
	x = y;
	y = temp;
	return 0;
}
  
vec4 quatInv(vec4 q)
{ 
  	return vec4(-q.x, -q.y, -q.z, q.w) * (1 / dot(q, q));
  	       // quat_conj()
}

vec4 quatMult(vec4 q1, vec4 q2)
{ 
	vec4 qr;
	qr.x = (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y);
	qr.y = (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x);
	qr.z = (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w);
	qr.w = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z);
	return qr;
}

vec3 rotate(vec4 qr, vec3 v)
{ 
	vec4 qr_conj = vec4(-qr.x, -qr.y, -qr.z, qr.w); // quat_conj()
	vec4 q_pos = vec4(v.xyz, 0);
	vec4 q_tmp = quatMult(qr, q_pos);
	return quatMult(q_tmp, qr_conj).xyz;
}

vec3 get_Ray_Dir()
{
    int cw = scene.canvas_width;
    int ch = scene.canvas_height;
	vec3 result = vec3((gl_FragCoord.xy - vec2(cw, ch) / 2) / ch, 1);
	vec3 normalized_result = normalize(rotate(scene.quat_camera_rotation, result));
	return normalized_result;
}

int _dbg()
{
	#if DBG
	#if DBG_First_Value
	if (dbgEd)
		return 0;
	#endif
	
	ivec2 pixel_coords = ivec2 (gl_FragCoord.xy);
    FragColor =vec4(1,0,0,1);
	dbgEd = true;
	#endif
	return 0;
}

int _dbg(float value)
{
	#if DBG
	#if DBG_First_Value
	if (dbgEd)
		return 0;
	#endif
	value = clamp(value, 0, 1);
	ivec2 pixel_coords = ivec2 (gl_FragCoord.xy);
    FragColor = vec4(value,value,value,1);
	dbgEd = true;
	#endif
	return 0;
}

int _dbg(vec3 value)
{
	#if DBG
	#if DBG_First_Value
	if (dbgEd)
		return 0;
	#endif

    ivec2 pixel_coords = ivec2 (gl_FragCoord.xy);
    FragColor = vec4(clamp(value, vec3(0), vec3(1)),1);
	dbgEd = true;
	#endif
	return 0;
}

vec4 Sphere_Texture(vec3 sphereNormal, vec4 quat, int texNum) {
	if (quat != vec4(0,0,0,1)) {
		sphereNormal = rotate(quat, sphereNormal);
	}
	float Pi = 3.14159265358979;
	float u = 0.5 + atan(sphereNormal.z, sphereNormal.x) / (2.*Pi);
	float v = 0.5 - asin(sphereNormal.y) / Pi;
	vec2 uv = vec2(u, v);
	vec2 df = fwidth(uv);
	if(df.x > 0.5) df.x = 0.;

	vec4 color;
	if (texNum == 1) {
		color = textureLod(texture_sphere_1, uv, log2(max(df.x, df.y)*1024.));
	}

	return color;
}

bool intersect_Sphere(vec3 ro, vec3 rd, vec4 object, bool hollow, float tmin, out float t)
{
	float c = dot( ro - object.xyz, ro - object.xyz ) - object.w*object.w;
	float discriminant = dot( ro - object.xyz, rd ) * dot( ro - object.xyz, rd ) - c;
	if (discriminant < 0.0) 
	    return false;

	t = -(dot( ro - object.xyz, rd )) - sqrt(discriminant);
	if (hollow && t < 0.0) 
		t = -(dot( ro - object.xyz, rd )) + sqrt(discriminant);
	return t > 0 && t < tmin;
}

vec3 optNormal;

bool intersect_Box(vec3 ro, vec3 rd, int num, float tmin, out float t) 
{
	raytBox box = boxes[num];

	// ray-box intersection in box space           
    vec3 n = (1.0 / (rotate(box.quat_rotation, rd))) * (rotate(box.quat_rotation, ro - box.pos));
    vec3 k = abs(1.0 / (rotate(box.quat_rotation, rd)))*box.form;             // rotate() -> convert from ray to box space
	
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;
	
	if ( max( max( t1.x, t1.y ), t1.z ) > min( min( t2.x, t2.y ), t2.z ) || min( min( t2.x, t2.y ), t2.z ) < 0.0) 
	    return false;
    
	if ( max( max( t1.x, t1.y ), t1.z ) >= tmin)
		return false;

	vec3 nor = -sign(rotate(box.quat_rotation, rd)) * step(t1.yzx,t1.xyz) * step(t1.zxy,t1.xyz);
	t = max( max( t1.x, t1.y ), t1.z );
	// convert to ray space
	optNormal = rotate(quatInv(box.quat_rotation), nor);
	return true;
}

vec4 Box_Texture(vec3 pt, vec3 normal, int num) {
	raytBox box = boxes[num];
	vec3 pos = rotate(box.quat_rotation, box.pos);
	pt = rotate(box.quat_rotation, pt);
	normal = rotate(box.quat_rotation, normal);
	return abs(normal.x)*texture(texture_box, 0.5*(pt.zy - pos.zy)-vec2(0.5)) + 
			abs(normal.y)*texture(texture_box, 0.5*(pt.zx - pos.zx)-vec2(0.5)) + 
			abs(normal.z)*texture(texture_box, 0.5*(pt.xy - pos.xy)-vec2(0.5));
}

// begin surface section
bool check_Surface_Edges(vec3 o, vec3 d, inout float tMin, inout float tMax, vec3 v_min, vec3 v_max, float epsilon)
{
	vec3 pt = d * tMin + o;
	if (! (greaterThan(pt, v_min) == bvec3(true) && lessThan(pt, v_max) == bvec3(true))) 
	{
		if (tMax < epsilon) 
		    return false;
		pt = d * tMax + o;
		if (! (greaterThan(pt, v_min) == bvec3(true) && lessThan(pt, v_max) == bvec3(true)))
			return false;
		swap_xy(tMin, tMax);
	}
	return true;
}
bool intersect_Surface(vec3 ro, vec3 rd, int num, float tmin, out float t)
{
    float Float_max = 3.402823466e+38;
	raytSurface surface = surfaces[num];

	float d1 = rotate(surface.quat_rotation, rd).x;
	float d2 = rotate(surface.quat_rotation, rd).y;
	float d3 = rotate(surface.quat_rotation, rd).z;
	float o1 = rotate(surface.quat_rotation, ro - surface.pos).x;
	float o2 = rotate(surface.quat_rotation, ro - surface.pos).y;
	float o3 = rotate(surface.quat_rotation, ro - surface.pos).z;

	float p1 = 2 * surface.a * d1 * o1 + 2 * surface.b * d2 * o2 + 2 * surface.c * d3 * o3 + surface.d * d3 + d2 * surface.e;
	float p2 = surface.a * d1 * d1 + surface.b * d2 * d2 + surface.c * d3 * d3;
	float p3 = surface.a * o1 * o1 + surface.b * o2 * o2 + surface.c * o3 * o3 + surface.d * o3 + surface.e * o2 + surface.f;
	float p4 = sqrt(p1 * p1 - 4 * p2 * p3);

	//division by zero
	if (abs(p2) < 1e-6)
	{
		t = -p3 / p1;
		return t > tmin;
	}

	float min = Float_max;
	float max = Float_max;

	float epsilon = 1e-4;

	if ((-p1 - p4) / (2 * p2) < min && (-p1 - p4) / (2 * p2) > epsilon)
	{
		min = (-p1 - p4) / (2 * p2);
		max = (-p1 + p4) / (2 * p2);
	}

	if ((-p1 + p4) / (2 * p2) < min && (-p1 + p4) / (2 * p2) > epsilon)
	{
		min = (-p1 + p4) / (2 * p2);
		max = (-p1 - p4) / (2 * p2);
	}

	if (!check_Surface_Edges(ro, rd, min, max, surface.v_min, surface.v_max, epsilon))
		return false;

	t = min;
	return t < tmin;
}

vec3 get_Surface_Normal(vec3 ro, vec3 rd, float t, int num) {
	raytSurface surface = surfaces[num];

	vec3 tm = rotate(surface.quat_rotation, rd) * t + rotate(surface.quat_rotation, ro - surface.pos);

	vec3 normal = vec3(2 * surface.a * tm.x, 2 * surface.b * tm.y + surface.e, 2 * surface.c * tm.z + surface.d);
	normal = rotate(quatInv(surface.quat_rotation), normal);
	return normalize(normal);
}
// end surface section

float maxDist = 1000000.0;

int SPHERE = 0;
int SURFACE = 1;
int BOX = 2;
int POINT_LIGHT = 3;

float calc_Inter(vec3 ro, vec3 rd, out int num, out int type)
{
	float tmin = maxDist;
	float t;
	
	int i = 0;
	while (i < Light_Point_Size) {
	    if (intersect_Sphere(ro, rd, lights_point[i].pos, false, tmin, t)) {
			num = i; tmin = t; type = POINT_LIGHT;
		}
		i++;
	}
    
    i = 0;
	while (i < Surface_Size) {
		if (intersect_Surface(ro, rd, i, tmin, t)) {
			num = i; tmin = t; type = SURFACE;
		}
		i++;
	}

	i = 0;
	while (i < Sphere_Size) {
		if (intersect_Sphere(ro, rd, spheres[i].obj, spheres[i].hollow, tmin, t)) {
			num = i; tmin = t; type = SPHERE;
		}
		i++;
	}

	i = 0;
	while (i < Box_Size) {
		if (intersect_Box(ro, rd, i, tmin, t)) {
			num = i; tmin = t; type = BOX;
		}
		i++;
	}
	
 	return tmin;
}

float in_Shadow(vec3 ro, vec3 rd, float dist)
{
	float t;
	float shadow = 0;

	int i = 0;
	while (i < Sphere_Size) {
		if (intersect_Sphere(ro, rd, spheres[i].obj, false, dist, t)) 
		    shadow = 1;
		i++;
	}

    i = 0;
	while (i < Box_Size) {
		if (intersect_Box(ro, rd, i, dist, t)) 
		    shadow = 1;
		i++;
	}

    i = 0;
	while (i < Surface_Size) {
		if (intersect_Surface(ro, rd, i, dist, t)) 
		    shadow = 1;
		i++;
	}

	return min(shadow, 1);
}

#define Shadow_Ambient {SHADOW_AMBIENT}

int calculate_Shade2(vec3 light_dir, vec3 light_color, float intensity, vec3 pt, vec3 rd, raytMaterial material, vec3 normal, bool doShadow, float dist, float distDiv, inout vec3 diffuse, inout vec3 specular) {
	light_dir = normalize(light_dir);
	// diffuse
	light_color *= clamp(dot(normal, light_dir), 0.0, 1.0);
	if (Shadow_Enabled == 1)
	    if (doShadow) {
		    vec3 shadow = vec3(1 - in_Shadow(pt, light_dir, dist));
		    light_color *= max(shadow, Shadow_Ambient);
	}
	
	diffuse += light_color * material.color * material.diffuse * intensity / distDiv;
	
	//specular
	if (material.specular > 0) {
		vec3 reflection = reflect(light_dir, normal);
		specular += light_color * pow(clamp(dot(rd, reflection), 0.0, 1.0), material.specular) * intensity / distDiv;
	}
	return 0;
}

#define Ambient_Color {AMBIENT_COLOR}

vec3 calculate_Shade(vec3 pt, vec3 rd, raytMaterial material, vec3 normal, bool doShadow)
{
	float dist, distDiv;
	vec3 light_color, light_dir;
	vec3 diffuse = vec3(0);
	vec3 specular = vec3(0);

	vec3 pixelColor = Ambient_Color * material.color;

    int i = 0;
	while (i < Light_Point_Size) {
		raytLightPoint light = lights_point[i];
		light_color = light.color;
		light_dir = light.pos.xyz - pt;
		dist = length(light_dir);
		distDiv = 1 + light.linear_k * dist + light.quadratic_k * dist * dist;

		calculate_Shade2(light_dir, light_color, light.intensity, pt, rd, material, normal, doShadow, dist, distDiv, diffuse, specular);
		i++;
	}

	i = 0;
	while (i < Light_Direct_Size) {
		light_color = lights_direct[i].color;
		light_dir = - lights_direct[i].direction;
		dist = maxDist;
		distDiv = 1;

		calculate_Shade2(light_dir, light_color, lights_direct[i].intensity, pt, rd, material, normal, doShadow, dist, distDiv, diffuse, specular);
		i++;
	}

	pixelColor = pixelColor + diffuse * material.kd + specular * material.ks;
	return pixelColor;
}

float get_Fresnel(vec3 normal, vec3 rd, float reflection)
{
    float n_dot_v = clamp(dot(normal, -rd), 0.0, 1.0);
	return reflection + (1.0 - reflection) * pow(1.0 - n_dot_v, 5.0);
}

float Fresnel_Reflect_Amount(float n1, float n2, vec3 normal, vec3 incident, float refl)
{
    if (Do_Fresnel == 1) {
        // Schlick aproximation
        float r0 = (n1-n2) / (n1+n2);
        r0 *= r0;
        float cosX = -dot(normal, incident);
        if (n1 > n2) {
            float n = n1 / n2;
            float sinT2 = n * n * (1.0 - cosX * cosX);
            // Total internal reflection
            if (sinT2 > 1.0)
                return 1.0;
            cosX = sqrt(1.0 - sinT2);
        }
        float x = 1.0 - cosX;
        float ret = r0 + (1.0 - r0) * pow(x, 5.0);

        // adjust reflect multiplier for object reflectivity
        ret = (refl + (1.0 - refl) * ret);
        return ret;
    }
    else
    	return refl;
}

hitRecord get_hit_info(vec3 ro, vec3 rd, vec3 pt, float t, int num, int type) {
	hitRecord hr;
	if (type == SPHERE) {
		raytSphere sphere = spheres[num];
		hr = hitRecord(sphere.mat, normalize(pt - sphere.obj.xyz), 0, 1);
		if (sphere.textureNum != 0) {
			vec4 texColor = Sphere_Texture(hr.normal, sphere.quat_rotation, sphere.textureNum);
			hr.mat.color = texColor.rgb;
			hr.alpha = texColor.a;
		}
	}
	if (type == BOX) {
		raytBox box = boxes[num];
		hr = hitRecord(box.mat, optNormal, 0, 1);
		if (box.textureNum != 0) {
			hr.mat.color = Box_Texture(pt, optNormal, num).rgb;
		}
	}
	if (type == SURFACE) {
		hr = hitRecord(surfaces[num].mat, get_Surface_Normal(ro, rd, t, num), 0, 1);
	}
	
	float distance = length(pt - ro);
	hr.bias_mult = (9e-3 * distance + 35) / 35e3;

	return hr;
}

// get one-step reflection color for refractive objects
vec3 Reflected_Color(vec3 ro, vec3 rd)
{
	vec3 color = vec3(0);
	vec3 pt;
	int num, type;
	float t = calc_Inter(ro, rd, num, type);
	if (type == POINT_LIGHT) 
	    return lights_point[num].color;
	hitRecord hr;
	if (t < maxDist) {
		pt = ro + rd * t;
		hr = get_hit_info(ro, rd, pt, t, num, type);
		ro = dot(rd, hr.normal) < 0 ? pt + hr.normal * hr.bias_mult : pt - hr.normal * hr.bias_mult;
		color = calculate_Shade(ro, rd, hr.mat, hr.normal, true);
	}
	return color;
}

#define Iterations {ITERATIONS}

void main()
{
	float reflect_Multiplier, refract_Multiplier, tm;
	vec3 col;
    raytMaterial mat;
	vec3 pt,n;
	vec4 ob;

	vec3 mask = vec3(1.0);
	vec3 color = vec3(0.0);
	vec3 ro = vec3(scene.camera_pos);
	vec3 rd = get_Ray_Dir();
	float absorb_Distance = 0.0;
	int type = 0;
	int num;
	hitRecord hr;
	
	int i = 0;
	while (i < Iterations)
	{
		tm = calc_Inter(ro, rd, num, type);
		if (tm < maxDist)
		{
			pt = ro + rd * tm;
			hr = get_hit_info(ro, rd, pt, tm, num, type);

			if (type == POINT_LIGHT) {
				color += lights_point[num].color * mask;
				break;
			}

			mat = hr.mat;
			n = hr.normal;

			bool outside = dot(rd, n) < 0;
			n = outside ? n : -n;

			if (Total_Internal_Reflection == 1)
			    if (mat.refraction > 0) 
				    reflect_Multiplier = Fresnel_Reflect_Amount( outside ? 1 : mat.refraction,
													  	  outside ? mat.refraction : 1,
											 		      rd, n, mat.reflection);
			    else reflect_Multiplier = get_Fresnel(n,rd,mat.reflection);
			else
			    reflect_Multiplier = get_Fresnel(n,rd,mat.reflection);

			refract_Multiplier = 1 - reflect_Multiplier;

			if (mat.refraction > 0.0) // Refractive
			{
				if (outside && mat.reflection > 0)
				{
					color += Reflected_Color(pt + n * hr.bias_mult, reflect(rd, n)) * reflect_Multiplier * mask;
					mask *= refract_Multiplier;
				}
				else if (!outside) {
					absorb_Distance += tm;    
        			vec3 absorb = exp(-mat.absorb * absorb_Distance);
					mask *= absorb;
				}
				if (Total_Internal_Reflection == 1)
				    //todo: rd = reflect(..) instead of break
				    if (reflect_Multiplier >= 1)
					    break;
				
				ro = pt - n * hr.bias_mult;
				rd = refract(rd, n, outside ? 1 / mat.refraction : mat.refraction);
				if (Reflect_Reduce_Iteration == 1)
				    i--;
			}
			else if (mat.reflection > 0.0) // Reflective
			{
				ro = pt + n * hr.bias_mult;
				color += calculate_Shade(ro, rd, mat, n, true) * refract_Multiplier * mask;
				rd = reflect(rd, n);
				mask *= reflect_Multiplier;
			}
			else // Diffuse
			{
				color += calculate_Shade(pt + n * hr.bias_mult, rd, mat, n, true) * mask * hr.alpha;
				if (hr.alpha < 1) {
					ro = pt - n * hr.bias_mult;
					mask *= 1 - hr.alpha;
				} 
				else {
					break;
				}
			}
		} 
		i++;
	}
	#if DBG == 0
	FragColor = vec4(color,1);
	#else
	if (!dbgEd) 
	    FragColor = vec4(color,1);
	#endif
}
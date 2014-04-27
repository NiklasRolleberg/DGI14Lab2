#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"

using namespace std;
using glm::vec3;
using glm::mat3;


//---------------------------------------------------------------------------
// structures
struct Intersection
{
	vec3 position;
	float distance;
	int triangleIndex;
};


// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
SDL_Surface* screen;
int t;
vector<Triangle> triangles;

//camera
float focalLength;
vec3 campos;
vec3 defCampos;

float cameraAngle = 0;
float yaw = 0;

//Illumination
vec3 lightPos(0, -0.5, -0.7);
vec3 lightColor = 14.f * vec3(1, 1, 1);
vec3 indirectLight = 0.2f*vec3(1, 1, 1);



// ----------------------------------------------------------------------------
// FUNCTIONS

bool ClosestIntersection(
	vec3 start,
	vec3 dir,
	const vector<Triangle>& triangles,
	Intersection& closestIntersection
	);

bool ClosestIntersectionVCramer(
	vec3 start,
	vec3 dir,
	const vector<Triangle>& triangles,
	Intersection& closestIntersection
	);

void Update();
void Draw();
void RotateVec(vec3& in);
vec3 DirectLight(const Intersection& i);
vec3 cramersRule(vec3 col1,vec3 col2,vec3 col3, vec3 b);
float det(vec3 col1, vec3 col2, vec3 col3);

int main(int argc, char* argv[])
{
	screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);	
	LoadTestModel(triangles);

	campos = vec3(0, 0, -3);
	defCampos = vec3(0, 0, -3); 

	focalLength = SCREEN_WIDTH;

	t = SDL_GetTicks();	// Set start value for timer.

	while (NoQuitMessageSDL())
	{
		Update();
		Draw();
	}

	//SDL_SaveBMP(screen, "screenshot.bmp");
	return 0;
}

void Update()
{
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2 - t);
	t = t2;
	if (dt < 20) {
		SDL_Delay(20 - dt);
	}
	cout << "Render time: " << dt << " ms." << endl;
	Uint8* keystate = SDL_GetKeyState(0);
	if (keystate[SDLK_UP])
	{
		yaw -= 0.05;
		campos = defCampos;
		RotateVec(campos);
	}
	if (keystate[SDLK_DOWN])
	{
		yaw += 0.05;
		campos = defCampos;
		RotateVec(campos);
	}
	if (keystate[SDLK_LEFT])
	{
		cameraAngle += 0.05;
		campos = defCampos;
		RotateVec(campos);
	}
	if (keystate[SDLK_RIGHT])
	{
		cameraAngle -= 0.05;
		campos = defCampos;
		RotateVec(campos);
	}
	if (keystate[SDLK_SPACE]) //reset
	{
		campos = vec3(0, 0, -3);
		cameraAngle = 0;
		yaw = 0;
		lightPos = vec3(0, -0.5, -0.7);
	}
	if (keystate[SDLK_w])
	{
		lightPos.z += 0.1;
	}
	if (keystate[SDLK_s])
	{
		lightPos.z -= 0.1;
	}
	if (keystate[SDLK_d])
	{
		lightPos.x += 0.1;
	}
	if (keystate[SDLK_a])
	{
		lightPos.x-= 0.1;
	}

	if (keystate[SDLK_e]) //Zoom in
	{
		float l = sqrt(glm::dot(defCampos, defCampos));
		if (l >= 1.5) {
			float nl = l - 0.3;
			defCampos = defCampos*(nl / l);
			campos = defCampos;
			RotateVec(campos);
		}
	}
	if (keystate[SDLK_q]) //Zoom out
	{
		float l = sqrt(glm::dot(defCampos, defCampos));
		if (l > 0) {
			float nl = l + 0.3;
			defCampos = defCampos*(nl / l);
			campos = defCampos;
			RotateVec(campos);
		}
	}
}

void Draw()
{
	if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);
	Intersection closest;
	for (int y = 0; y<SCREEN_HEIGHT; ++y)
	{
		for (int x = 0; x<SCREEN_WIDTH; ++x)
		{
			vec3 dir(x - (SCREEN_WIDTH / 2), y - (SCREEN_HEIGHT / 2), focalLength);
			dir = glm::normalize(dir);
			RotateVec(dir);
			bool a = ClosestIntersectionVCramer(campos, dir, triangles, closest);
			if (a)
			{
				vec3 Dlight = DirectLight(closest);
				//PutPixelSDL(screen, x, y, (Dlight+indirectLight)*triangles[closest.triangleIndex].color);
				//PutPixelSDL(screen,x,y,triangles[closest.triangleIndex].color);

				vec3 color = (Dlight)*triangles[closest.triangleIndex].color;

				int bounces = 4;
				float r = 0.8;
				//Bounce 1
				Intersection bounce;
				if (bounces > 0 && ClosestIntersectionVCramer(closest.position, triangles[closest.triangleIndex].normal + dir, triangles, bounce))
				{

					color += r*(DirectLight(bounce))*triangles[bounce.triangleIndex].color;
					for (int i = 0; i < bounces - 1; i++) {
						if (ClosestIntersectionVCramer(bounce.position, triangles[bounce.triangleIndex].normal + dir, triangles, bounce))
						{
							color += (r)*(DirectLight(bounce))*triangles[bounce.triangleIndex].color;
						}
					}
				}

				//PutPixelSDL(screen, x, y, (Dlight + indirectLight+color)*triangles[closest.triangleIndex].color);
				//PutPixelSDL(screen, x, y, (Dlight + indirectLight + color)*triangles[closest.triangleIndex].color);
				PutPixelSDL(screen, x, y, (indirectLight)*color); //mirror
			}
			else {
				PutPixelSDL(screen, x, y, vec3(0,0,0));
			}
		}
		SDL_UpdateRect(screen, 0, 0, 0, 0);
	}

	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_UpdateRect(screen, 0, 0, 0, 0);
}


bool ClosestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closestIntersection)
{
	bool first = true;
	int index;
	float distance;
	vec3 pos;

	for (int i = 0; i < triangles.size(); i++)
	{
		Triangle triangle = triangles[i];
		vec3 v0 = triangle.v0;
		vec3 v1 = triangle.v1;
		vec3 v2 = triangle.v2;
		vec3 e1 = v1 - v0;
		vec3 e2 = v2 - v0;
		vec3 b = start - v0;
		vec3 x = cramersRule(-dir,e1,e2,b);

		float t = x.x;
		float u = x.y;
		float v = x.z;

		if ((v + u) <= 1 && v >= 0 && u >= 0 && t>0) {
			if (first || t < distance) {
				index = i;
				distance = t;
				first = false;
			}
		}
	}

	if (first == false){

		closestIntersection.triangleIndex = index;
		closestIntersection.distance = distance;
		closestIntersection.position = start+distance*dir;
		return true;
	}
	else {
		return false;
	}
}

// With cramers rule implemented and not drawing triangles facing the wrong way
bool ClosestIntersectionVCramer(vec3 start,vec3 dir,const vector<Triangle>& triangles,Intersection& closestIntersection)
{
	bool first = true;
	int index;
	float distance;
	vec3 pos;

	for (int i = 0; i < triangles.size(); i++)
	{
		Triangle triangle = triangles[i];

		if (glm::dot(triangle.normal, dir) < 0)
		{
			vec3 v0 = triangle.v0;
			vec3 v1 = triangle.v1;
			vec3 v2 = triangle.v2;
			vec3 e1 = v1 - v0;
			vec3 e2 = v2 - v0;
			vec3 b = start - v0;

			float A0 = det(-dir, e1, e2);
			float t = det(b, e1, e2) / A0;

			if (t > 0) 
			{
				float u = det(-dir, b, e2) / A0;

				if (u >= 0) 
				{
					float v = det(-dir, e1, b) / A0;

					if (u + v <= 1 && v >= 0) 
					{
						if (first || t < distance) 
						{
							index = i;
							distance = t;
							first = false;
						}
					}
				}
			}
		}
	}


	if (first == false){

		closestIntersection.triangleIndex = index;
		closestIntersection.distance = distance;
		closestIntersection.position = start + distance*dir;
		return true;
	}
	else {
		return false;
	}
}


void RotateVec(vec3& in) 
{
	// Rotate around y axis
	vec3 row1(cos(cameraAngle), 0, sin(cameraAngle));
	vec3 row2(0, 1, 0);
	vec3 row3(-sin(cameraAngle), 0, cos(cameraAngle));

	float x = glm::dot(row1, in);
	float y = glm::dot(row2, in);
	float z = glm::dot(row3, in);

	in.x = x;
	in.y = y;
	in.z = z;

	// Rotate around x axis
	row1 = vec3(1, 0, 0);
	row2 = vec3(0, cos(yaw), -sin(yaw));
	row3 = vec3(0, sin(yaw), cos(yaw));

	x = glm::dot(row1, in);
	y = glm::dot(row2, in);
	z = glm::dot(row3, in);

	in.x = x;
	in.y = y;
	in.z = z;
}

vec3 DirectLight(const Intersection& i) 
{
	vec3 normal = triangles[i.triangleIndex].normal;
	vec3 r = i.position - lightPos;

	float d = sqrt(r.x*r.x + r.y*r.y + r.z*r.z);
	float P;
	if (glm::dot(normal, r) < 0) {
		
		Intersection sh;
		bool a = ClosestIntersection(lightPos, glm::normalize(r), triangles, sh);
		if (a) {
			if (sh.distance+0.001 < d) {
				P = 0;// (0.5 / (4.f* 3.14*d*d));
			}
			else{
				P = (1.f / (4.f* 3.14*d*d));
			}
		}
		else{
			P = (1.f / (4.f* 3.14*d*d));
		}
		vec3 light = P * lightColor;
		return light;
	}
	return vec3(0, 0, 0);
}

vec3 cramersRule(vec3 col1, vec3 col2, vec3 col3, vec3 b)
{
	float A0 = det(col1, col2, col3);
	float Ax = det(b, col2, col3);
	float Ay = det(col1, b, col3);
	float Az = det(col1, col2, b);

	return vec3(Ax/A0,Ay/A0,Az/A0);
}

float det(vec3 col1, vec3 col2, vec3 col3) 
{
	float plus = (col1[0] * col2[1] * col3[2]) + (col1[1] * col2[2] * col3[0]) + (col1[2] * col2[0] * col3[1]);
	float minus = (col1[2] * col2[1] * col3[0]) + (col1[1] * col2[0] * col3[2]) + (col1[0] * col2[2] * col3[1]);

	return plus - minus;
}

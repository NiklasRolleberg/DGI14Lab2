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

const int SCREEN_WIDTH = 200;
const int SCREEN_HEIGHT = 200;
SDL_Surface* screen;
int t;
vector<Triangle> triangles;
float focalLength;

//vec3 defaultCampos;
vec3 campos;
float cameraAngle = 0;
float yaw = 0;

// ----------------------------------------------------------------------------
// FUNCTIONS

bool ClosestIntersection(
	vec3 start,
	vec3 dir,
	const vector<Triangle>& triangles,
	Intersection& closestIntersection
	);

void Update();
void Draw();
void RotateVec(vec3& in); // rev- option to mirror rotation

int main(int argc, char* argv[])
{
	screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT);	
	LoadTestModel(triangles);

	campos = vec3(0, 0, -3);
	focalLength = SCREEN_WIDTH;

	t = SDL_GetTicks();	// Set start value for timer.

	
	while (NoQuitMessageSDL())
	{
		Update();
		Draw();
	}
	

	//Update();
	//Draw();
	//Update();
	//SDL_Delay(50000);


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
	//cout << "Render time: " << dt << " ms." << endl;
	Uint8* keystate = SDL_GetKeyState(0);
	if (keystate[SDLK_UP])
	{
		yaw -= 0.05;
		campos = vec3(0, 0, -3);
		RotateVec(campos);
	}
	if (keystate[SDLK_DOWN])
	{
		yaw += 0.05;
		campos = vec3(0, 0, -3);
		RotateVec(campos);
	}
	if (keystate[SDLK_LEFT])
	{
		cameraAngle += 0.05;
		campos = vec3(0, 0, -3);
		RotateVec(campos);
	}
	if (keystate[SDLK_RIGHT])
	{
		cameraAngle -= 0.05;
		campos = vec3(0, 0, -3);
		RotateVec(campos);
	}
	if (keystate[SDLK_SPACE]) // reset
	{
		campos = vec3(0, 0, -3);
		cameraAngle = 0;
		yaw = 0;
	}
	/*
	if (keystate[SDLK_PAGEUP]) //Zoom in
	{
		float l = sqrt(defaultCampos.x*defaultCampos.x + defaultCampos.y*defaultCampos.y + defaultCampos.z*defaultCampos.z);
		defaultCampos.x = defaultCampos.x*(l + 1) / l;
		defaultCampos.y = defaultCampos.y*(l + 1) / l;
		defaultCampos.z = defaultCampos.z*(l + 1) / l;
	}
	if (keystate[SDLK_PAGEUP]) //Zoom out
	{
		float l = sqrt(defaultCampos.x*defaultCampos.x + defaultCampos.y*defaultCampos.y + defaultCampos.z*defaultCampos.z);
		if (l > 1) {
			defaultCampos.x = defaultCampos.x*(l - 1) / l;
			defaultCampos.y = defaultCampos.y*(l - 1) / l;
			defaultCampos.z = defaultCampos.z*(l - 1) / l;
		}
	}*/
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
			//cout << dir.x << endl;
			RotateVec(dir);
			bool a = ClosestIntersection(campos, dir, triangles, closest);
			if (a)
			{
				PutPixelSDL(screen,x,y,triangles[closest.triangleIndex].color);
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

	for (int i = 0; i < triangles.size(); i++)
	{
		Triangle triangle = triangles[i];

		//vec3 normal = triangle.normal;
		//float d = normal.x*dir.x + normal.y*dir.y + normal.z*dir.z;

			vec3 v0 = triangle.v0;
			vec3 v1 = triangle.v1;
			vec3 v2 = triangle.v2;
			vec3 e1 = v1 - v0;
			vec3 e2 = v2 - v0;
			vec3 b = start - v0;
			mat3 A(-dir, e1, e2);
			vec3 x = glm::inverse(A) * b;

			float r = abs(x.x);
			float u = x.y;
			float v = x.z;

			if ((v + u) <= 1 && v >= 0 && u >= 0) {
				if (first || r < distance) {
					index = i;
					distance = r;
					first = false;
				}
			}
	}


	if (first == false){

		closestIntersection.triangleIndex = index;
		//closestIntersection.distance = distance;
		//closestIntersection.position = start + distance*dir;
		return true;
	}
	else {
		return false;
	}
}

void RotateVec(vec3& in) {

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
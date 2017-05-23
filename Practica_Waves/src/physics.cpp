#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw_gl3.h>
#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include "GL_framework.h"
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <forward_list>
#include <map>
#include <math.h>

bool show_test_window = false;
int updateRange = 20;
bool euler = true;
int waterfallIncrementX = -3;
float timePerFrame = 0.033;
float lenght = 0.5;
//float d;
int eixX = 14;
int eixY = 18;
int maxMesh = eixX*eixY;

glm::vec3 initial;
float cont = 0;
//void Spring(int part1, int part2);
float cD = 1.f;
#define WATER 1000;
#define TEST 10.f;

glm::vec3 normal;
float gravity = -9.81f;

struct Particle {
	glm::vec3 pos;
	glm::vec3 posInitial;
	glm::vec3 lastPos;
	glm::vec3 vel;
	glm::vec3 lastVel;
	glm::vec3 force;
};

struct OurShpere {
	glm::vec3 pos;
	glm::vec3 vel;
	float rad;
	float mass = 20;
	glm::vec3 linealMomentum;
	glm::vec3 force;

};

OurShpere *sphere = new OurShpere();

struct Waves {
	float amplitude;
	glm::vec3 k;
	float freq;
	float phi;
};
int const lengthWaves = 4;
Waves waves[lengthWaves]; 
std::forward_list <float> listaPos;


void GUI() {
	{	//FrameRate
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		//GUI Waterfall
	
		ImGui::SliderFloat("Time", &cont, 0, 20);
		ImGui::SliderFloat("Mass", &sphere->mass, 1, 300);
		ImGui::SliderFloat("Buoyancy", &cD, 0, 1);
	}

	// ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);

		ImGui::ShowTestWindow(&show_test_window);
	}
}

namespace LilSpheres {
	extern const int maxParticles;
	extern void setupParticles(int numTotalParticles, float radius = 0.05f);
	extern void cleanupParticles();
	extern void updateParticles(int startIdx, int count, float* array_data);
	extern void drawParticles(int startIdx, int count);
}
namespace ClothMesh {
	extern void setupClothMesh();
	extern void cleanupClothMesh();
	extern void updateClothMesh(float* array_data);
	extern void drawClothMesh();
}
namespace Sphere {
	extern glm::vec3 centro = { 0.f, 1.f, 0.f };
	extern void setupSphere(glm::vec3 pos = centro, float radius = 1.f);
	extern void cleanupSphere();
	extern void updateSphere(glm::vec3 pos, float radius = 1.f);
	extern void drawSphere();

}

float *mesh = new float[3 * maxMesh];
Particle *pC = new Particle[LilSpheres::maxParticles];


namespace GerstnerWaves {

	glm::vec3 Wave(float dt, Particle pC) {
		glm::vec3 temp;
		
		glm::vec3 x = glm::vec3(0.f);
		float y = 0.f;

		for (int i = 0; i < 2; i++) {
			x += (waves[i].k / glm::normalize((waves[i].k))* waves[i].amplitude * glm::sin(glm::dot(waves[i].k, pC.posInitial) - waves[i].freq * dt + waves[i].phi));
			y += waves[i].amplitude + waves[i].amplitude * glm::cos(glm::dot(waves[i].k, pC.posInitial) - waves[i].freq * dt + waves[i].phi);
		}
		temp = glm::vec3(pC.posInitial.x - x.x, y , pC.posInitial.z - x.z);
		
		return temp;
	}

	void Buoyancy() {
		//primer trobar Vsub
		//std::forward_list <glm::vec3> vecPos;
		std::map <float, glm::vec3> mapPos;
		float dis, sum = 0;
		float mitjana = 0;
		float yEnfonsat;
		glm::vec3 yNorm = glm::vec3(0, 1, 0);
		float density = WATER;

		for (int i = 0; i < LilSpheres::maxParticles; i++) {
			dis = glm::distance(sphere->pos, glm::vec3(pC[i].pos.x, 0.f, pC[i].pos.z));
			mapPos[dis] = pC[i].pos;
		}
		int j = 0;
		for (std::map<float, glm::vec3>::iterator it = mapPos.begin(); it != mapPos.end(); it++) {
			sum += it->second.y;
			j++;
			if (j >= 4) {
				mitjana = sum / 4;
				break;
			}
		}
		//checks if
		if (sphere->pos.y - sphere->rad < mitjana) {

			yEnfonsat = glm::distance(sphere->pos.y - sphere->rad, mitjana);
			float volumSub;
			volumSub = yEnfonsat * pow(sphere->rad, 2);
			sphere->force = density * glm::abs(gravity) * volumSub * yNorm;

			//drag force section (nice joke xd!)
				glm::vec3 dragForce;
				//module of relative force
				float moduloVel = glm::sqrt(glm::pow(sphere->vel.x, 2) + glm::pow(sphere->vel.y, 2) + glm::pow(sphere->vel.z, 2));
				//applying the formula
				dragForce = -0.5f * density * cD * glm::abs(volumSub) *  moduloVel * sphere->vel;
				sphere->force += dragForce;
			
		}
		//classic fm
		else sphere->force = glm::vec3(0, gravity * sphere->mass, 0);

		
	}
}


void PhysicsInit() {
	srand((unsigned)time(NULL));
	/*int randomXZ = rand() % 11 + (-5);
	float randomY = rand() % 10;*/
	//sphere->pos = glm::vec3(randomXZ, randomY, randomXZ)
	sphere->pos = glm::vec3 (0.f, 5.f, 0.f);
	sphere->rad = ((float)rand() / RAND_MAX) * 0.5f + 1;
	

	float randFreq = ((float)rand() / RAND_MAX);
	//1a ona
	if (randFreq > 0.2) {
		waves[0].amplitude = 1.0;
		waves[0].freq = randFreq * 2;
		waves[0].phi = 0.f;
		waves[0].k = glm::vec3(0.5f, 0, 0.2f);

		//2a ona
		waves[1].amplitude = 0.8;
		waves[1].freq = randFreq * 1;
		waves[1].phi = 2.f;
		waves[1].k = glm::vec3(0.1f, 0, 0.6f);
		//3a ona
		waves[2].amplitude = 0.9;
		waves[2].freq = randFreq * 3;
		waves[2].phi = 0.f;
		waves[2].k = glm::vec3(0.1f, 0, 0.6f);
		//4a ona
		waves[3].amplitude = 0.3;
		waves[3].freq = randFreq * 5;
		waves[3].phi = 3.f;
		waves[3].k = glm::vec3(0.8f, 0, 0.3f);
	}
	//Grid
	for (int i = 0; i < LilSpheres::maxParticles; ++i) {
		
		mesh[3 * i + 0] = pC[i].pos.x = -3 + lenght * (i % 14);
		mesh[3 * i + 1] = pC[i].pos.y = 5;
		mesh[3 * i + 2] = pC[i].pos.z = (14 * lenght) / 2 + lenght - lenght * (i / 14);

		pC[i].vel = glm::vec3(0.f, 0.f, 0.f);
		pC[i].force = glm::vec3(0.f, 0.f, 0.f);
		pC[i].posInitial = glm::vec3(pC[i].pos.x, pC[i].pos.y, pC[i].pos.z);
	}
	sphere->linealMomentum = glm::vec3(0, -9.8f, 0);
	ClothMesh::updateClothMesh(mesh);
}


void PhysicsUpdate(float dt) {


	cont += dt;
	
	if (cont >= 20) {
		PhysicsInit();
		cont = 0;
	}

	//euler
	for (int i = 0; i < maxMesh; i++)
	{
		pC[i].pos = GerstnerWaves::Wave(cont, pC[i]);

		mesh[3 * i + 0] = pC[i].pos.x;
		mesh[3 * i + 1] = pC[i].pos.y;
		mesh[3 * i + 2] = pC[i].pos.z;
	}
	GerstnerWaves::Buoyancy();

	sphere->linealMomentum = sphere->linealMomentum + dt * sphere->force;
	sphere->vel = sphere->linealMomentum / sphere->mass;
	sphere->pos = sphere->pos + dt * sphere->vel;

	Sphere::updateSphere(sphere->pos, sphere->rad);
	ClothMesh::updateClothMesh(mesh);



}


void PhysicsCleanup() {

	//TODO
	delete[] mesh;
	//delete[] pC;
}

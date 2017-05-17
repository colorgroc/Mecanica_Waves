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


bool show_test_window = false;
int updateRange = 20;
bool euler = true;
int waterfallIncrementX = -3;
float timePerFrame = 0.033;
float lenght = 0.5;
float d;
int eixX = 14;
int eixY = 18;
int maxMesh = eixX*eixY;
float Ks = 0;
float KdS = 10;
float KdSh = 8;
float KdB = 5;
float coefElasticity = 0.2;
float coefFriction = 0.1;
glm::vec3 temp;
glm::vec3 vTangencial;
bool collision = false;
float col;
glm::vec3 initial;
float cont = 0;
//void Spring(int part1, int part2);

#define WATER 1000;
#define TEST 10;

glm::vec3 normal;
glm::vec3 gravity = glm::vec3(0, -9.8, 0);

struct Particle {
	glm::vec3 pos;
	glm::vec3 posInitial;
	glm::vec3 lastPos;
	glm::vec3 vel;
	glm::vec3 lastVel;
	glm::vec3 force;
};

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


struct OurShpere {
	glm::vec3 pos;
	float rad;
	float mass = 3;
	glm::vec3 force;
};



float *mesh = new float[3 * maxMesh];
Particle *pC = new Particle[LilSpheres::maxParticles];
OurShpere *sphere = new OurShpere();


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

		for (int i = 0; i < LilSpheres::maxParticles; i++) {
			for (std::map<float, glm::vec3>::iterator it = mapPos.begin(); it != mapPos.end(); it++) {
				dis = glm::distance(sphere->pos, glm::vec3(pC[i].pos.x, 0.f, pC[i].pos.z));
				mapPos[dis] = pC[i].pos;
			}
		}
		int j = 0;
		for (std::map<float, glm::vec3>::iterator it = mapPos.begin(); it != mapPos.end(); it++) {
			sum += it->second.y;
			j++;
			if (j >= 4) {
				mitjana = sum / 4;
				break;
			}
			//to be continued
			
		}

	}


}


void PhysicsInit() {
	srand((unsigned)time(NULL));
	/*int randomXZ = rand() % 11 + (-5);
	float randomY = rand() % 10;*/
	//sphere->pos = glm::vec3(randomXZ, randomY, randomXZ)
	sphere->pos = glm::vec3 (0.f, 5.f, 0.f);
	sphere->rad = ((float)rand() / RAND_MAX) * 0.5f + 1;
	

	for (int i = 0; i < lengthWaves; i++) {

		float randFreq = ((float)rand() / RAND_MAX);
		glm::vec3 randK = glm::vec3(((float)rand() / RAND_MAX), 0.f, ((float)rand() / RAND_MAX));
		float randAmpl = ((float)rand() / RAND_MAX);
			waves[i].amplitude = randAmpl + i;
			waves[i].freq = randFreq + i;
			waves[i].phi = ((float)rand() / RAND_MAX);
			waves[i].k = randK;

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
	Sphere::updateSphere(sphere->pos, sphere->rad);
	ClothMesh::updateClothMesh(mesh);



}


void PhysicsCleanup() {

	//TODO
	delete[] mesh;
	//delete[] pC;
}

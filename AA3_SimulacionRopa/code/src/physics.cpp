#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include <glm\vec3.hpp>
#include <glm\geometric.hpp>
#include <vector>

// CONSTANTS:
	// Plane Array ID (index):
#define BOX_TOP 0
#define BOX_BOTTOM 1
#define BOX_LEFT 2
#define BOX_RIGHT 3
#define BOX_FRONT 4
#define BOX_BACK 5
	// Amount of planes for Box:
#define TOTAL_BOX_PLANES 6

// Cloth dimensions:
#define CLOTH_WIDTH 14
#define CLOTH_HEIGHT 18

#define RESET_TIME 20.0f

// Type of Capsule collision:
enum class CapsuleCollision { A, B, BODY, NONE };

// Data structure for Particles:
struct Particle {
	glm::vec3 pos = { 0,0,0 };
	glm::vec3 vel = { 0,0,0 };
	glm::vec3 prevPos = { 0,0,0 };
	glm::vec3 prevVel = { 0,0,0 };
	float mass;
};
// Data structure for Planes:
struct Plane {
	glm::vec3 normal;
	float d;
};

struct SphereObj {
	glm::vec3 pos;
	float radius;
};

#pragma region FunctionDeclaration
void ConvertClothToGPU(float *gpu);
Particle MoveParticle(Particle particle, float dt);
void ClothInit();
Particle CollideParticle(Particle particle);
bool checkPlaneCollision(Plane plane, Particle particle);
bool checkSphereCollision(Particle particle);
#pragma endregion Declaration of functions, defined below the main code

#pragma region GlobalVariables
float *clothGPU;
Particle cloth[CLOTH_WIDTH][CLOTH_HEIGHT];
Plane planes[TOTAL_BOX_PLANES];
glm::vec3 gravity = 9.81f * glm::vec3(0, -1, 0);
float clothHeight = 8;
SphereObj sphere;
float resetTime = 0;
#pragma endregion

namespace ClothMesh {
	extern void setupClothMesh();
	extern void cleanupClothMesh();
	extern void updateClothMesh(float* array_data);
	extern void drawClothMesh();
}

namespace Sphere {
	extern void setupSphere(glm::vec3 pos = glm::vec3(0.f, 1.f, 0.f), float radius = 1.f);
	extern void cleanupSphere();
	extern void updateSphere(glm::vec3 pos, float radius = 1.f);
	extern void drawSphere();
}

bool show_test_window = false;
void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, ImGuiWindowFlags_AlwaysAutoResize);
	// Do your GUI code here....
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate
		ImGui::Separator();
		ImGui::SliderFloat("Cloth Height", &clothHeight, 0, 10);
		ImGui::Text("Time before resetting: %.2f", RESET_TIME - resetTime);
		if (ImGui::Button("Reset now!")) { resetTime = RESET_TIME; }
	}
	// .........................


	ImGui::End();

	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if(show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}

void PhysicsInit() {
	// Do your initialization code here...
	// ...................................

	clothGPU = new float[CLOTH_WIDTH*CLOTH_HEIGHT * 3];

	ClothInit();

	ClothMesh::setupClothMesh();

	ConvertClothToGPU(clothGPU);	// Takes cloth as input and outputs to clothGPU.
	ClothMesh::updateClothMesh(clothGPU);

	// Box planes setup:
	Plane current;
	
		// Top:
	current.normal = glm::vec3(0, -1, 0);
	current.d = 10;
	planes[BOX_TOP] = current;
		// Bottom:
	current.normal = glm::vec3(0, 1, 0);
	current.d = 0;
	planes[BOX_BOTTOM] = current;
		// Left:
	current.normal = glm::vec3(1, 0, 0);
	current.d = -5;
	planes[BOX_LEFT] = current;
		// Right:
	current.normal = glm::vec3(-1, 0, 0);
	current.d = 5;
	planes[BOX_RIGHT] = current;
		// Front:
	current.normal = glm::vec3(0, 0, -1);
	current.d = 5;
	planes[BOX_FRONT] = current;
		// Back:
	current.normal = glm::vec3(0, 0, 1);
	current.d = -5;
	planes[BOX_BACK] = current;

	sphere.pos = { 0,0,0 };
	sphere.radius = 2.f;
	Sphere::setupSphere(sphere.pos, sphere.radius);
}

void PhysicsUpdate(float dt) {
	// Do your update code here...
	// ...........................

	resetTime += dt;
	if (resetTime > RESET_TIME) {
		ClothInit();
		resetTime = 0;
	}

	for (int y = 0; y < CLOTH_HEIGHT; y++) {
		for (int x = 0; x < CLOTH_WIDTH; x++) {
			cloth[x][y] = MoveParticle(cloth[x][y], dt);
			cloth[x][y] = CollideParticle(cloth[x][y]);
			
			cloth[x][y].prevPos = cloth[x][y].pos;
			cloth[x][y].prevVel = cloth[x][y].vel;
		}
	}

	ConvertClothToGPU(clothGPU);	// Takes cloth as input and outputs to clothGPU.
	ClothMesh::updateClothMesh(clothGPU);
	ClothMesh::drawClothMesh();

	Sphere::updateSphere(sphere.pos, sphere.radius);
	Sphere::drawSphere();
	
	// Pasamos del array apto para CPU al array apto para GPU:
	//
}

void PhysicsCleanup() {
	// Do your cleanup code here...
	// ............................
	//cloth = nullptr;
	clothGPU = nullptr;
	delete cloth;
	delete clothGPU;

	ClothMesh::cleanupClothMesh();
	Sphere::cleanupSphere();

}

//	 SPECIAL FUNCTIONS

void ClothInit() {
	// Init cloth particle values:
	//int x = 0;
	for (int x = 0; x < CLOTH_WIDTH; x++) {
		for (int y = 0; y < CLOTH_HEIGHT; y++) {
			cloth[x][y].pos = { 4.5f - x*0.7f, clothHeight, 5 - y*0.55f };
			cloth[x][y].prevPos = cloth[x][y].pos;
			cloth[x][y].vel = { 0, 0, 0 };
			cloth[x][y].prevVel = cloth[x][y].vel;
			cloth[x][y].mass = 1;
		}
	}
}

void ConvertClothToGPU(float *gpu){
	int gpuIndex = 0;
	int x = 0;
	for (int y = 0; y < CLOTH_HEIGHT; y++) {
		for (int x = 0; x < CLOTH_WIDTH; x++) {
			gpu[gpuIndex] = cloth[x][y].pos.x;
			gpuIndex++;
			gpu[gpuIndex] = cloth[x][y].pos.y;
			gpuIndex++;
			gpu[gpuIndex] = cloth[x][y].pos.z;
			gpuIndex++;
		}
	}
}

Particle MoveParticle(Particle particle, float dt) {
	glm::vec3 newPos, newVel;

	newPos = particle.pos + dt * particle.vel;
	newVel = particle.vel + dt * gravity; // Assuming mass is 1.
	particle.pos = newPos;
	particle.vel = newVel;

	return particle;
}

Particle CollideParticle(Particle particle) {
	glm::vec3 newPos, newVel;

	newPos = particle.pos;
	newVel = particle.vel;
	
	// Check collision:
	for (int j = 0; j < TOTAL_BOX_PLANES; j++) {
		glm::vec3 mirroredPos, mirroredVel;
		glm::vec3 collPoint;
		Plane colPlane = planes[j];

		// Plane collision:
		if (checkPlaneCollision(colPlane, particle)) {
			// Mirror position and velocity:
			glm::vec3 mirroredPos, mirroredVel;
			mirroredPos = newPos - 1 * (glm::dot(colPlane.normal, newPos))*colPlane.normal;
			mirroredVel = newVel - 1 * (glm::dot(colPlane.normal, newVel))*colPlane.normal;

			particle.pos = mirroredPos;
			particle.vel = mirroredVel;
		}

		//Sphere Collision:
		if (checkSphereCollision(particle)) {
			collPoint = particle.pos;	// CUTRE!! TENDR�A QUE SER EL PUNTO EXACTO!
			Plane colPlane;
			colPlane.normal = collPoint - sphere.pos;
			colPlane.normal = glm::normalize(colPlane.normal);
			colPlane.d = -1 * (colPlane.normal.x*collPoint.x + colPlane.normal.y*collPoint.y + colPlane.normal.z*collPoint.z);
			mirroredPos = newPos - 2 * (glm::dot(colPlane.normal, newPos) + colPlane.d)*colPlane.normal;
			mirroredVel = newVel - 2 * (glm::dot(colPlane.normal, newVel) + colPlane.d)*colPlane.normal;

			particle.pos = mirroredPos;
			particle.vel = mirroredVel;
		}
	}

	return particle;
}

// Check for collision with a plane:
bool checkPlaneCollision(Plane plane, Particle particle) {
	bool hasCollided = false;
	if (glm::dot(glm::dot(plane.normal, particle.prevPos),glm::dot(plane.normal, particle.pos)) <= 0) {
		hasCollided = true;
	}
	return hasCollided;
}

// Check for collision with a sphere:
bool checkSphereCollision(Particle particle) {
	bool hasCollided = false;
	if (glm::distance(particle.pos, sphere.pos) < sphere.radius) {
		hasCollided = true;
	}
	return hasCollided;
}
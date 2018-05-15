#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include <glm\vec3.hpp>
#include <glm\geometric.hpp>
#include <vector>

#pragma region CONSTANTS
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
#define FLUID_WIDTH 14
#define FLUID_HEIGHT 18

#define RESTART_DELAY 15

#define MAX_WAVES 5
#pragma endregion All the defines.

#pragma region DataStructures
// Data structure for Planes:
struct Node {
	glm::vec3 pos;
	glm::vec3 initPos;
};

struct Fluid {
	Node nodes[FLUID_WIDTH][FLUID_HEIGHT];
};

struct Wave {
	float amplitude;
	float frequency;
	glm::vec3 wavevector;
	float offset; // phi = phase of the wave. (diapo 27).
};

struct Plane {
	glm::vec3 normal;
	float d;
};

struct SphereObj {
	glm::vec3 pos;
	float radius;
};
#pragma endregion Node, Fluid, Wave, Plane, SphereObj

#pragma region FunctionDeclaration
Fluid InitFluid();
Wave InitWave();
void ConvertFluidToGPU(float *gpu, Fluid _fluid);
#pragma endregion Declaration of functions, defined below the main code

#pragma region GlobalVariables
float *fluidGPU = new float[FLUID_WIDTH*FLUID_HEIGHT];
Fluid fluid;
Wave waves[MAX_WAVES];
Plane planes[TOTAL_BOX_PLANES];
float startHeight = 5;
float clock;
int activeWaves = 0;
int prevActiveWaves = 0;
#pragma endregion

#pragma region NamespaceDependencies
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
#pragma endregion

#pragma region GL_Framework_Functions
bool show_test_window = false;
void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, ImGuiWindowFlags_AlwaysAutoResize);
	// Do your GUI code here....
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate
		ImGui::Text("Simulation Clock: %.1f s", clock);
		if (ImGui::Button("Reset Fluid")) {
			clock = 0; fluid = InitFluid();
		}
		ImGui::Separator();
		ImGui::Text("Active Waves: %d", activeWaves);
		ImGui::SliderInt("Waves", &activeWaves, 0, MAX_WAVES);
		ImGui::Separator();
		ImGui::SliderFloat("Wave Start Height", &startHeight, 0, 10);
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
	fluid = InitFluid();
	//ConvertFluidToGPU(fluidGPU, fluid);
	//ClothMesh::updateClothMesh(fluidGPU);
}

void PhysicsUpdate(float dt) {
	clock += dt;
	if (prevActiveWaves != activeWaves) {	// Number of waves changed
		for (int i = 0; i < activeWaves; i++) { waves[i] = InitWave(); }
	}
	if (clock > RESTART_DELAY) { clock = 0; fluid = InitFluid(); }

	ConvertFluidToGPU(fluidGPU, fluid);
	ClothMesh::updateClothMesh(fluidGPU);

	// Set previous wave count:
	prevActiveWaves = activeWaves;
}

void PhysicsCleanup() {
	fluidGPU = nullptr;
	delete fluidGPU;
}
#pragma endregion

#pragma region FluidFunctions

Fluid InitFluid() {
	Fluid _fluid;
	for (int i = 0; i < FLUID_WIDTH; i++) {
		for (int j = 0; j < FLUID_HEIGHT; j++) {
			_fluid.nodes[i][j].pos = glm::vec3(i, startHeight, j);
		}
	}
	return _fluid;
}

Wave InitWave() {
	Wave wv;
	wv.amplitude = rand() % 10;
	wv.frequency = rand() % 10;
	wv.offset = rand() % 10;
	wv.wavevector.x = rand() % 10;
	wv.wavevector.y = 0;	// ???
	wv.wavevector.z = rand() % 10;
	return wv;
}

void ConvertFluidToGPU(float *gpu, Fluid _fluid){
	int gpuIndex = 0;
	for (int j = 0; j < FLUID_HEIGHT; j++) {
		for (int i = 0; i < FLUID_WIDTH; i++) {
			gpu[gpuIndex] = 0;
			gpuIndex++;
			gpu[gpuIndex] = 0;
			gpuIndex++;
			gpu[gpuIndex] = 0;
			gpuIndex++;
		}
	}
}

void CalculateWave(Fluid fl, Wave wave, float dt) {	// Gerstner Wave calculation
	for (int i = 0; i < FLUID_WIDTH; i++) {
		for (int j = 0; j < FLUID_HEIGHT; j++) {
			Node node = fl.nodes[i][j];
			node.pos.x = node.initPos.x - (wave.wavevector.x) * wave.amplitude * sin(glm::dot(wave.wavevector, node.initPos) - wave.frequency*dt);
			node.pos.z = node.initPos.z - (wave.wavevector.z) * wave.amplitude * sin(glm::dot(wave.wavevector, node.initPos) - wave.frequency*dt);
			node.pos.y = wave.amplitude * cos(glm::dot(wave.wavevector, node.initPos) - wave.frequency * dt);
		}
	}
}

#pragma endregion
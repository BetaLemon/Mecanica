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

#define INIT_X_OFFSET -FLUID_WIDTH/2
#define INIT_Z_OFFSET -FLUID_HEIGHT/2

#define RESTART_DELAY 15

#define MAX_WAVES 5

#define PI 3.1415f
#define G 9.81f
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

struct Ball {
	glm::vec3 pos;
	glm::vec3 vel;
	glm::vec3 force;
	float mass;
	float radius;
	float dragCoeff; // Cd de la formula de Drag Forces.
};
#pragma endregion Node, Fluid, Wave, Plane, SphereObj

#pragma region FunctionDeclaration
Fluid InitFluid();
Wave InitWave();
void InitAllWaves(Wave * _waves);
Ball InitBall();
void ConvertFluidToGPU(float *gpu, Fluid _fluid);
Fluid CalculateWaves(Fluid fl, Wave *waves, int numOfWaves, float dt);
Ball EulerSolver(Ball b, float dt);
Ball CalculateBallForces(Ball b, float dt);
Ball AddGravity(Ball b);
float GenerateRandom(float min, float max);
#pragma endregion Declaration of functions, defined below the main code

#pragma region GlobalVariables
float *fluidGPU = new float[FLUID_WIDTH*FLUID_HEIGHT];
Fluid fluid;
Wave waves[MAX_WAVES];
Plane planes[TOTAL_BOX_PLANES];
float startHeight = 5;
float clock;
float speed = 1;
int activeWaves = 1;
int prevActiveWaves = 1;
Ball ball;
extern bool renderSphere;
// GUI:
bool disableReset = false;
bool showBall = true;
bool showCustomWave = false;
bool showCustomBall = false;
Wave customWave = { 0.1, 0.1, glm::vec3(1,0,0), 0 };
Ball customBall = { glm::vec3(0,startHeight + 1,0), glm::vec3(0), glm::vec3(0), 1, 1 };
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
		ImGui::Separator();
		if(!disableReset) ImGui::Text("Simulation Clock: %.1f s (Restarts at 15s)", clock);
		else ImGui::Text("Simulation Clock: %.1f s (Doesn't restart)", clock);
		ImGui::Checkbox("Disable Reset", &disableReset);
		if (ImGui::Button("Reset Fluid")) {
			clock = 0; fluid = InitFluid(); InitAllWaves(waves);
		}
		ImGui::SliderFloat("Speed (Breaks Ball)", &speed, 1, 20);
		ImGui::Separator();
		ImGui::Text("Active Waves: %d", activeWaves);
		ImGui::SliderInt("Waves", &activeWaves, 1, MAX_WAVES);
		ImGui::Separator();
		ImGui::SliderFloat("Wave Start Height", &startHeight, 0, 10);
		ImGui::Checkbox("Make your own wave!", &showCustomWave);
		if (showCustomWave) {
			ImGui::Separator();
			ImGui::DragFloat("Amplitude", &customWave.amplitude, 0.05f, 0, 1);
			ImGui::DragFloat("Frequency", &customWave.frequency, 0.5f, 0, 20);
			ImGui::DragFloat3("Direction", &customWave.wavevector[0]);
			ImGui::DragFloat("Offset (Phi)", &customWave.offset);
			if (ImGui::Button("Apply!")) {
				InitAllWaves(waves);
			}
		}
		ImGui::Separator();
		ImGui::Checkbox("Show Ball", &showBall);
		if (showBall) {
			if (ImGui::Button("Reset Ball")) {
				ball = InitBall();
			}
			ImGui::Checkbox("Customize Ball", &showCustomBall);
			if (showCustomBall) {
				ImGui::DragFloat3("Init. Position", &customBall.pos[0]);
				ImGui::DragFloat3("Init. Velocity", &customBall.vel[0]);
				ImGui::DragFloat("Mass", &customBall.mass);
				ImGui::DragFloat("Radius", &customBall.radius);
				ImGui::DragFloat("Drag Coefficient", &customBall.dragCoeff, 0.2f, 0, 5);
				if(ImGui::Button("Apply!")){
					ball = InitBall();
				}
			}
		}
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
	ball = InitBall();
	InitAllWaves(waves);

	//ConvertFluidToGPU(fluidGPU, fluid);
	//ClothMesh::updateClothMesh(fluidGPU);
}

void PhysicsUpdate(float dt) {
	clock += dt;
	if (prevActiveWaves != activeWaves) {	// Number of waves changed
		InitAllWaves(waves);
	}
	if (clock > RESTART_DELAY && !disableReset) {
		clock = 0;
		fluid = InitFluid(); 
		ball = InitBall();
		InitAllWaves(waves);
	}

	//for(int i = 0; i < activeWaves; i++) { fluid = CalculateWave(fluid, waves[i], clock); }
	fluid = CalculateWaves(fluid, waves, activeWaves, clock*speed);

	ConvertFluidToGPU(fluidGPU, fluid);
	ClothMesh::updateClothMesh(fluidGPU);

	if (showBall) {
		ball = CalculateBallForces(ball, clock);
		ball = EulerSolver(ball, dt * speed);
		Sphere::updateSphere(ball.pos, ball.radius);
	}

	renderSphere = showBall;


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
			_fluid.nodes[i][j].pos = glm::vec3(i + INIT_X_OFFSET, startHeight, j + INIT_Z_OFFSET);
			_fluid.nodes[i][j].initPos = _fluid.nodes[i][j].pos;
		}
	}
	return _fluid;
}

Wave InitWave() {
	Wave wv;
	wv.amplitude = ((double)rand() / (RAND_MAX));
	wv.frequency = GenerateRandom(0.1f, 5);
	wv.offset = ((double)rand() / (RAND_MAX));
	wv.wavevector.x = GenerateRandom(-1, 1);
	wv.wavevector.y = GenerateRandom(-1, 1);
	wv.wavevector.z = GenerateRandom(-1, 1);
	if (showCustomWave) { wv = customWave; }
	return wv;
}

void InitAllWaves(Wave * _waves) {
	for (int i = 0; i < activeWaves; i++) { _waves[i] = InitWave(); }
}

Ball InitBall() {
	Ball tmp;
	tmp.pos.x = GenerateRandom(-5, 5);
	tmp.pos.y = GenerateRandom(startHeight+1, 15);
	tmp.pos.z = GenerateRandom(-5, 5);
	tmp.radius = 1;
	tmp.mass = 1;
	tmp.force = glm::vec3(0);
	tmp.vel = glm::vec3(0);
	tmp.dragCoeff = GenerateRandom(0, 5);
	if (showCustomBall) { tmp = customBall; }
	return tmp;
}

void ConvertFluidToGPU(float *gpu, Fluid _fluid){
	int gpuIndex = 0;
	for (int j = 0; j < FLUID_HEIGHT; j++) {
		for (int i = 0; i < FLUID_WIDTH; i++) {
			gpu[gpuIndex] = _fluid.nodes[i][j].pos.x;
			gpuIndex++;
			gpu[gpuIndex] = _fluid.nodes[i][j].pos.y;
			gpuIndex++;
			gpu[gpuIndex] = _fluid.nodes[i][j].pos.z;
			gpuIndex++;
		}
	}
}

// DEPRECATED:
Fluid CalculateWave(Fluid fl, Wave wave, float dt) {	// Gerstner Wave calculation
	for (int i = 0; i < FLUID_WIDTH; i++) {
		for (int j = 0; j < FLUID_HEIGHT; j++) {
			Node node = fl.nodes[i][j];
			node.pos.x = node.initPos.x - (wave.wavevector.x) * wave.amplitude * sin(glm::dot(wave.wavevector, node.initPos) - wave.frequency*dt);
			node.pos.z = node.initPos.z - (wave.wavevector.z) * wave.amplitude * sin(glm::dot(wave.wavevector, node.initPos) - wave.frequency*dt);
			node.pos.y = wave.amplitude * cos(glm::dot(wave.wavevector, node.initPos) - wave.frequency * dt);
			fl.nodes[i][j] = node;
		}
	}
	return fl;
}

Fluid CalculateWaves(Fluid fl, Wave *w, int numOfWaves, float dt) {
	for (int i = 0; i < FLUID_WIDTH; i++) {
		for (int j = 0; j < FLUID_HEIGHT; j++) {
			Node node = fl.nodes[i][j];
			glm::vec3 waveCalc = glm::vec3(0);
			for (int i = 0; i < numOfWaves; i++) {
				waveCalc.x += (glm::normalize(w[i].wavevector).x) * w[i].amplitude * sin(glm::dot(w[i].wavevector, node.initPos) - w[i].frequency*dt);
				waveCalc.z += (glm::normalize(w[i].wavevector).z) * w[i].amplitude * sin(glm::dot(w[i].wavevector, node.initPos) - w[i].frequency*dt);
				waveCalc.y += w[i].amplitude * cos(glm::dot(w[i].wavevector, node.initPos) - w[i].frequency * dt);
			}

			node.pos.x = node.initPos.x - waveCalc.x;
			node.pos.z = node.initPos.z - waveCalc.z;
			node.pos.y = node.initPos.y + waveCalc.y;
			fl.nodes[i][j] = node;
		}
	}
	return fl;
}

float CalculateSphereSubmergeHeight(Ball sp, Wave * w, int numOfWaves, float dt) {
	float height = startHeight;

	for (int i = 0; i < numOfWaves; i++) {
		height += w[i].amplitude * cos(glm::dot(w[i].wavevector, sp.pos) - w[i].frequency * dt);
	}
	
	if (sp.pos.y > height) {	// ball center is over the surface
		if (sp.pos.y - sp.radius > height) {	// ball center+radius over surface (the ball is in the air).
			height = 0;
		}
		else {	// center is above height, but ball touches water.
			height = height - (sp.pos.y - sp.radius);
		}
	}
	else {	// ball center is under the surface
		if (sp.pos.y + sp.radius > height) {	// ball center is under surface, but ball is not fully submerged.
			height = height - sp.pos.y;
		}
		else {	// ball is fully submerged.
			height = sp.radius * 2;
		}
	}

	return height;
}

float CalculateSphereVolume(Ball sp, float h) {	// h is the submerged sphere height.
	/* https://en.wikipedia.org/wiki/Spherical_cap */
	return ((PI * pow(h, 2)) / 3) * (3 * sp.radius - h);
}

float CalculateDragArea(Ball b, float h) {
	float circleRadius = sqrt(pow(b.radius, 2) - pow((b.radius - h), 2));
	float circleArea = PI * pow(circleRadius, 2);
	return circleArea;
}

glm::vec3 CalculateBuoyancyForce(Ball sp, float dt) {
	float h, vol;
	h = CalculateSphereSubmergeHeight(sp, waves, activeWaves, dt);
	vol = CalculateSphereVolume(sp, h);

	return G * vol * glm::vec3(0, 1, 0);
}

glm::vec3 CalculateDragForce(Ball b, float dt) {
	// Considering that density is 1.
	float h = CalculateSphereSubmergeHeight(b, waves, activeWaves, dt);
	float area = CalculateDragArea(b, h);
	return -0.5f * b.dragCoeff * area * b.vel;
}

Ball EulerSolver(Ball b, float dt) {
	b.pos.x = b.pos.x + dt * b.vel.x;
	b.pos.y = b.pos.y + dt * b.vel.y;
	b.pos.z = b.pos.z + dt * b.vel.z;

	b.vel.x = b.vel.x + dt * b.force.x / b.mass;
	b.vel.y = b.vel.y + dt * b.force.y / b.mass;
	b.vel.z = b.vel.z + dt * b.force.z / b.mass;

	return b;
}

Ball CalculateBallForces(Ball b, float dt) {
	b = AddGravity(b);
	glm::vec3 buoy = CalculateBuoyancyForce(b, dt);
	b.force += buoy;
	if (buoy != glm::vec3(0)) { b.force += CalculateDragForce(b, dt); }
	return b;
}

Ball AddGravity(Ball b) {
	b.force = glm::vec3(0, -1 * G, 0);
	return b;
}

float GenerateRandom(float min, float max) {
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = max - min;
	float r = random * diff;
	return min + r;
}

#pragma endregion
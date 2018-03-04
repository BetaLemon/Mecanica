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

	// Maximum lifespan of particles:
//#define MAX_PARTICLE_LIFE 3
float MAX_PARTICLE_LIFE = 3;

// Type of Emitter being used:
enum class ParticleEmitterType{ FOUNTAIN, CASCADE };
// Data structure for Particles:
struct Particle {
	glm::vec3 pos;
	glm::vec3 vel;
	float life;
	glm::vec3 prevPos;
	glm::vec3 prevVel;
	bool isAlive(){
		return life < MAX_PARTICLE_LIFE;
	};
};
// Data structure for Planes:
struct Plane {
	glm::vec3 normal;
	float d;
};

// Declaration of functions, defined below the main code:
bool checkCollision(Plane plane, Particle particle);
Particle GenerateParticle();
void ParticlesUpdate(float dt);
void ParticlesToGPU();

//Particle particles[SHRT_MAX];	// Esto aquí muy feo.
float *particlesGPU;
Particle *particles;
Plane planes[TOTAL_BOX_PLANES];
ParticleEmitterType emitterType = ParticleEmitterType::CASCADE;
glm::vec3 gravity = 9.81f * glm::vec3(0, -1, 0);

glm::vec3 sourcePos = { 0, 5, 0 };
glm::vec3 sourceAngle = { 0, -1, 0 };
float inertia;
int EmissionRate = 1;

namespace LilSpheres {															// Parámetros:
	extern void updateParticles(int startIdx, int count, float* array_data);	// (donde empieza, cuántos elementos, un array así:
																				// [x, y, z, x, y, z, x, y, z, x, y, z, ...]
																				// [  P1   ,   P2   ,   P3   ,    P4  , ...]
	/* array_data tendrá que ser array_data = pos + startIndex*3; 
		porque en GPU: [xyz, xyz, xyz...], en lugar de [x,y,z,x,y,z,x,y,z...] (CPU)

		Buscar información acerca de RING BUFFER!!!
	*/
	extern const int maxParticles;
}

bool show_test_window = false;
void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	// Do your GUI code here....
	{	
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate
		//ImGui::SliderInt("Amount of particles", &amountParticles, 1, LilSpheres::maxParticles);
		ImGui::SliderInt("Particles per wave (in 1 second)", &EmissionRate, 1, 100);
		if(ImGui::Button("Reset particles")) {
			for (int i = 0; i < LilSpheres::maxParticles; i++) {
				particles[i].life = MAX_PARTICLE_LIFE+1;
			}
		}
		ImGui::InputFloat3("Particle Source Position", &sourcePos[0]);
		ImGui::InputFloat3("Particle Source Velocity", &sourceAngle[0]);
		ImGui::InputFloat("Particle Inertia", &inertia);
		ImGui::InputFloat("Particle Lifespan", &MAX_PARTICLE_LIFE);
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

	particles = new Particle[LilSpheres::maxParticles];
	particlesGPU = new float[LilSpheres::maxParticles * 3];

	for (int i = 0; i < EmissionRate; i++) {
		particles[i] = GenerateParticle();
	}

	// Box planes setup:
	Plane current;
	
		// Top:
	current.normal = glm::vec3(0, -1, 0);
	current.d = -10;
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

	ParticlesToGPU();
}

void PhysicsUpdate(float dt) {
	// Do your update code here...
	// ...........................
	/*
	for (int i = 1; i < EmissionRate; i++) {
		particles[i] = GenerateParticle();
	}*/

	/// PARTICLES:
	ParticlesUpdate(dt);
	
	// Pasamos del array apto para CPU al array apto para GPU:
	ParticlesToGPU();
}

void PhysicsCleanup() {
	// Do your cleanup code here...
	// ............................
}

Particle MoveAndCollideParticle(Particle particle, float dt) {
	glm::vec3 newPos, newVel;

	newPos = particle.pos + dt * particle.vel;
	newVel = particle.vel + dt * gravity; // Assuming mass is 1.
	particle.pos = newPos;
	particle.vel = newVel;

	// Check collision:
	for (int j = 0; j < TOTAL_BOX_PLANES; j++) {
		if (checkCollision(planes[j], particle)) {
			// Mirror position and velocity:
			glm::vec3 mirroredPos, mirroredVel;
			mirroredPos = newPos - 2 * (glm::dot(planes[j].normal, newPos) + planes[j].d)*planes[j].normal;
			mirroredVel = newVel - 2 * (glm::dot(planes[j].normal, newVel) + planes[j].d)*planes[j].normal;

			particle.pos = mirroredPos;
			particle.vel = mirroredVel;
		}
	}

	// Updating previous position and velocity:
	particle.prevPos = particle.pos;
	particle.prevVel = particle.vel;

	return particle;
}

void ParticlesUpdate(float dt) {
	bool needParticles = true;
	int generatedParticles = 0;

	for (int i = 0; i < LilSpheres::maxParticles; i++) {
		// If the particle is still alive, get Updated:
		if (particles[i].isAlive()) {
			// Add time delta to lifespan of particle:
			particles[i].life += dt;
			particles[i] = MoveAndCollideParticle(particles[i], dt);
		}
		else if (needParticles) {
			particles[i] = GenerateParticle();
			generatedParticles++;
		}
		if (generatedParticles >= EmissionRate) { needParticles = false; }
	}
}

void ParticlesToGPU() {
	int j = 0;
	for (int i = 0; i < LilSpheres::maxParticles; i++) {
		particlesGPU[j] = (particles[i].pos.x);
		j++;
		particlesGPU[j] = (particles[i].pos.y);
		j++;
		particlesGPU[j] = (particles[i].pos.z);
		j++;
	}
	// Actualizamos las partículas:
	LilSpheres::updateParticles(0, LilSpheres::maxParticles, &particlesGPU[0]);
}

bool checkCollision(Plane plane, Particle particle) {
	bool hasCollided = false;
	if ((glm::dot(plane.normal, particle.prevPos) + plane.d)*(glm::dot(plane.normal, particle.pos) + plane.d) <= 0) {
		hasCollided = true;
	}
	return hasCollided;
}

Particle GenerateParticle() {
	glm::vec3 randPos, randVel;
	Particle tmp;
	float randomZ;
	switch (emitterType) {
	case ParticleEmitterType::CASCADE:
		randomZ = 0 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (10 - 0)));
		randomZ -= 5;
		randPos = glm::vec3(sourcePos.x, sourcePos.y, randomZ);
		randVel = sourceAngle;
		break;
	case ParticleEmitterType::FOUNTAIN:
		randomZ = 0 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (10 - 0)));
		randomZ -= 5;
		randPos = glm::vec3(0, 9.99f, randomZ);
		randVel = glm::vec3(0, 1, 0);
		break;
	default:
		randPos = glm::vec3((rand() % 10) - 5, -(rand() % 10), (rand() % 10) - 5);
		randVel = glm::vec3((rand() % 2) - 1, (rand() % 2) - 1, (rand() % 2) - 1);
		break;
	}

	tmp.life = 0;
	tmp.pos = randPos;
	tmp.vel = randVel;
	return tmp;
}


#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include <glm\vec3.hpp>
#include <glm\geometric.hpp>
#include <vector>

#define BOX_TOP 0
#define BOX_BOTTOM 1
#define BOX_LEFT 2
#define BOX_RIGHT 3
#define BOX_FRONT 4
#define BOX_BACK 5
#define TOTAL_BOX_PLANES 6

#define MAX_PARTICLE_LIFE 3
#define MAX_PARTICLE_NUM 10

enum class ParticleEmitterType{ FOUNTAIN, CASCADE };

struct Particle {
	glm::vec3 pos;
	glm::vec3 vel;
	float life;
	glm::vec3 prevPos;
	glm::vec3 prevVel;
};

struct Plane {
	glm::vec3 normal;
	float d;
};

bool checkCollision(Plane plane, Particle particle);
Particle GenerateParticle();

//Particle particles[SHRT_MAX];	// Esto aqu� muy feo.
std::vector<Particle> particles;
Plane planes[TOTAL_BOX_PLANES];
ParticleEmitterType emitterType = ParticleEmitterType::CASCADE;

float Ypos = 0;

namespace LilSpheres {															// Par�metros:
	extern void updateParticles(int startIdx, int count, float* array_data);	// (donde empieza, cu�ntos elementos, un array as�:
																				// [x, y, z, x, y, z, x, y, z, x, y, z, ...]
																				// [  P1   ,   P2   ,   P3   ,    P4  , ...]
	/* array_data tendr� que ser array_data = pos + startIndex*3; 
		porque en GPU: [xyz, xyz, xyz...], en lugar de [x,y,z,x,y,z,x,y,z...] (CPU)

		Buscar informaci�n acerca de RING BUFFER!!!
	*/
}

bool show_test_window = false;
void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	// Do your GUI code here....
	{	
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate
		ImGui::SliderFloat("Y position of cascade", &Ypos, 0, 10);
		if(ImGui::Button("Reset particles")) {
			for (int i = 0; i < particles.size(); i++) {
				particles[i] = GenerateParticle();
			}
		}
		ImGui::Text("Life of particle: %.3f", particles[0].life);
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

	//for (int i = 0; i < SHRT_MAX; i++) { particles[i] = { glm::vec3(0, 0, 0), glm::vec3(0, 0, 0)}; }
	for (int i = 0; i < MAX_PARTICLE_NUM; i++) {
		//glm::vec3 randPos, randVel;
		//randPos = glm::vec3((rand() % 10) - 5, -(rand() % 10), (rand() % 10) - 5);
		//randVel = glm::vec3(rand() % 1, rand() % 1, rand() % 1);
		//particles.push_back({ randPos, randVel });
		particles.push_back(GenerateParticle());
	}

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
}

void PhysicsUpdate(float dt) {
	// Do your update code here...
	// ...........................
	/*
	if (particles.size() < MAX_PARTICLE_NUM) {
		for (int i = 0; i < MAX_PARTICLE_NUM - particles.size(); i++) {
			particles.push_back(GenerateParticle());
		}
	}
	*/
	glm::vec3 gravity = 9.81f * glm::vec3(0, -1, 0);

	/// PARTICLES:
	for (int i = 0; i < particles.size(); i++){
		glm::vec3 newPos, newVel;
		/*
		// Check collision:
		// en lugar de cutre, habr�a que usar la formula de la Colision (la d es la de la ecuaci�n del plano)
		//Cutre:
		if (current.pos.x > 5 || current.pos.x < -5) {
			current.vel.x *= -1;
		}
		if (current.pos.y > 10 || current.pos.y < 0) {
			current.vel.y *= -1;
		}
		if (current.pos.z > 5 || current.pos.z < -5) {
			current.vel.z *= -1;
		}
		*/

		particles[i].life += dt;
		if (particles[i].life > MAX_PARTICLE_LIFE) {
			/*glm::vec3 randPos, randVel;
			randPos = glm::vec3((rand() % 10)-5, -(rand() % 10), (rand() % 10)-5);
			randVel = glm::vec3(rand() % 1, rand() % 1, rand() % 1);
			current = { randPos, randVel };
			current.life = 0;
			particles[i] = current;*/
			particles[i] = GenerateParticle();
			//particles.erase(particles.begin() + i);
			break;
		}

		newPos = particles[i].pos + dt * particles[i].vel;
		newVel = particles[i].vel + dt * gravity; // Assuming mass is 1.
		particles[i].pos = newPos;
		particles[i].vel = newVel;

		// Check collision: BIEN HECHO!
		for (int j = 0; j < TOTAL_BOX_PLANES; j++) {
			if (checkCollision(planes[j], particles[i])) {
				// Mirror position and velocity:
				glm::vec3 mirroredPos, mirroredVel;
				mirroredPos = newPos - 2 * (glm::dot(planes[j].normal, newPos) + planes[j].d)*planes[j].normal;
				mirroredVel = newVel - 2 * (glm::dot(planes[j].normal, newVel) + planes[j].d)*planes[j].normal;

				particles[i].pos = mirroredPos;
				particles[i].vel = mirroredVel;
			}
		}

		// Updating position and velocity:
		//newPos = current.pos + dt * current.vel;
		//newVel = current.vel + dt * gravity; // Assuming mass is 1.
		particles[i].prevPos = particles[i].pos;
		particles[i].prevVel = particles[i].vel;

		//particles[i] = current;
	}
	
	// Pasamos del array apto para CPU al array apto para GPU:
	float particlesGPU[MAX_PARTICLE_NUM * 3];
	int j = 0;
	for (int i = 0; i < particles.size(); i++) {
		particlesGPU[j] = particles[i].pos.x;
		j++;
		particlesGPU[j] = particles[i].pos.y;
		j++;
		particlesGPU[j] = particles[i].pos.z;
		j++;
	}
	// Actualizamos las part�culas:
	LilSpheres::updateParticles(0, MAX_PARTICLE_NUM, particlesGPU);
}

void PhysicsCleanup() {
	// Do your cleanup code here...
	// ............................
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
		randPos = glm::vec3(0, -Ypos, randomZ);
		randVel = glm::vec3(0, -1, 0);
		break;
	case ParticleEmitterType::FOUNTAIN:
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
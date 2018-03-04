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

#define MAX_PARTICLE_LIFE 2
#define MAX_PARTICLE_NUM 100

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

//Particle particles[SHRT_MAX];	// Esto aquí muy feo.
std::vector<Particle> particles;
Plane planes[TOTAL_BOX_PLANES];

namespace LilSpheres {															// Parámetros:
	extern void updateParticles(int startIdx, int count, float* array_data);	// (donde empieza, cuántos elementos, un array así:
																				// [x, y, z, x, y, z, x, y, z, x, y, z, ...]
																				// [  P1   ,   P2   ,   P3   ,    P4  , ...]
	/* array_data tendrá que ser array_data = pos + startIndex*3; 
		porque en GPU: [xyz, xyz, xyz...], en lugar de [x,y,z,x,y,z,x,y,z...] (CPU)

		Buscar información acerca de RING BUFFER!!!
	*/
}

bool show_test_window = false;
void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	// Do your GUI code here....
	{	
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate
		
	}
	// .........................
	
	ImGui::End();

	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if(show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}

Particle GenerateParticle() {
	glm::vec3 randPos, randVel;
	randPos = glm::vec3((rand() % 10) - 5, -(rand() % 10), (rand() % 10) - 5);
	randVel = glm::vec3(rand() % 1, rand() % 1, rand() % 1);
	Particle tmp;
	tmp.life = 0;
	tmp.pos = randPos;
	tmp.vel = randVel;
	return tmp;
}

void PhysicsInit() {
	// Do your initialization code here...
	// ...................................

	//for (int i = 0; i < SHRT_MAX; i++) { particles[i] = { glm::vec3(0, 0, 0), glm::vec3(0, 0, 0)}; }
	for (int i = 0; i < SHRT_MAX; i++) {
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

	if (particles.size() < SHRT_MAX) {
		for (int i = 0; i < SHRT_MAX - particles.size(); i++) {
			particles.push_back(GenerateParticle());
		}
	}
	
	glm::vec3 gravity = 9.81f * glm::vec3(0, -1, 0);

	/// PARTICLES:
	for (int i = 0; i < particles.size(); i++){
		glm::vec3 newPos, newVel;
		Particle current = particles[i];
		/*
		// Check collision:
		// en lugar de cutre, habría que usar la formula de la Colision (la d es la de la ecuación del plano)
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

		current.life += dt;
		if (current.life > MAX_PARTICLE_LIFE) {
			/*glm::vec3 randPos, randVel;
			randPos = glm::vec3((rand() % 10)-5, -(rand() % 10), (rand() % 10)-5);
			randVel = glm::vec3(rand() % 1, rand() % 1, rand() % 1);
			current = { randPos, randVel };
			current.life = 0;
			particles[i] = current;*/
			particles[i] = GenerateParticle();
			break;
		}

		newPos = current.pos + dt * current.vel;
		newVel = current.vel + dt * gravity; // Assuming mass is 1.
		current.pos = newPos;
		current.vel = newVel;

		// Check collision: BIEN HECHO!
		for (int i = 0; i < TOTAL_BOX_PLANES; i++) {
			if (checkCollision(planes[i], current)) {
				// Mirror position and velocity:
				glm::vec3 mirroredPos, mirroredVel;
				mirroredPos = newPos - 2 * (glm::dot(planes[i].normal, newPos) + planes[i].d)*planes[i].normal;
				mirroredVel = newVel - 2 * (glm::dot(planes[i].normal, newVel) + planes[i].d)*planes[i].normal;

				current.pos = mirroredPos;
				current.vel = mirroredVel;
			}
		}

		// Updating position and velocity:
		//newPos = current.pos + dt * current.vel;
		//newVel = current.vel + dt * gravity; // Assuming mass is 1.
		current.prevPos = current.pos;
		current.prevVel = current.vel;


	

		particles[i] = current;
	}
	
	// Pasamos del array apto para CPU al array apto para GPU:
	float particlesGPU[SHRT_MAX * 3];
	int j = 0;
	for (int i = 0; i < SHRT_MAX; i++) {
		particlesGPU[j] = particles[i].pos.x;
		j++;
		particlesGPU[j] = particles[i].pos.y;
		j++;
		particlesGPU[j] = particles[i].pos.z;
		j++;
	}
	// Actualizamos las partículas:
	LilSpheres::updateParticles(0, SHRT_MAX, particlesGPU);
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
#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include <glm\vec3.hpp>

struct Particle {
	glm::vec3 pos;
	glm::vec3 vel;
};

Particle particles[SHRT_MAX];	// Esto aquí muy feo.

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

void PhysicsInit() {
	// Do your initialization code here...
	// ...................................

	//for (int i = 0; i < SHRT_MAX; i++) { particles[i] = { glm::vec3(0, 0, 0), glm::vec3(0, 0, 0)}; }
	for (int i = 0; i < SHRT_MAX; i++) {
		glm::vec3 randPos, randVel;
		randPos = glm::vec3(rand()%5, rand()%5, rand()%5);
		randVel = glm::vec3(rand() % 10, rand() % 10, rand() % 10);
		particles[i] = { randPos, randVel };
	}
}

void PhysicsUpdate(float dt) {
	// Do your update code here...
	// ...........................
	
	glm::vec3 gravity = 9.81f * glm::vec3(0, -1, 0);
	/// PARTICLES:
	for (int i = 0; i < SHRT_MAX; i++){
		glm::vec3 newPos, newVel;
		Particle current = particles[i];

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

		newPos = current.pos + dt * current.vel;
		newVel = current.vel + dt * gravity; // Assuming mass is 1.
		current.pos = newPos;
		current.vel = newVel;

	

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
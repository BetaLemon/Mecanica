#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include <glm\glm.hpp>
#include <glm\gtc\quaternion.hpp>
#include <glm\gtx\quaternion.hpp>
#include <glm\geometric.hpp>
#include <vector>

#pragma region RenderDependencies
namespace Cube {
	extern void setupCube();
	extern void cleanupCube();
	extern void updateCube(const glm::mat4& transform);
	extern void drawCube();
}
#pragma endregion

struct Rigidbody {
	float mass;
	float size;
	glm::vec3 position;	// At Center of Mass = "CoM".
	glm::vec3 linearVel;
	glm::quat orientation;
	glm::vec3 linearMomentum;
	glm::vec3 angularMomentum;
	glm::mat3 Ibody;
	glm::vec3 forces;
	glm::vec3 torque;
};

struct Plane {
	glm::vec3 normal;
	float d;
};

Rigidbody rb1;
bool process = false;
bool showDebugInfo = true;

#pragma region FunctionDeclaration
void InitRB(Rigidbody &rb);
Rigidbody AddGravityToRB(Rigidbody rb, float gravity);
Rigidbody ComputeEulerRB(Rigidbody rb, float dt);
glm::mat4 CubeTransformMatrix(Rigidbody rb);
#pragma endregion


bool show_test_window = false;
void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	// Do your GUI code here....
	{	
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate
		
	}
	// .........................
	if(ImGui::Button("Process")) {
		process = true;
	}
	if (ImGui::Button("Show/Hide Debug Info")) {
		showDebugInfo = !showDebugInfo;
	}
	ImGui::Separator();
	if (showDebugInfo) {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Debug Info:");
		ImGui::DragFloat3("Position", (float*)&rb1.position);
		ImGui::DragFloat3("Linear Velocity", (float*)&rb1.linearVel);
	}

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

	InitRB(rb1);

	Cube::setupCube();
}

void PhysicsUpdate(float dt) {
	if (process) {
		// Do your update code here...
		// ...........................

		rb1 = AddGravityToRB(rb1, 9.81f);
		rb1 = ComputeEulerRB(rb1, dt);

		//glm::toMat4()
		Cube::updateCube(CubeTransformMatrix(rb1));
		process = false;
	}
	Cube::drawCube();
}

void PhysicsCleanup() {
	// Do your cleanup code here...
	// ............................

	Cube::cleanupCube();
}

void InitRB(Rigidbody &rb) {
	Rigidbody _rb;

	_rb.angularMomentum = glm::vec3(0);
	_rb.forces = glm::vec3(0);
	_rb.linearMomentum = glm::vec3(0);
	_rb.linearVel = glm::vec3(0);
	_rb.size = 1;
	_rb.mass = 1;
	_rb.Ibody[0] = glm::vec3(1 / 12 * _rb.mass * (pow(_rb.size,2) + pow(_rb.size,2)), 0, 0);	// Columna 0
	_rb.Ibody[1] = glm::vec3(0, 1 / 12 * _rb.mass * (pow(_rb.size, 2) + pow(_rb.size, 2)), 0);	// Columna 1
	_rb.Ibody[2] = glm::vec3(0, 0, 1 / 12 * _rb.mass * (pow(_rb.size, 2) + pow(_rb.size, 2)));	// Columna 2
	_rb.orientation = glm::quat();
	_rb.position = glm::vec3(0, 5, 0);
	_rb.torque = glm::vec3(0);

	rb = _rb;
}

Rigidbody AddGravityToRB(Rigidbody rb, float gravity) {
	rb.forces.y -= gravity;
	return rb;
}

Rigidbody ComputeEulerRB(Rigidbody rb, float dt) {
	// Position:
	rb.position = rb.position + dt * rb.linearVel;
	// Linear Velocity:
	rb.linearVel = rb.linearMomentum / rb.mass;
	// Linear Momentum:
	rb.linearMomentum = rb.linearMomentum + dt * rb.forces;
	return rb;
}

glm::mat3 QuatToMat(glm::quat qt) {
	glm::mat3 mat;
	glm::vec3 col;

	col = { 0.5f - pow(qt.y,2) - pow(qt.z, 2),
			qt.x * qt.y - qt.w * qt.z,
			qt.x * qt.z + qt.w * qt.y
	};
	mat[0] = col;// Columna 0

	col = { qt.x * qt.y + qt.w * qt.z,
			0.5f - pow(qt.x, 2) - pow(qt.z, 2),
			qt.y * qt.z - qt.w * qt.x
	};
	mat[1] = col;// Columna 1

	col = { qt.x * qt.z - qt.w * qt.y,
			qt.y * qt.z + qt.w * qt.x,
			0.5f - pow(qt.x, 2) - pow(qt.y, 2)
	};
	mat[2] = col;// Columna 1

	mat *= 2;
	return mat;
}

glm::mat4 CubeTransformMatrix(Rigidbody rb) {
	glm::mat4 mat;

	glm::mat3 tmp;
	tmp = QuatToMat(rb.orientation);

	mat[0] = glm::vec4(tmp[0].x, tmp[0].y, tmp[0].z, 0);	// Columna 0
	mat[1] = glm::vec4(tmp[1].x, tmp[1].y, tmp[1].z, 0);	// Columna 1
	mat[2] = glm::vec4(tmp[2].x, tmp[2].y, tmp[2].z, 0);	// Columna 2
	mat[3] = glm::vec4(rb.position.x, rb.position.y, rb.position.z, 1);	// Columna 3

	return mat;
}
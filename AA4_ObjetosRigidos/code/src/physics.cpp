#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include <glm\glm.hpp>
#include <glm\gtc\quaternion.hpp>
#include <glm\gtx\quaternion.hpp>
#include <glm\geometric.hpp>
#include <vector>

#pragma region ConstantDefines
// Plane Array ID (index):
#define BOX_TOP 0
#define BOX_BOTTOM 1
#define BOX_LEFT 2
#define BOX_RIGHT 3
#define BOX_FRONT 4
#define BOX_BACK 5
// Amount of planes for Box:
#define TOTAL_BOX_PLANES 6
#pragma endregion

#pragma region RenderDependencies
namespace Cube {
	extern void setupCube();
	extern void cleanupCube();
	extern void updateCube(const glm::mat4& transform);
	extern void drawCube();
}
#pragma endregion

struct Particle {
	glm::vec3 pos, prevPos;
	Particle() { pos = glm::vec3(0); prevPos = glm::vec3(0); };
	Particle(glm::vec3 _pos) { pos = _pos; prevPos = _pos; };
};

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
	Particle * boundingBox;
};

struct Plane {
	glm::vec3 normal;
	float d;
};

#pragma region GlobalVars
Rigidbody rb1;
Plane planes[TOTAL_BOX_PLANES];
bool process = false;
bool showDebugInfo = true;
#pragma endregion

#pragma region FunctionDeclaration
void InitRB(Rigidbody &rb);
Rigidbody AddGravityToRB(Rigidbody rb, float gravity);
Rigidbody ComputeEulerRB(Rigidbody rb, float dt);
glm::mat4 CubeTransformMatrix(Rigidbody rb);
bool checkPlaneCollision(Plane plane, Particle particle);
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
	Cube::updateCube(CubeTransformMatrix(rb1));
}

void PhysicsUpdate(float dt) {
	if (process) {
		// Do your update code here...
		// ...........................

		rb1 = AddGravityToRB(rb1, 9.81f);
		rb1 = ComputeEulerRB(rb1, dt);

		if (checkPlaneCollision(planes[BOX_BOTTOM], rb1.boundingBox[0])) {
			printf("Putes, Harry, putes.");
		}

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

	//   4---------7
	//  /|        /|
	// / |       / |
	//5---------6  |
	//|  0------|--3
	//| /       | /
	//|/        |/
	//1---------2

	float len = _rb.size / 2;
	_rb.boundingBox = new Particle[8];
	_rb.boundingBox[0] = Particle(glm::vec3(-len, -len, -len) + _rb.position);
	_rb.boundingBox[1] = Particle(glm::vec3(-len, -len, len) + _rb.position);
	_rb.boundingBox[2] = Particle(glm::vec3(len, -len, len) + _rb.position);
	_rb.boundingBox[3] = Particle(glm::vec3(len, -len, -len) + _rb.position);
	_rb.boundingBox[4] = Particle(glm::vec3(-len, len, -len) + _rb.position);
	_rb.boundingBox[5] = Particle(glm::vec3(-len, len, len) + _rb.position);
	_rb.boundingBox[6] = Particle(glm::vec3(len, len, len) + _rb.position);
	_rb.boundingBox[7] = Particle(glm::vec3(len, len, -len) + _rb.position);

	rb = _rb;
}

void InitPlanes() {
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

// Checks for collision with a plane:
bool checkPlaneCollision(Plane plane, Particle particle) {
	bool hasCollided = false;
	if ((glm::dot(plane.normal, particle.prevPos) + plane.d)*(glm::dot(plane.normal, particle.pos) + plane.d) <= 0) {
		hasCollided = true;
	}
	return hasCollided;
}
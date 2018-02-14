#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>

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
}

void PhysicsUpdate(float dt) {
	// Do your update code here...
	// ...........................
}

void PhysicsCleanup() {
	// Do your cleanup code here...
	// ............................
}
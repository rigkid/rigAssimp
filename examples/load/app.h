#pragma once
#include "core/U_core.h"
#include "OrbitNav.h"
#include <entt/entt.hpp>
#include <string>
#include <vector>

class LoadApp : public rigkit::IApp {
  public:
	LoadApp() {
		window().width = 800;
		window().height = 600;
		window().title = "rigAssimp - load";
	}
	void parseCommandLineArgs(const rigkit::CommandLineArgs& args) override;
	void setup() override;
	void update(float dt) override;
	void draw() override {}

	void addWheel(float y);

  private:
	void consumeDrops();
	void openMeshes(const std::vector<std::string>& paths);
	void frameCamera();
	void navigateCamera();
	void fitPresent(const glm::vec3& center, float radius);

	std::vector<std::string> m_pendingDrops;
	std::vector<entt::entity> m_meshes;
	entt::entity m_camera = entt::null;
	entt::entity m_light = entt::null;
	rig::OrbitNavState m_orbitNav;
	float m_wheel = 0.f;
	bool m_frameHeld = false;
	float m_time = 0.f;
	float m_lightRadius = 3.2f;
	float m_lightHeight = 3.0f;
};

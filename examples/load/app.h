#pragma once
#include "core/U_core.h"
#include <entt/entt.hpp>

class LoadApp : public rigkit::IApp {
  public:
	LoadApp() {
		window().width = 800;
		window().height = 600;
		window().title = "rigAssimp — load";
	}
	void setup() override;
	void update(float dt) override;
	void draw() override {}

  private:
	entt::entity m_stl = entt::null;
	entt::entity m_obj = entt::null;
	entt::entity m_light = entt::null;
	float m_time = 0.f;
};

#include "app.h"

#include <cmath>
#include <filesystem>

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "packs/rigAssimp/src/AssimpIO.h"
#include "packs/rigAssimp/src/rigAssimp.h"
#include "packs/rigComponent/src/CLight.h"
#include "packs/rigComponent/src/CTransform.h"
#include "packs/rigComponent/src/rig.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigRender3D/src/rigRender3D.h"
#include "packs/rigSystems/src/rigSystems.h"

namespace {

void logIo(const char* label, const std::filesystem::path& path, const rig::assimp::IoResult& io) {
	if (!io.ok) {
		spdlog::error("[load] {} {}: {}", label, path.string(), io.error);
		return;
	}
	if (!io.warning.empty()) {
		spdlog::warn("[load] {} {}: {}", label, path.string(), io.warning);
	} else {
		spdlog::info("[load] {} {}", label, path.string());
	}
}

} // namespace

void LoadApp::setup() {
	spdlog::info("load — Assimp prism.stl + cube.obj → CMesh (rigRender3D)");
	m_engine->setClearColor(0.09f, 0.10f, 0.13f, 1.0f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack(std::make_shared<rigkit::rigComponent>());
	packs->registerPack(std::make_shared<rigkit::rigSystems>());
	packs->registerPack(std::make_shared<rigkit::rigRender3D>());
	packs->registerPack(std::make_shared<rigkit::rigAssimp>());
	packs->initAll();
	packs->setupAll();

	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}

	auto cam = rig::makeCamera(*ecs, {0.f, 1.2f, 3.6f}, true, "load-camera");
	auto& camXf = ecs->getComponent<rigkit::ecs::CTransform>(cam);
	camXf.euler = {-0.28f, 0.0f, 0.0f};

	// Key light — point reads clearer than a dim directional at the origin.
	m_light =
		rig::makeLight(*ecs, {2.4f, 3.2f, 2.0f}, rigkit::ecs::CLight::Type::Point, "load-key");
	auto& lightData = ecs->getComponent<rigkit::ecs::CLight>(m_light);
	lightData.intensity = 1.15f;
	lightData.ambient = 0.28f;
	lightData.banded = true;
	lightData.bands = 4;
	lightData.colorR = 1.0f;
	lightData.colorG = 0.96f;
	lightData.colorB = 0.90f;

	const auto data = std::filesystem::path(AppPaths::getDataDir());
	const auto stlPath = data / "prism.stl";
	const auto objPath = data / "cube.obj";

	rig::assimp::IoResult stlIo;
	m_stl = rig::assimp::makeMeshFromFile(*ecs, stlPath.string(), rig::fill(0.35f, 0.75f, 0.95f),
										  "assimp-stl", &stlIo);
	logIo("STL", stlPath, stlIo);
	if (m_stl != entt::null) {
		ecs->getComponent<rigkit::ecs::CTransform>(m_stl).position = {-1.15f, 0.f, 0.f};
	}

	rig::assimp::IoResult objIo;
	m_obj = rig::assimp::makeMeshFromFile(*ecs, objPath.string(), rig::fill(0.95f, 0.55f, 0.28f),
										  "assimp-obj", &objIo);
	logIo("OBJ", objPath, objIo);
	if (m_obj != entt::null) {
		ecs->getComponent<rigkit::ecs::CTransform>(m_obj).position = {1.15f, 0.f, 0.f};
	}
}

void LoadApp::update(float dt) {
	m_time += dt;
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	if (m_light != entt::null && ecs->hasComponent<rigkit::ecs::CTransform>(m_light)) {
		auto& lx = ecs->getComponent<rigkit::ecs::CTransform>(m_light);
		lx.position = {2.4f * std::cos(m_time * 0.35f), 3.0f,
					   2.0f * std::sin(m_time * 0.35f)};
	}
	if (m_stl != entt::null && ecs->hasComponent<rigkit::ecs::CTransform>(m_stl)) {
		auto& t = ecs->getComponent<rigkit::ecs::CTransform>(m_stl);
		t.euler.y = m_time * 0.85f;
		t.euler.x = 0.25f * std::sin(m_time * 0.6f);
	}
	if (m_obj != entt::null && ecs->hasComponent<rigkit::ecs::CTransform>(m_obj)) {
		auto& t = ecs->getComponent<rigkit::ecs::CTransform>(m_obj);
		t.euler.y = -m_time * 0.7f;
	}
}

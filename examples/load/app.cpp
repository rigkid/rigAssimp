#include "app.h"

#include <cmath>
#include <filesystem>
#include <spdlog/spdlog.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "core/util/AppPaths.h"
#include "packs/rigAssimp/src/AssimpIO.h"
#include "packs/rigAssimp/src/rigAssimp.h"
#include "packs/rigComponent/src/CCamera.h"
#include "packs/rigComponent/src/CLight.h"
#include "packs/rigComponent/src/CMesh.h"
#include "packs/rigComponent/src/COrbitDrive.h"
#include "packs/rigComponent/src/CTransform.h"
#include "packs/rigComponent/src/rig.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigRender3D/src/rigRender3D.h"
#include "packs/rigSystems/src/OrbitNav.h"
#include "packs/rigSystems/src/rigSystems.h"

namespace {

constexpr glm::vec3 kFills[] = {
	{0.35f, 0.75f, 0.95f},
	{0.95f, 0.55f, 0.28f},
	{0.55f, 0.85f, 0.45f},
	{0.90f, 0.70f, 0.35f},
};

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

bool isStepPath(const std::filesystem::path& path) {
	const auto ext = path.extension().string();
	return ext == ".step" || ext == ".stp" || ext == ".STEP" || ext == ".STP";
}

bool meshBounds(const rigkit::ecs::CMesh& mesh, glm::vec3& outMin, glm::vec3& outMax) {
	if (mesh.positions.empty()) {
		return false;
	}
	outMin = mesh.positions.front();
	outMax = mesh.positions.front();
	for (const auto& p : mesh.positions) {
		outMin = glm::min(outMin, p);
		outMax = glm::max(outMax, p);
	}
	return true;
}

void onScroll(GLFWwindow* window, double /*x*/, double y) {
	auto* app = static_cast<LoadApp*>(glfwGetWindowUserPointer(window));
	if (app) {
		app->addWheel(static_cast<float>(y));
	}
}

} // namespace

void LoadApp::addWheel(float y) {
	m_wheel += y;
}

void LoadApp::parseCommandLineArgs(const rigkit::CommandLineArgs& args) {
	rigkit::IApp::parseCommandLineArgs(args);
	for (const auto& path : args.getPositionalArgs()) {
		if (!path.empty()) {
			m_pendingDrops.push_back(path);
		}
	}
}

void LoadApp::setup() {
	spdlog::info("load - Assimp prism.stl + cube.obj to CMesh (rigRender3D)");
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

	m_camera = rig::makeOrbitCamera(*ecs, {0.f, 0.5f, 0.f}, 4.2f, 0.36f, true, "load-camera");

	// Key light - point reads clearer than a dim directional at the origin.
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
	auto stl = rig::assimp::makeMeshFromFile(*ecs, stlPath.string(), rig::fill(0.35f, 0.75f, 0.95f),
											 "assimp-stl", &stlIo);
	logIo("STL", stlPath, stlIo);
	if (stl != entt::null) {
		ecs->getComponent<rigkit::ecs::CTransform>(stl).position = {-1.15f, 0.f, 0.f};
		m_meshes.push_back(stl);
	}

	rig::assimp::IoResult objIo;
	auto obj = rig::assimp::makeMeshFromFile(*ecs, objPath.string(), rig::fill(0.95f, 0.55f, 0.28f),
											 "assimp-obj", &objIo);
	logIo("OBJ", objPath, objIo);
	if (obj != entt::null) {
		ecs->getComponent<rigkit::ecs::CTransform>(obj).position = {1.15f, 0.f, 0.f};
		m_meshes.push_back(obj);
	}
	frameCamera();

	if (auto* win = m_engine->getWindow()) {
		glfwSetWindowUserPointer(win, this);
		glfwSetScrollCallback(win, onScroll);
		const auto exts = rig::assimp::supportedExtensions();
		std::string list;
		for (const auto& ext : exts) {
			if (!list.empty()) {
				list += ", ";
			}
			list += ext;
		}
		spdlog::info("load - drop a mesh on the window ({})", list);
		spdlog::info("load - orbit: MMB or Alt+LMB; pan: Shift+MMB or Alt+MMB; dolly: Alt+RMB / wheel; F frames");
	}
	consumeDrops();
}

void LoadApp::consumeDrops() {
	if (m_engine) {
		for (auto& path : m_engine->takeDroppedPaths()) {
			m_pendingDrops.push_back(std::move(path));
		}
	}
	if (m_pendingDrops.empty()) {
		return;
	}
	const auto paths = std::move(m_pendingDrops);
	m_pendingDrops.clear();
	for (const auto& path : paths) {
		spdlog::info("[load] drop {}", path);
	}
	openMeshes(paths);
}

void LoadApp::openMeshes(const std::vector<std::string>& paths) {
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}

	std::vector<entt::entity> loaded;
	float cursorX = 0.f;
	const bool spread = paths.size() > 1;
	for (size_t i = 0; i < paths.size(); ++i) {
		const std::filesystem::path path = paths[i];
		std::error_code ec;
		if (std::filesystem::is_directory(path, ec)) {
			spdlog::warn("[load] skip directory {}", path.string());
			continue;
		}
		const auto fill = kFills[i % (sizeof(kFills) / sizeof(kFills[0]))];
		rig::assimp::IoResult io;
		auto e = rig::assimp::makeMeshFromFile(*ecs, path.string(),
											  rig::fill(fill.x, fill.y, fill.z),
											  path.stem().string(), &io);
		logIo("open", path, io);
		if (e == entt::null) {
			if (isStepPath(path)) {
				spdlog::warn("[load] Assimp only reads IFC-flavoured STEP; export STL / OBJ / glTF");
			}
			continue;
		}
		auto& mesh = ecs->getComponent<rigkit::ecs::CMesh>(e);
		auto& xf = ecs->getComponent<rigkit::ecs::CTransform>(e);
		glm::vec3 mn{}, mx{};
		if (meshBounds(mesh, mn, mx)) {
			const glm::vec3 center = (mn + mx) * 0.5f;
			const float width = mx.x - mn.x;
			if (spread) {
				xf.position = {cursorX + width * 0.5f - center.x, -mn.y, -center.z};
				cursorX += width + 0.35f;
			} else {
				xf.position = {-center.x, -mn.y, -center.z};
			}
		}
		loaded.push_back(e);
	}
	if (loaded.empty()) {
		return;
	}

	for (auto e : m_meshes) {
		if (e != entt::null) {
			ecs->destroyEntity(e);
		}
	}
	m_meshes = std::move(loaded);
	m_time = 0.f;
	frameCamera();
}

void LoadApp::fitPresent(const glm::vec3& center, float radius) {
	auto* ecs = m_engine->getECSManager();
	if (!ecs || m_camera == entt::null || !ecs->hasComponent<rigkit::ecs::CCamera>(m_camera)) {
		return;
	}
	auto& cam = ecs->getComponent<rigkit::ecs::CCamera>(m_camera);
	cam.farClip = std::max(1000.f, radius * 8.f);
	cam.nearClip = std::max(0.01f, cam.farClip * 0.0001f);
	m_lightRadius = std::max(radius * 0.75f, 1.f);
	m_lightHeight = center.y + std::max(radius * 0.35f, 1.f);
}

void LoadApp::frameCamera() {
	auto* ecs = m_engine->getECSManager();
	if (!ecs || m_camera == entt::null) {
		return;
	}

	glm::vec3 worldMin(0.f);
	glm::vec3 worldMax(0.f);
	bool any = false;
	for (auto e : m_meshes) {
		if (e == entt::null || !ecs->hasComponent<rigkit::ecs::CMesh>(e) ||
			!ecs->hasComponent<rigkit::ecs::CTransform>(e)) {
			continue;
		}
		const auto& mesh = ecs->getComponent<rigkit::ecs::CMesh>(e);
		const auto& xf = ecs->getComponent<rigkit::ecs::CTransform>(e);
		glm::vec3 mn{}, mx{};
		if (!meshBounds(mesh, mn, mx)) {
			continue;
		}
		const glm::vec3 wmin = xf.position + mn;
		const glm::vec3 wmax = xf.position + mx;
		if (!any) {
			worldMin = wmin;
			worldMax = wmax;
			any = true;
		} else {
			worldMin = glm::min(worldMin, wmin);
			worldMax = glm::max(worldMax, wmax);
		}
	}
	if (!any) {
		return;
	}

	rig::orbitFrame(*ecs, m_camera, worldMin, worldMax);
	const glm::vec3 center = (worldMin + worldMax) * 0.5f;
	float radius = 0.5f * glm::length(worldMax - worldMin);
	if (ecs->hasComponent<rigkit::ecs::COrbitDrive>(m_camera)) {
		radius = ecs->getComponent<rigkit::ecs::COrbitDrive>(m_camera).radius;
	}
	fitPresent(center, radius);
}

void LoadApp::navigateCamera() {
	auto* ecs = m_engine->getECSManager();
	auto* win = m_engine->getWindow();
	if (!ecs || !win || m_camera == entt::null) {
		m_wheel = 0.f;
		return;
	}

	double mx = 0.0;
	double my = 0.0;
	glfwGetCursorPos(win, &mx, &my);
	int viewW = 0;
	int viewH = 0;
	glfwGetWindowSize(win, &viewW, &viewH);

	const bool lmb = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
	const bool rmb = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
	const bool mmb = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
	const bool alt = glfwGetKey(win, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
					 glfwGetKey(win, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
	const bool shift = glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
					   glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
	const bool frameKey = glfwGetKey(win, GLFW_KEY_F) == GLFW_PRESS;

	rig::OrbitNavFrame frame;
	frame.mouseX = static_cast<float>(mx);
	frame.mouseY = static_cast<float>(my);
	frame.wheel = m_wheel;
	m_wheel = 0.f;
	frame.orbit = mmb || (alt && lmb);
	frame.pan = (mmb && shift) || (alt && mmb);
	frame.dolly = alt && rmb;
	frame.viewW = static_cast<float>(viewW);
	frame.viewH = static_cast<float>(viewH);
	if (viewW > 1 && viewH > 1) {
		const float ndcX = (2.f * frame.mouseX / frame.viewW) - 1.f;
		const float ndcY = 1.f - (2.f * frame.mouseY / frame.viewH);
		frame.pivot = rig::orbitPickPivot(*ecs, m_camera, ndcX, ndcY, frame.viewW / frame.viewH);
		frame.havePivot = true;
	}
	rig::orbitNavigate(*ecs, m_camera, frame, m_orbitNav);

	if (frameKey && !m_frameHeld) {
		frameCamera();
	}
	m_frameHeld = frameKey;
}

void LoadApp::update(float dt) {
	consumeDrops();
	navigateCamera();
	m_time += dt;
	auto* ecs = m_engine->getECSManager();
	if (!ecs) {
		return;
	}
	if (m_light != entt::null && ecs->hasComponent<rigkit::ecs::CTransform>(m_light)) {
		auto& lx = ecs->getComponent<rigkit::ecs::CTransform>(m_light);
		lx.position = {m_lightRadius * std::cos(m_time * 0.35f), m_lightHeight,
					   m_lightRadius * std::sin(m_time * 0.35f)};
	}
}

#include "rigAssimp.h"
#include "core/RigKitEngine.h"
#include "core/pack/PackRegistry.h"
#include <spdlog/spdlog.h>

namespace rigkit {

rigAssimp::rigAssimp() : IPack("rigAssimp") {
	

}

bool rigAssimp::init() {
	spdlog::info("[rigAssimp] init");
	return true;
}

void rigAssimp::setup() {
	spdlog::info("[rigAssimp] setup (IO helpers; no systems; nothing depends on this pack)");
}

} // namespace rigkit

namespace {
struct rigAssimpRegistrar {
	rigAssimpRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigAssimp", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigAssimp>());
		});
	}
};
static rigAssimpRegistrar rigAssimp_auto_reg;
} // namespace

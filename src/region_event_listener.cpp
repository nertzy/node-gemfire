#include "region_event_listener.hpp"
#include "region_event_registry.hpp"

using namespace gemfire;

namespace node_gemfire {

void RegionEventListener::afterCreate(const EntryEvent & event) {
  RegionEventRegistry::getInstance()->emit("create", event);
}

RegionEventRegistry RegionEventRegistry::instance = RegionEventRegistry();

}  // namespace node_gemfire

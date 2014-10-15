#include "region_event_listener.hpp"
#include "region_event_registry.hpp"

using namespace gemfire;

namespace node_gemfire {

void RegionEventListener::afterCreate(const EntryEvent & event) {
  RegionEventRegistry::getInstance()->emit("create", event);
}

void RegionEventListener::afterUpdate(const EntryEvent & event) {
  RegionEventRegistry::getInstance()->emit("update", event);
}

RegionEventRegistry RegionEventRegistry::instance = RegionEventRegistry();

}  // namespace node_gemfire

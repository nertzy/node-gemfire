#include "region_event_registry.hpp"

#include <string>
#include <cassert>
#include <set>
#include <vector>
#include "conversions.hpp"
#include "events.hpp"

using namespace gemfire;

namespace node_gemfire {

void RegionEventRegistry::add(node_gemfire::Region * region) {
  assert(region->regionPtr != NULLPTR);

  AttributesMutatorPtr attrMutatorPtr(region->regionPtr->getAttributesMutator());
  attrMutatorPtr->setCacheListener(listener);

  regionSet.insert(region);
}

void RegionEventRegistry::remove(node_gemfire::Region * region) {
  regionSet.erase(region);
}

void RegionEventRegistry::emit(const std::string & eventName, const EntryEvent & event) {
  eventStream->add(event);
}

RegionEventRegistry * RegionEventRegistry::getInstance() {
  return &instance;
}

void RegionEventRegistry::emitCallback(uv_async_t * async) {
  RegionEventRegistry * regionEventRegistry = reinterpret_cast<RegionEventRegistry *>(async->data);
  regionEventRegistry->publishEvents();
}

void RegionEventRegistry::emitCallback(uv_async_t * async, int status) {
  emitCallback(async);
}

void RegionEventRegistry::publishEvents() {
  NanScope();

  std::vector<gemfire::EntryEventPtr> eventVector(eventStream->nextEvents());

  for (std::vector<EntryEventPtr>::iterator iterator(eventVector.begin());
       iterator != eventVector.end();
       ++iterator) {
    EntryEventPtr eventPtr(*iterator);

    Local<Object> eventPayload(NanNew<Object>());
    eventPayload->Set(NanNew("key"), v8Value(eventPtr->getKey()));
    eventPayload->Set(NanNew("value"), v8Value(eventPtr->getNewValue()));

    for (std::set<Region *>::iterator iterator(regionSet.begin());
         iterator != regionSet.end();
         ++iterator) {
      Region * region(*iterator);
      Local<Object> regionObject(NanObjectWrapHandle(region));
      if (region->regionPtr == eventPtr->getRegion()) {
        emitEvent(regionObject, "create", eventPayload);
      }
    }
  }
}

}  // namespace node_gemfire

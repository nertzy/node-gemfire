#include "region_event_registry.hpp"

#include <string>
#include <cassert>
#include <set>
#include <vector>
#include "events.hpp"

using namespace v8;
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
  eventStream->add(new EventStream::Event(eventName, event));
}

RegionEventRegistry * RegionEventRegistry::getInstance() {
  return &instance;
}

void RegionEventRegistry::emitCallback(uv_async_t * async, int status) {
  RegionEventRegistry * regionEventRegistry = reinterpret_cast<RegionEventRegistry *>(async->data);
  regionEventRegistry->publishEvents();
}

void RegionEventRegistry::publishEvents() {
  NanScope();

  std::vector<EventStream::Event *> eventVector(eventStream->nextEvents());

  for (std::vector<EventStream::Event *>::iterator iterator(eventVector.begin());
       iterator != eventVector.end();
       ++iterator) {
    EventStream::Event * event(*iterator);
    Local<Object> eventPayload(event->v8Object());

    for (std::set<Region *>::iterator iterator(regionSet.begin());
         iterator != regionSet.end();
         ++iterator) {
      Region * region(*iterator);
      Local<Object> regionObject(NanObjectWrapHandle(region));
      if (region->regionPtr == event->getRegion()) {
        emitEvent(regionObject, event->getName().c_str(), eventPayload);
      }
    }

    delete event;
  }
}

}  // namespace node_gemfire

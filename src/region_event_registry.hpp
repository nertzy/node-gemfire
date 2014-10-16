#ifndef __REGION_EVENT_REGISTRY_HPP__
#define __REGION_EVENT_REGISTRY_HPP__

#include <gfcpp/Region.hpp>
#include <gfcpp/EntryEvent.hpp>
#include <string>
#include <set>
#include "region_event_listener.hpp"
#include "event_stream.hpp"

namespace node_gemfire {

class Region;

class RegionEventRegistry {
 public:
  RegionEventRegistry() :
    listener(new RegionEventListener),
    eventStream(new EventStream(this, (uv_async_cb) emitCallback)) {}

  static void emitCallback(uv_async_t * async, int status);

  void add(node_gemfire::Region * region);
  void remove(node_gemfire::Region * region);
  void emit(const std::string & eventName, const gemfire::EntryEvent & event);
  static RegionEventRegistry * getInstance();

 private:
  void publishEvents();

  gemfire::CacheListenerPtr listener;
  static RegionEventRegistry instance;
  std::set<node_gemfire::Region *> regionSet;
  EventStream * eventStream;
};

}  // namespace node_gemfire

#include "region.hpp"

#endif

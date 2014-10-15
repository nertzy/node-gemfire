#ifndef __REGION_EVENT_LISTENER_HPP__
#define __REGION_EVENT_LISTENER_HPP__

#include <gfcpp/CacheListener.hpp>

namespace node_gemfire {

class RegionEventListener : public gemfire::CacheListener {
 public:
  RegionEventListener() {}
  virtual void afterCreate(const gemfire::EntryEvent & event);
};

}  // namespace node_gemfire

#endif

#include "event_stream.hpp"
#include <nan.h>
#include <vector>
#include <string>
#include "conversions.hpp"

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

void EventStream::add(Event * event) {
  uv_mutex_lock(&mutex);

  eventVector.push_back(event);
  uv_ref(reinterpret_cast<uv_handle_t *>(&async));
  uv_mutex_unlock(&mutex);

  uv_async_send(&async);
}

std::vector<EventStream::Event *> EventStream::nextEvents() {
  uv_mutex_lock(&mutex);

  std::vector<Event *> returnValue;

  for (std::vector<Event *>::iterator iterator(eventVector.begin());
       iterator != eventVector.end();
       ++iterator) {
    Event * event(*iterator);
    returnValue.push_back(event);
  }

  eventVector.clear();

  uv_unref(reinterpret_cast<uv_handle_t *>(&async));
  uv_mutex_unlock(&mutex);

  return returnValue;
}

Local<Object> EventStream::Event::v8Object() {
  NanEscapableScope();

  Local<Object> eventPayload(NanNew<Object>());

  eventPayload->Set(NanNew("key"), v8Value(entryEventPtr->getKey()));
  eventPayload->Set(NanNew("oldValue"), v8Value(entryEventPtr->getOldValue()));
  eventPayload->Set(NanNew("newValue"), v8Value(entryEventPtr->getNewValue()));

  return NanEscapeScope(eventPayload);
}

std::string EventStream::Event::getName() {
  return eventName;
}

gemfire::RegionPtr EventStream::Event::getRegion() {
  return entryEventPtr->getRegion();
}

}  // namespace node_gemfire

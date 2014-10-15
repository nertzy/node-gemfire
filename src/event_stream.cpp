#include "event_stream.hpp"
#include <vector>

using namespace gemfire;

namespace node_gemfire {

void EventStream::add(const gemfire::EntryEvent & event) {
  uv_mutex_lock(&mutex);

  gemfire::EntryEventPtr entryEventPtr(
      new EntryEvent(event.getRegion(),
                     event.getKey(),
                     event.getOldValue(),
                     event.getNewValue(),
                     event.getCallbackArgument(),
                     event.remoteOrigin()));

  entryEventVector.push_back(entryEventPtr);
  uv_ref(reinterpret_cast<uv_handle_t *>(&async));
  uv_mutex_unlock(&mutex);

  uv_async_send(&async);
}

std::vector<gemfire::EntryEventPtr> EventStream::nextEvents() {
  uv_mutex_lock(&mutex);

  std::vector<gemfire::EntryEventPtr> returnValue;

  for (std::vector<gemfire::EntryEventPtr>::iterator iterator(entryEventVector.begin());
       iterator != entryEventVector.end();
       ++iterator) {
    gemfire::EntryEventPtr entryEventPtr(*iterator);
    returnValue.push_back(entryEventPtr);
  }

  entryEventVector.clear();

  uv_unref(reinterpret_cast<uv_handle_t *>(&async));
  uv_mutex_unlock(&mutex);

  return returnValue;
}

}  // namespace node_gemfire

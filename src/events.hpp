#ifndef __EVENTS_HPP__
#define __EVENTS_HPP__

#include <v8.h>
#include <string>

using namespace v8;

namespace node_gemfire {

void emitEvent(const Local<Object> & emitter, const char * eventName);
void emitEvent(const Local<Object> & emitter, const char * eventName, const Local<Value> & payload);
void emitError(const Local<Object> & emitter, const Local<Value> & error);

}  // namespace node_gemfire

#endif

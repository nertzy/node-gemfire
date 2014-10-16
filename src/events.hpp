#ifndef __EVENTS_HPP__
#define __EVENTS_HPP__

#include <v8.h>
#include <string>

namespace node_gemfire {

void emitEvent(const v8::Local<v8::Object> & emitter,
               const char * eventName);

void emitEvent(const v8::Local<v8::Object> & emitter,
               const char * eventName,
               const v8::Local<v8::Value> & payload);

void emitError(const v8::Local<v8::Object> & emitter,
               const v8::Local<v8::Value> & error);

}  // namespace node_gemfire

#endif

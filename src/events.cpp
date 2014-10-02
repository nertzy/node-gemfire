#include "events.hpp"
#include <nan.h>

namespace node_gemfire {

using namespace v8;

void emitEvent(const Local<Object> & emitter, const char * eventName) {
  NanScope();

  static const int argc = 1;
  Local<Value> argv[argc] = { NanNew(eventName) };
  NanMakeCallback(emitter, "emit", argc, argv);
}

void emitEvent(const Local<Object> & emitter, const char * eventName, const Local<Value> & payload) {
  NanScope();

  static const int argc = 2;
  Local<Value> argv[argc] = { NanNew(eventName), payload };
  NanMakeCallback(emitter, "emit", argc, argv);
}

void emitError(const Local<Object> & emitter, const Local<Value> & error) {
  emitEvent(emitter, "error", error);
}

}  // namespace node_gemfire

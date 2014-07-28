#include <v8.h>
#include <node.h>
#include <gfcpp/CacheFactory.hpp>
#include <nan.h>

NAN_METHOD(get_hello) {
  NanScope();
  NanReturnValue(NanNew<v8::String>("hello"));
}

NAN_METHOD(get_version) {
  NanScope();
  NanReturnValue(NanNew<v8::String>(gemfire::CacheFactory::getVersion()));
}

extern "C" {
  static void start(v8::Handle<v8::Object> target) {
    NanScope();
    NODE_SET_METHOD(target, "hello", get_hello);
    NODE_SET_METHOD(target, "version", get_version);
  }
}

NODE_MODULE(pivotal_gemfire, start)


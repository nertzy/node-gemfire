#include <v8.h>
#include <node.h>
#include <node_version.h>
#include <gfcpp/CacheFactory.hpp>
#include <nan.h>

using namespace v8;
using namespace gemfire;

NAN_METHOD(get_hello) {
  NanScope();
  NanReturnValue(NanNew<String>("hello"));
}

NAN_METHOD(get_version) {
  NanScope();
  NanReturnValue(NanNew<String>(CacheFactory::getVersion()));
}

extern "C" {
  static void start(Handle<Object> target) {
    NanScope();
    NODE_SET_METHOD(target, "hello", get_hello);
    NODE_SET_METHOD(target, "version", get_version);
  }
}

NODE_MODULE(pivotal_gemfire, start)


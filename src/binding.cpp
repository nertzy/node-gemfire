#include <v8.h>
#include <nan.h>
#include <gfcpp/CacheFactory.hpp>
#include "cache.hpp"
#include "region.hpp"

using namespace v8;

NAN_METHOD(version) {
  NanScope();
  NanReturnValue(NanNew(gemfire::CacheFactory::getVersion()));
}

static void Initialize(Handle<Object> exports) {
  NanScope();
  node_gemfire::Cache::Init(exports);
  node_gemfire::Region::Init(exports);

  NODE_SET_METHOD(exports, "version", version);
}

NODE_MODULE(gemfire, Initialize)


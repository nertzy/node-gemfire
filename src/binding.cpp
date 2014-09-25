#define NODE_GEMFIRE_VERSION "0.0.3"

#include <v8.h>
#include <nan.h>
#include <gfcpp/CacheFactory.hpp>
#include "cache.hpp"
#include "region.hpp"
#include "select_results.hpp"

using namespace v8;

static void Initialize(Local<Object> exports) {
  NanScope();

  exports->Set(NanNew("version"),
      NanNew(NODE_GEMFIRE_VERSION),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  exports->Set(NanNew("gemfireVersion"),
      NanNew(gemfire::CacheFactory::getVersion()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  node_gemfire::Cache::Init(exports);
  node_gemfire::Region::Init(exports);
  node_gemfire::SelectResults::Init(exports);
}

NODE_MODULE(gemfire, Initialize)

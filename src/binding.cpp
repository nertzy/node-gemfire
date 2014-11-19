#define NODE_GEMFIRE_VERSION "0.0.7-pre2"

#include <v8.h>
#include <nan.h>
#include <gfcpp/CacheFactory.hpp>
#include "dependencies.hpp"
#include "cache.hpp"
#include "region.hpp"
#include "select_results.hpp"

using namespace v8;

namespace node_gemfire {

NAN_METHOD(Initialize) {
  NanScope();

  Local<Object> gemfire(NanNew<Object>());

  gemfire->Set(NanNew("version"),
      NanNew(NODE_GEMFIRE_VERSION),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  gemfire->Set(NanNew("gemfireVersion"),
      NanNew(gemfire::CacheFactory::getVersion()),
      static_cast<PropertyAttribute>(ReadOnly | DontDelete));

  node_gemfire::Cache::Init(gemfire);
  node_gemfire::Region::Init(gemfire);
  node_gemfire::SelectResults::Init(gemfire);

  NanAssignPersistent(dependencies, args[0]->ToObject());

  NanReturnValue(gemfire);
}

}  // namespace node_gemfire

static void Initialize(Local<Object> exports) {
  Local<FunctionTemplate> initializeTemplate(NanNew<FunctionTemplate>(node_gemfire::Initialize));
  exports->Set(NanNew("initialize"), initializeTemplate->GetFunction());
}

NODE_MODULE(gemfire, Initialize)

#ifndef __FUNCTIONS_HPP__
#define __FUNCTIONS_HPP__

#include <nan.h>
#include <v8.h>
#include <gfcpp/Cache.hpp>

namespace node_gemfire {

using namespace v8;
using namespace gemfire;

Local<Value> executeFunction(_NAN_METHOD_ARGS,
                             const CachePtr & cachePtr,
                             const ExecutionPtr & executionPtr);

}  // namespace node_gemfire

#endif

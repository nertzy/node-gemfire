#ifndef __FUNCTIONS_HPP__
#define __FUNCTIONS_HPP__

#include <nan.h>
#include <v8.h>
#include <gfcpp/Cache.hpp>

namespace node_gemfire {

v8::Local<v8::Value> executeFunction(_NAN_METHOD_ARGS,
                                     const gemfire::CachePtr & cachePtr,
                                     const gemfire::ExecutionPtr & executionPtr);

}  // namespace node_gemfire

#endif

#ifndef __CONVERSIONS_HPP__

#include <v8.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>

using namespace v8;

namespace node_gemfire {

gemfire::CacheablePtr gemfireValueFromV8(const Handle<Value> & v8Value,
                                         const gemfire::CachePtr & cachePtr);
gemfire::PdxInstancePtr gemfireValueFromV8(const Handle<Object> & v8Object,
                                           const gemfire::CachePtr & cachePtr);
gemfire::CacheableKeyPtr gemfireKeyFromV8(const Handle<Value> & v8Value,
                                          const gemfire::CachePtr & cachePtr);
gemfire::VectorOfCacheableKeyPtr gemfireKeysFromV8(const Handle<Array> & v8Value,
                                          const gemfire::CachePtr & cachePtr);

Handle<Value> v8ValueFromGemfire(const gemfire::CacheablePtr & valuePtr);
Handle<Object> v8ValueFromGemfire(const gemfire::StructPtr & structPtr);
Handle<Value> v8ValueFromGemfire(const gemfire::PdxInstancePtr & pdxInstancePtr);
Handle<Object> v8ValueFromGemfire(const gemfire::SelectResultsPtr & selectResultsPtr);
Handle<Array> v8ValueFromGemfire(const gemfire::CacheableVectorPtr & vectorPtr);
Handle<Object> v8ValueFromGemfire(const gemfire::HashMapOfCacheablePtr & hashMapPtr);
Handle<Boolean> v8ValueFromGemfire(bool value);

}  // namespace node_gemfire

#define __CONVERSIONS_HPP__
#endif

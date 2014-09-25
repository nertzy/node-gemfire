#ifndef __CONVERSIONS_HPP__

#include <v8.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>
#include <string>

using namespace v8;

namespace node_gemfire {

gemfire::CacheablePtr gemfireValueFromV8(const Local<Value> & v8Value,
                                         const gemfire::CachePtr & cachePtr);
gemfire::PdxInstancePtr gemfireValueFromV8(const Local<Object> & v8Object,
                                           const gemfire::CachePtr & cachePtr);

gemfire::CacheableKeyPtr gemfireKeyFromV8(const Local<Value> & v8Value,
                                          const gemfire::CachePtr & cachePtr);
gemfire::VectorOfCacheableKeyPtr gemfireKeysFromV8(const Local<Array> & v8Value,
                                          const gemfire::CachePtr & cachePtr);

gemfire::HashMapOfCacheablePtr gemfireHashMapFromV8(const Local<Object> & v8Object,
                                           const gemfire::CachePtr & cachePtr);

Local<Value> v8ValueFromGemfire(const gemfire::CacheablePtr & valuePtr);
Local<Object> v8ValueFromGemfire(const gemfire::StructPtr & structPtr);
Local<Value> v8ValueFromGemfire(const gemfire::PdxInstancePtr & pdxInstancePtr);
Local<Object> v8ValueFromGemfire(const gemfire::SelectResultsPtr & selectResultsPtr);
Local<Array> v8ValueFromGemfire(const gemfire::CacheableVectorPtr & vectorPtr);
Local<Object> v8ValueFromGemfire(const gemfire::HashMapOfCacheablePtr & hashMapPtr);
Local<Boolean> v8ValueFromGemfire(bool value);

std::string getClassName(const Local<Object> & v8Object);

}  // namespace node_gemfire

#define __CONVERSIONS_HPP__
#endif

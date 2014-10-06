#ifndef __CONVERSIONS_HPP__
#define __CONVERSIONS_HPP__

#include <v8.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>
#include <string>

using namespace v8;

namespace node_gemfire {

gemfire::CacheablePtr gemfireValue(const Local<Value> & v8Value,
                                         const gemfire::CachePtr & cachePtr);
gemfire::PdxInstancePtr gemfireValue(const Local<Object> & v8Object,
                                           const gemfire::CachePtr & cachePtr);
gemfire::CacheableObjectArrayPtr gemfireValue(const Local<Array> & v8Value,
                                         const gemfire::CachePtr & cachePtr);
gemfire::CacheableDatePtr gemfireValue(const Local<Date> & v8Value);

gemfire::CacheableKeyPtr gemfireKey(const Local<Value> & v8Value,
                                          const gemfire::CachePtr & cachePtr);
gemfire::VectorOfCacheableKeyPtr gemfireKeys(const Local<Array> & v8Value,
                                          const gemfire::CachePtr & cachePtr);

gemfire::HashMapOfCacheablePtr gemfireHashMap(const Local<Object> & v8Object,
                                           const gemfire::CachePtr & cachePtr);
gemfire::CacheableVectorPtr gemfireVector(const Local<Array> & v8Array,
                                           const gemfire::CachePtr & cachePtr);

Local<Value> v8Value(const gemfire::CacheablePtr & valuePtr);
Local<Value> v8Value(const gemfire::CacheableKeyPtr & keyPtr);
Local<Value> v8Value(const gemfire::CacheableInt64Ptr & valuePtr);
Local<Object> v8Value(const gemfire::StructPtr & structPtr);
Local<Value> v8Value(const gemfire::PdxInstancePtr & pdxInstancePtr);
Local<Object> v8Value(const gemfire::SelectResultsPtr & selectResultsPtr);
Local<Object> v8Value(const gemfire::CacheableHashMapPtr & hashMapPtr);
Local<Object> v8Value(const gemfire::HashMapOfCacheablePtr & hashMapPtr);
Local<Array> v8Value(const gemfire::VectorOfCacheableKeyPtr & vectorPtr);
Local<Date> v8Value(const gemfire::CacheableDatePtr & datePtr);
Local<Boolean> v8Value(bool value);

template<typename T>
Local<Array> v8Array(const gemfire::SharedPtr<T> & iterablePtr) {
  NanEscapableScope();

  unsigned int length = iterablePtr->size();
  Local<Array> v8Array(NanNew<Array>(length));

  unsigned int i = 0;
  for (typename T::Iterator iterator(iterablePtr->begin()); iterator != iterablePtr->end(); ++iterator) {
    v8Array->Set(i, v8Value(*iterator));
    i++;
  }

  return NanEscapeScope(v8Array);
}

template<typename T>
Local<Object> v8Object(const gemfire::SharedPtr<T> & hashMapPtr) {
  NanEscapableScope();

  Local<Object> v8Object(NanNew<Object>());

  for (typename T::Iterator i = hashMapPtr->begin(); i != hashMapPtr->end(); i++) {
    gemfire::CacheablePtr keyPtr(i.first());
    gemfire::CacheablePtr valuePtr(i.second());

    v8Object->Set(v8Value(keyPtr),
        v8Value(valuePtr));
  }

  return NanEscapeScope(v8Object);
}

std::string getClassName(const Local<Object> & v8Object);

}  // namespace node_gemfire

#endif

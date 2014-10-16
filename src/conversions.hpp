#ifndef __CONVERSIONS_HPP__
#define __CONVERSIONS_HPP__

#include <v8.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>
#include <string>

namespace node_gemfire {

gemfire::CacheablePtr gemfireValue(const v8::Local<v8::Value> & v8Value,
                                         const gemfire::CachePtr & cachePtr);
gemfire::PdxInstancePtr gemfireValue(const v8::Local<v8::Object> & v8Object,
                                           const gemfire::CachePtr & cachePtr);
gemfire::CacheableObjectArrayPtr gemfireValue(const v8::Local<v8::Array> & v8Value,
                                         const gemfire::CachePtr & cachePtr);
gemfire::CacheableDatePtr gemfireValue(const v8::Local<v8::Date> & v8Value);

gemfire::CacheableKeyPtr gemfireKey(const v8::Local<v8::Value> & v8Value,
                                          const gemfire::CachePtr & cachePtr);
gemfire::VectorOfCacheableKeyPtr gemfireKeys(const v8::Local<v8::Array> & v8Value,
                                          const gemfire::CachePtr & cachePtr);

gemfire::HashMapOfCacheablePtr gemfireHashMap(const v8::Local<v8::Object> & v8Object,
                                           const gemfire::CachePtr & cachePtr);
gemfire::CacheableVectorPtr gemfireVector(const v8::Local<v8::Array> & v8Array,
                                           const gemfire::CachePtr & cachePtr);

v8::Local<v8::Value> v8Value(const gemfire::CacheablePtr & valuePtr);
v8::Local<v8::Value> v8Value(const gemfire::CacheableKeyPtr & keyPtr);
v8::Local<v8::Value> v8Value(const gemfire::CacheableInt64Ptr & valuePtr);
v8::Local<v8::Object> v8Value(const gemfire::StructPtr & structPtr);
v8::Local<v8::Value> v8Value(const gemfire::PdxInstancePtr & pdxInstancePtr);
v8::Local<v8::Object> v8Value(const gemfire::SelectResultsPtr & selectResultsPtr);
v8::Local<v8::Object> v8Value(const gemfire::CacheableHashMapPtr & hashMapPtr);
v8::Local<v8::Object> v8Value(const gemfire::HashMapOfCacheablePtr & hashMapPtr);
v8::Local<v8::Array> v8Value(const gemfire::VectorOfCacheableKeyPtr & vectorPtr);
v8::Local<v8::Date> v8Value(const gemfire::CacheableDatePtr & datePtr);
v8::Local<v8::Boolean> v8Value(bool value);

template<typename T>
v8::Local<v8::Array> v8Array(const gemfire::SharedPtr<T> & iterablePtr) {
  NanEscapableScope();

  unsigned int length = iterablePtr->size();
  v8::Local<v8::Array> v8Array(NanNew<v8::Array>(length));

  unsigned int i = 0;
  for (typename T::Iterator iterator(iterablePtr->begin());
       iterator != iterablePtr->end();
       ++iterator) {
    v8Array->Set(i, v8Value(*iterator));
    i++;
  }

  return NanEscapeScope(v8Array);
}

template<typename T>
v8::Local<v8::Object> v8Object(const gemfire::SharedPtr<T> & hashMapPtr) {
  NanEscapableScope();

  v8::Local<v8::Object> v8Object(NanNew<v8::Object>());

  for (typename T::Iterator iterator = hashMapPtr->begin();
       iterator != hashMapPtr->end();
       iterator++) {
    gemfire::CacheablePtr keyPtr(iterator.first());
    gemfire::CacheablePtr valuePtr(iterator.second());

    v8Object->Set(v8Value(keyPtr),
        v8Value(valuePtr));
  }

  return NanEscapeScope(v8Object);
}

std::string getClassName(const v8::Local<v8::Object> & v8Object);

}  // namespace node_gemfire

#endif

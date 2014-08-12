#ifndef __CONVERSIONS_HPP__

#include <v8.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>

using namespace v8;
using namespace gemfire;

class V8ObjectFormatter {
 public:
    static PdxInstancePtr toPdxInstance(CachePtr cache, Local<Object> v8Object);
    static Handle<Value> fromPdxInstance(PdxInstancePtr pdxInstance);
};

CacheablePtr gemfireValueFromV8(Handle<Value> v8Value, CachePtr cachePtr);
Handle<Value> v8ValueFromGemfire(CacheablePtr valuePtr);

#define __CONVERSIONS_HPP__
#endif

#ifndef __V8_OBJECT_FORMATTER_HPP__

#include <v8.h>
#include <nan.h>
#include <gfcpp/PdxInstanceFactory.hpp>
#include <gfcpp/CacheFactory.hpp>

class V8ObjectFormatter {
  public:
    static gemfire::PdxInstancePtr toPdxInstance(gemfire::CachePtr cache, v8::Local<v8::Object> v8Object);
    static v8::Local<v8::Object> fromPdxInstance(gemfire::PdxInstancePtr pdxInstance);
};

#define __V8_OBJECT_FORMATTER_HPP__
#endif

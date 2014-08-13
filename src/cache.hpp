#ifndef __CACHE_HPP__

#include <v8.h>
#include <nan.h>
#include <node.h>
#include <gfcpp/Cache.hpp>

using namespace v8;

namespace node_gemfire {

class Cache : public node::ObjectWrap {
 public:
    static void Init(Handle<Object> exports);

    gemfire::CachePtr cachePtr;
 protected:
    ~Cache();
    static NAN_METHOD(New);
    static NAN_METHOD(ExecuteQuery);
    static NAN_METHOD(GetRegion);

    explicit Cache(gemfire::CachePtr cachePtr) : cachePtr(cachePtr) {}
};

}  // namespace node_gemfire

#define __CACHE_HPP__
#endif

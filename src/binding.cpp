#include <v8.h>
#include <node.h>
#include <node_version.h>
#include <gfcpp/CacheFactory.hpp>

static v8::Handle<v8::Value> get_hello(const v8::Arguments& args)
{
    v8::HandleScope scope;
    return scope.Close(v8::String::New("hello"));
}

static v8::Handle<v8::Value> get_version(const v8::Arguments& args)
{
    const char* version = gemfire::CacheFactory::getVersion();

    v8::HandleScope scope;
    return scope.Close(v8::String::New(version));
}

extern "C" {
    static void start(v8::Handle<v8::Object> target) {
        v8::HandleScope scope;
        NODE_SET_METHOD(target, "hello", get_hello);
        NODE_SET_METHOD(target, "version", get_version);
    }
}

NODE_MODULE(pivotal_gemfire, start)


#ifndef __REGION_HPP__
#define __REGION_HPP__

#include <v8.h>
#include <nan.h>
#include <node.h>
#include <gfcpp/Region.hpp>
#include "region_event_registry.hpp"

namespace node_gemfire {

class Region : public node::ObjectWrap {
 public:
  Region(v8::Local<v8::Object> regionHandle,
         v8::Local<v8::Object> cacheHandle,
         gemfire::RegionPtr regionPtr) :
    regionPtr(regionPtr) {
      Wrap(regionHandle);
      NanAssignPersistent(this->cacheHandle, cacheHandle);
    }

  virtual ~Region() {
    RegionEventRegistry::getInstance()->remove(this);
    NanDisposePersistent(cacheHandle);
  }

  static void Init(v8::Local<v8::Object> exports);
  static v8::Local<v8::Value> New(v8::Local<v8::Object> cacheObject,
                                  gemfire::RegionPtr regionPtr);
  static NAN_METHOD(Clear);
  static NAN_METHOD(Put);
  static NAN_METHOD(PutSync);
  static NAN_METHOD(Get);
  static NAN_METHOD(GetSync);
  static NAN_METHOD(GetAll);
  static NAN_METHOD(Entries);
  static NAN_METHOD(PutAll);
  static NAN_METHOD(Remove);
  static NAN_METHOD(ServerKeys);
  static NAN_METHOD(Keys);
  static NAN_METHOD(Values);
  static NAN_METHOD(ExecuteFunction);
  static NAN_METHOD(RegisterAllKeys);
  static NAN_METHOD(UnregisterAllKeys);
  static NAN_METHOD(DestroyRegion);
  static NAN_METHOD(LocalDestroyRegion);
  static NAN_METHOD(Inspect);
  static NAN_GETTER(Name);
  static NAN_GETTER(Attributes);

  template<typename T>
  static NAN_METHOD(Query);

  gemfire::RegionPtr regionPtr;

 private:
  v8::Persistent<v8::Object> cacheHandle;
  static v8::Persistent<v8::Function> constructor;
};

}  // namespace node_gemfire

#endif

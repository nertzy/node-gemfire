#include "v8_object_formatter.hpp"

using namespace v8;
using namespace gemfire;

void randomString(char * string, const unsigned int length) {
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  for (unsigned int i = 0; i < length; ++i) {
    string[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  string[length] = 0;
};

PdxInstancePtr V8ObjectFormatter::toPdxInstance(CachePtr cache, Local<Object> v8Object) {
  NanScope();

  char * pdxClassName = new char[32];
  randomString(pdxClassName, 32);

  PdxInstanceFactoryPtr pdxInstanceFactory = cache->createPdxInstanceFactory(pdxClassName);

  Local<Array> v8Keys = v8Object->GetOwnPropertyNames();
  for(unsigned int i = 0; i < v8Keys->Length(); i++) {
    Local<String> v8Key = v8Keys->Get(i)->ToString();
    Local<Value> v8Value = v8Object->Get(v8Key)->ToString();

    String::Utf8Value key(v8Key);
    String::Utf8Value value(v8Value);

    pdxInstanceFactory->writeString(*key, *value);
  }

  return pdxInstanceFactory->create();
}

Local<Object> V8ObjectFormatter::fromPdxInstance(PdxInstancePtr pdxInstance) {
  NanScope();

  Local<Object> v8Object = NanNew<Object>();

  CacheableStringArrayPtr gemfireKeys = pdxInstance->getFieldNames();
  for(int i = 0; i < gemfireKeys->length(); i++) {
    const char* key = gemfireKeys[i]->asChar();
    char* value;

    pdxInstance->getField(key, &value);

    Local<String> v8Key = NanNew<String>(key);
    Local<String> v8Value = NanNew<String>(value);

    v8Object->Set(v8Key, v8Value);
  }

  return v8Object;
}

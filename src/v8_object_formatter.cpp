#include "v8_object_formatter.hpp"

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

gemfire::PdxInstancePtr V8ObjectFormatter::toPdxInstance(gemfire::CachePtr cache, v8::Local<v8::Object> v8Object) {
  NanScope();

  char * pdxClassName = new char[32];
  randomString(pdxClassName, 32);

  gemfire::PdxInstanceFactoryPtr pdxInstanceFactory = cache->createPdxInstanceFactory(pdxClassName);

  v8::Local<v8::Array> v8Keys = v8Object->GetOwnPropertyNames();
  for(unsigned int i = 0; i < v8Keys->Length(); i++) {
    v8::Local<v8::String> v8Key = v8Keys->Get(i)->ToString();
    v8::Local<v8::Value> v8Value = v8Object->Get(v8Key)->ToString();

    v8::String::AsciiValue key(v8Key);
    v8::String::AsciiValue value(v8Value);

    pdxInstanceFactory->writeString(*key, *value);
  }

  return pdxInstanceFactory->create();
}

v8::Local<v8::Object> V8ObjectFormatter::fromPdxInstance(gemfire::PdxInstancePtr pdxInstance) {
  NanScope();

  v8::Local<v8::Object> v8Object = NanNew<v8::Object>();

  gemfire::CacheableStringArrayPtr gemfireKeys = pdxInstance->getFieldNames();
  for(int i = 0; i < gemfireKeys->length(); i++) {
    const char* key = gemfireKeys[i]->asChar();
    char* value;

    pdxInstance->getField(key, &value);

    v8::Local<v8::String> v8Key = NanNew<v8::String>(key);
    v8::Local<v8::String> v8Value = NanNew<v8::String>(value);

    v8Object->Set(v8Key, v8Value);
  }

  return v8Object;
}

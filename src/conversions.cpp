#include <node.h>
#include <nan.h>
#include <v8.h>
#include <gfcpp/GemfireCppCache.hpp>
#include <sstream>
#include "conversions.hpp"
#include "exceptions.hpp"

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

PdxInstancePtr V8ObjectFormatter::toPdxInstance(CachePtr cachePtr, Local<Object> v8Object) {
  try {
    NanScope();

    char * pdxClassName = new char[32];
    randomString(pdxClassName, 32);

    PdxInstanceFactoryPtr pdxInstanceFactory = cachePtr->createPdxInstanceFactory(pdxClassName);

    Local<Array> v8Keys = v8Object->GetOwnPropertyNames();
    for(unsigned int i = 0; i < v8Keys->Length(); i++) {
      Local<Value> v8Key = v8Keys->Get(i);

      String::Utf8Value key(v8Key);

      Local<Value> v8Value = v8Object->Get(v8Key);

      if(v8Value->IsString()){
        String::Value v8String(v8Value);
        unsigned int length = v8Value->ToString()->Length();
        wchar_t * wcharData = new wchar_t[length + 1];

        for(unsigned int i = 0; i < length; i++) {
          wcharData[i] = (*v8String)[i];
        }
        wcharData[length] = 0;

        pdxInstanceFactory->writeWideString(*key, wcharData);
        delete [] wcharData;
      }
      else if(v8Value->IsArray()){
        pdxInstanceFactory->writeObjectArray(*key, gemfireValueFromV8(v8Value, cachePtr));
      }
      else {
        pdxInstanceFactory->writeObject(*key, gemfireValueFromV8(v8Value, cachePtr));
      }
    }

    return pdxInstanceFactory->create();
  }
  catch(const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
    return NULLPTR;
  }
}

Handle<Value> V8ObjectFormatter::fromPdxInstance(PdxInstancePtr pdxInstance) {
  try {
    NanScope();

    CacheableStringArrayPtr gemfireKeys = pdxInstance->getFieldNames();

    if(gemfireKeys == NULLPTR) {
      NanReturnValue(NanNew<Object>());
    }

    Local<Object> v8Object = NanNew<Object>();
    for(int i = 0; i < gemfireKeys->length(); i++) {
      const char * key = gemfireKeys[i]->asChar();
      CacheablePtr value;

      try {
        pdxInstance->getField(key, value);
      }
      catch( gemfire::IllegalStateException &exception ) {
        // Unfortunately, getting an object array field from Gemfire as a vanilla CacheablePtr triggers an
        // exception. We don't know a better way to detect that we are about to read in an array, so for now we
        // catch the exception and assume we are receiving an array.
        CacheableObjectArrayPtr valueArray;
        pdxInstance->getField(key, valueArray);
        value = valueArray;
      }

      Local<String> v8Key = NanNew<String>(key);
      Handle<Value> v8Value = v8ValueFromGemfire(value);

      v8Object->Set(v8Key, v8Value);
    }

    NanReturnValue(v8Object);
  }
  catch(const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
    NanReturnUndefined();
  }
}

CacheablePtr gemfireValueFromV8(Handle<Value> v8Value, CachePtr cachePtr) {
  CacheablePtr gemfireValuePtr;

  if(v8Value->IsString()) {
    gemfireValuePtr = CacheableString::create(*String::Utf8Value(v8Value));
  }
  else if(v8Value->IsBoolean()) {
    gemfireValuePtr = CacheableBoolean::create(v8Value->ToBoolean()->Value());
  }
  else if(v8Value->IsNumber()) {
    gemfireValuePtr = CacheableDouble::create(v8Value->ToNumber()->Value());
  }
  else if(v8Value->IsDate()) {
    long millisecondsSinceEpoch = Date::Cast(*v8Value)->NumberValue();

    timeval timeSinceEpoch;
    timeSinceEpoch.tv_sec = millisecondsSinceEpoch / 1000;
    timeSinceEpoch.tv_usec = (millisecondsSinceEpoch % 1000) * 1000;

    gemfireValuePtr = CacheableDate::create(timeSinceEpoch);
  }
  else if(v8Value->IsArray()) {
    Handle<Array> v8Array = Handle<Array>::Cast(v8Value);
    unsigned int length = v8Array->Length();

    gemfireValuePtr = CacheableObjectArray::create();
    for(unsigned int i = 0; i < length; i++) {
      ((CacheableObjectArrayPtr) gemfireValuePtr)->push_back(gemfireValueFromV8(v8Array->Get(i), cachePtr));
    }
  }
  else if(v8Value->IsObject()) {
    gemfireValuePtr = V8ObjectFormatter::toPdxInstance(cachePtr, v8Value->ToObject());
  }
  else if(v8Value->IsNull()) {
    gemfireValuePtr = CacheableUndefined::create();
  }
  else {
    gemfireValuePtr = NULLPTR;
  }

  return gemfireValuePtr;
};

Handle<Value> v8ValueFromGemfire(CacheablePtr valuePtr) {
  NanScope();

  if(valuePtr == NULLPTR) {
    NanReturnUndefined();
  }

  int typeId = valuePtr->typeId();
  if(typeId == GemfireTypeIds::CacheableASCIIString) {
    NanReturnValue(NanNew(((CacheableStringPtr) valuePtr)->asChar()));
  }
  if(typeId == GemfireTypeIds::CacheableString) {
    std::wstring wideString = ((CacheableStringPtr) valuePtr)->asWChar();

    unsigned int length = wideString.length();
    uint16_t * buffer = new uint16_t[length + 1];
    for(unsigned int i = 0; i <= length; i++) {
      buffer[i] = wideString[i];
    }
    Local<String> string = NanNew<String>(buffer);

    delete[] buffer;

    NanReturnValue(NanNew(string));
  }
  if(typeId == GemfireTypeIds::CacheableBoolean) {
    NanReturnValue(NanNew<Boolean>(((CacheableBooleanPtr) valuePtr)->value()));
  }
  if(typeId == GemfireTypeIds::CacheableDouble) {
    NanReturnValue(NanNew<Number>(((CacheableDoublePtr) valuePtr)->value()));
  }
  if(typeId == GemfireTypeIds::CacheableDate) {
    NanReturnValue(NanNew<Date>((double) ((CacheableDatePtr) valuePtr)->milliseconds()));
  }
  if(typeId == GemfireTypeIds::CacheableUndefined) {
    NanReturnNull();
  }
  if(typeId == GemfireTypeIds::CacheableObjectArray) {
    CacheableObjectArrayPtr gemfireArray = (CacheableObjectArrayPtr) valuePtr;
    unsigned int length = gemfireArray->length();

    Handle<Array> v8Array = NanNew<Array>(length);
    for(unsigned int i = 0; i < length; i++) {
      v8Array->Set(i, v8ValueFromGemfire((*gemfireArray)[i]));
    }

    NanReturnValue(v8Array);
  }
  else if(typeId > GemfireTypeIds::CacheableStringHuge) {
    //We are assuming these are Pdx
    NanReturnValue(V8ObjectFormatter::fromPdxInstance(valuePtr));
  }
  else {
    std::stringstream errorMessageStream;
    errorMessageStream << "Unknown typeId: " << typeId;
    NanThrowError(errorMessageStream.str().c_str());
    NanReturnUndefined();
  }
}


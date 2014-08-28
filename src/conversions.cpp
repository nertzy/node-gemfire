#include <node.h>
#include <nan.h>
#include <v8.h>
#include <gfcpp/GemfireCppCache.hpp>
#include <sstream>
#include "conversions.hpp"
#include "exceptions.hpp"

using namespace v8;
using namespace gemfire;

void randomString(char * str, const unsigned int length) {
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  for (unsigned int i = 0; i < length; ++i) {
    str[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  str[length] = 0;
}

std::wstring wstringFromV8String(const Handle<String> & v8String) {
  unsigned int length = v8String->Length();
  wchar_t * buffer = new wchar_t[length + 1];
  NanUcs2String v8Data(v8String);
  for (unsigned int i = 0; i < length; i++) {
    buffer[i] = (*v8Data)[i];
  }
  buffer[length] = 0;

  std::wstring wstring(buffer);
  delete [] buffer;

  return wstring;
}

Handle<String> v8StringFromWstring(const std::wstring & wideString) {
  NanScope();

  unsigned int length = wideString.length();
  uint16_t * buffer = new uint16_t[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    buffer[i] = wideString[i];
  }
  buffer[length] = 0;

  Local<String> v8String = NanNew(buffer);
  delete[] buffer;

  NanReturnValue(v8String);
}

PdxInstancePtr V8ObjectFormatter::toPdxInstance(const CachePtr & cachePtr, const Local<Object> & v8Object) {
  try {
    NanScope();

    char pdxClassName[33];
    randomString(pdxClassName, 32);

    PdxInstanceFactoryPtr pdxInstanceFactory = cachePtr->createPdxInstanceFactory(pdxClassName);

    Local<Array> v8Keys = v8Object->GetOwnPropertyNames();
    unsigned int length = v8Keys->Length();

    for (unsigned int i = 0; i < length; i++) {
      Local<Value> v8Key = v8Keys->Get(i);
      Local<Value> v8Value = v8Object->Get(v8Key);

      // Copy the key string since gemfire is going to clean it up for us
      String::Utf8Value v8String(v8Key);
      char * key = new char[v8String.length()+1];
      snprintf(key, v8String.length() + 1, "%s", *v8String);

      CacheablePtr cacheablePtr = gemfireValueFromV8(v8Value, cachePtr);
      if (v8Value->IsArray()) {
        pdxInstanceFactory->writeObjectArray(key, cacheablePtr);
      } else {
        pdxInstanceFactory->writeObject(key, cacheablePtr);
      }
    }

    return pdxInstanceFactory->create();
  }
  catch(const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
    return NULLPTR;
  }
}

Handle<Value> V8ObjectFormatter::fromPdxInstance(const PdxInstancePtr & pdxInstance) {
  try {
    NanScope();

    CacheableStringArrayPtr gemfireKeys = pdxInstance->getFieldNames();

    if (gemfireKeys == NULLPTR) {
      NanReturnValue(NanNew<Object>());
    }

    Local<Object> v8Object = NanNew<Object>();
    int length = gemfireKeys->length();

    for (int i = 0; i < length; i++) {
      const char * key = gemfireKeys[i]->asChar();

      CacheablePtr value;
      try {
        pdxInstance->getField(key, value);
      }
      catch(gemfire::IllegalStateException &exception) {
        // Unfortunately, getting an object array field from Gemfire as a vanilla CacheablePtr
        // triggers an exception. We don't know a better way to detect that we are about to read in
        // an array, so for now we catch the exception and assume we are receiving an array.
        CacheableObjectArrayPtr valueArray;
        pdxInstance->getField(key, valueArray);
        value = valueArray;
      }

      v8Object->Set(NanNew(key), v8ValueFromGemfire(value));
    }

    NanReturnValue(v8Object);
  }
  catch(const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
    NanReturnUndefined();
  }
}

CacheablePtr gemfireValueFromV8(const Handle<Value> & v8Value, const CachePtr & cachePtr) {
  CacheablePtr gemfireValuePtr;

  if (v8Value->IsString()) {
    gemfireValuePtr = CacheableString::create(wstringFromV8String(v8Value->ToString()).c_str());
  } else if (v8Value->IsBoolean()) {
    gemfireValuePtr = CacheableBoolean::create(v8Value->ToBoolean()->Value());
  } else if (v8Value->IsNumber()) {
    gemfireValuePtr = CacheableDouble::create(v8Value->ToNumber()->Value());
  } else if (v8Value->IsDate()) {
    uint64 millisecondsSinceEpoch = Date::Cast(*v8Value)->NumberValue();

    timeval timeSinceEpoch;
    timeSinceEpoch.tv_sec = millisecondsSinceEpoch / 1000;
    timeSinceEpoch.tv_usec = (millisecondsSinceEpoch % 1000) * 1000;

    gemfireValuePtr = CacheableDate::create(timeSinceEpoch);
  } else if (v8Value->IsArray()) {
    Handle<Array> v8Array = Handle<Array>::Cast(v8Value);
    unsigned int length = v8Array->Length();

    gemfireValuePtr = CacheableObjectArray::create();
    for (unsigned int i = 0; i < length; i++) {
      ((CacheableObjectArrayPtr) gemfireValuePtr)->push_back(
        gemfireValueFromV8(v8Array->Get(i), cachePtr));
    }
  } else if (v8Value->IsObject()) {
    gemfireValuePtr = V8ObjectFormatter::toPdxInstance(cachePtr, v8Value->ToObject());
  } else if (v8Value->IsNull()) {
    gemfireValuePtr = CacheableUndefined::create();
  } else {
    gemfireValuePtr = NULLPTR;
  }

  return gemfireValuePtr;
}

Handle<Value> v8ValueFromGemfire(const CacheablePtr & valuePtr) {
  NanScope();

  if (valuePtr == NULLPTR) {
    NanReturnUndefined();
  }

  int typeId = valuePtr->typeId();
  if (typeId == GemfireTypeIds::CacheableASCIIString) {
    NanReturnValue(NanNew(((CacheableStringPtr) valuePtr)->asChar()));
  }
  if (typeId == GemfireTypeIds::CacheableString) {
    NanReturnValue(v8StringFromWstring(((CacheableStringPtr) valuePtr)->asWChar()));
  }
  if (typeId == GemfireTypeIds::CacheableBoolean) {
    NanReturnValue(NanNew(((CacheableBooleanPtr) valuePtr)->value()));
  }
  if (typeId == GemfireTypeIds::CacheableDouble) {
    NanReturnValue(NanNew(((CacheableDoublePtr) valuePtr)->value()));
  }
  if (typeId == GemfireTypeIds::CacheableDate) {
    NanReturnValue(NanNew<Date>(
          static_cast<double>(((CacheableDatePtr) valuePtr)->milliseconds())));
  }
  if (typeId == GemfireTypeIds::CacheableUndefined) {
    NanReturnNull();
  }
  if (typeId == GemfireTypeIds::Struct) {
    NanReturnValue(v8ObjectFromGemfireStruct(valuePtr));
  }
  if (typeId == GemfireTypeIds::CacheableObjectArray) {
    CacheableObjectArrayPtr gemfireArray = (CacheableObjectArrayPtr) valuePtr;
    unsigned int length = gemfireArray->length();

    Handle<Array> v8Array = NanNew<Array>(length);
    for (unsigned int i = 0; i < length; i++) {
      v8Array->Set(i, v8ValueFromGemfire((*gemfireArray)[i]));
    }

    NanReturnValue(v8Array);
  }
  if (typeId == GemfireTypeIds::CacheableVector) {
    CacheableVectorPtr gemfireVector = (CacheableVectorPtr) valuePtr;
    unsigned int length = gemfireVector->length();

    Handle<Array> v8Array = NanNew<Array>(length);
    for (unsigned int i = 0; i < length; i++) {
      v8Array->Set(i, v8ValueFromGemfire((*gemfireVector)[i]));
    }

    NanReturnValue(v8Array);
  } else if (typeId > GemfireTypeIds::CacheableStringHuge) {
    // We are assuming these are Pdx
    NanReturnValue(V8ObjectFormatter::fromPdxInstance(valuePtr));
  } else {
    std::stringstream errorMessageStream;
    errorMessageStream << "Unknown typeId: " << typeId;
    NanThrowError(errorMessageStream.str().c_str());
    NanReturnUndefined();
  }
}

Handle<Array> arrayFromSelectResults(const SelectResultsPtr & selectResultsPtr) {
  NanScope();

  Local<Array> array = NanNew<Array>();

  SelectResultsIterator iterator = selectResultsPtr->getIterator();

  while (iterator.hasNext()) {
    const SerializablePtr result = iterator.next();
    Handle<Value> v8Value = v8ValueFromGemfire(result);
    array->Set(array->Length(), v8Value);
  }

  NanReturnValue(array);
}

Handle<Object> v8ObjectFromGemfireStruct(const gemfire::StructPtr & structPtr) {
  NanScope();

  Local<Object> v8Object = NanNew<Object>();

  unsigned int length = structPtr->length();
  for (unsigned int i = 0; i < length; i++) {
    v8Object->Set(NanNew(structPtr->getFieldName(i)),
                  v8ValueFromGemfire((*structPtr)[i]));
  }

  NanReturnValue(v8Object);
}

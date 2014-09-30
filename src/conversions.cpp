#include <node.h>
#include <nan.h>
#include <v8.h>
#include <math.h>
#include <gfcpp/GemfireCppCache.hpp>
#include <string>
#include <sstream>
#include <set>
#include "conversions.hpp"
#include "exceptions.hpp"
#include "select_results.hpp"

using namespace v8;
using namespace gemfire;

namespace node_gemfire {

std::string getClassName(const Local<Object> & v8Object) {
  NanScope();

  Local<Array> v8Keys(v8Object->GetOwnPropertyNames());

  std::set<std::string> fieldNames;
  unsigned int length = v8Keys->Length();
  for (unsigned int i = 0; i < length; i++) {
    Local<Value> v8Key(v8Keys->Get(i));
    NanUtf8String utf8FieldName(v8Key);
    char * fieldName = *utf8FieldName;
    unsigned int size = utf8FieldName.Size();

    std::stringstream fullFieldName;

    for (unsigned int i = 0; i < size - 1; i++) {
      char fieldNameChar = fieldName[i];
      switch (fieldNameChar) {
        case ',':
        case '[':
        case ']':
        case '\\':
          fullFieldName << '\\';
      }
      fullFieldName << fieldNameChar;
    }

    Local<Value> v8Value(v8Object->Get(v8Key));
    if (v8Value->IsArray() && !v8Value->IsString()) {
      fullFieldName << "[]";
    }
    fullFieldName << ',';

    fieldNames.insert(fullFieldName.str());
  }

  std::stringstream className;
  className << "JSON: ";

  for (std::set<std::string>::iterator i(fieldNames.begin()); i != fieldNames.end(); ++i) {
    className << *i;
  }

  return className.str();
}

std::wstring wstringFromV8String(const Local<String> & v8String) {
  NanScope();

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

Local<String> v8StringFromWstring(const std::wstring & wideString) {
  NanEscapableScope();

  unsigned int length = wideString.length();
  uint16_t * buffer = new uint16_t[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    buffer[i] = wideString[i];
  }
  buffer[length] = 0;

  Local<String> v8String(NanNew(buffer));
  delete[] buffer;

  return NanEscapeScope(v8String);
}

PdxInstancePtr gemfireValueFromV8(const Local<Object> & v8Object, const CachePtr & cachePtr) {
  NanEscapableScope();

  try {
    std::string pdxClassName = getClassName(v8Object);
    PdxInstanceFactoryPtr pdxInstanceFactory = cachePtr->createPdxInstanceFactory(pdxClassName.c_str());

    Local<Array> v8Keys(v8Object->GetOwnPropertyNames());
    unsigned int length = v8Keys->Length();

    for (unsigned int i = 0; i < length; i++) {
      Local<Value> v8Key(v8Keys->Get(i));
      Local<Value> v8Value(v8Object->Get(v8Key));

      NanUtf8String fieldName(v8Key);

      CacheablePtr cacheablePtr(gemfireValueFromV8(v8Value, cachePtr));
      if (v8Value->IsArray()) {
        pdxInstanceFactory->writeObjectArray(*fieldName, cacheablePtr);
      } else {
        pdxInstanceFactory->writeObject(*fieldName, cacheablePtr);
      }
    }

    return pdxInstanceFactory->create();
  }
  catch(const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
    return NULLPTR;
  }
}

CacheableKeyPtr gemfireKeyFromV8(const Local<Value> & v8Value, const CachePtr & cachePtr) {
  CacheableKeyPtr keyPtr;
  try {
    keyPtr = gemfireValueFromV8(v8Value, cachePtr);
  }
  catch(const ClassCastException & exception) {
    return NULLPTR;
  }

  return keyPtr;
}

VectorOfCacheableKeyPtr gemfireKeysFromV8(const Local<Array> & v8Value,
                                          const CachePtr & cachePtr) {
  VectorOfCacheableKeyPtr vectorPtr(new VectorOfCacheableKey());

  for (unsigned int i = 0; i < v8Value->Length(); i++) {
    CacheableKeyPtr keyPtr = gemfireKeyFromV8(v8Value->Get(i), cachePtr);

    if (keyPtr == NULLPTR) {
      return NULLPTR;
    } else {
      vectorPtr->push_back(keyPtr);
    }
  }

  return vectorPtr;
}

Local<Value> v8ValueFromGemfire(const PdxInstancePtr & pdxInstance) {
  NanEscapableScope();

  try {
    CacheableStringArrayPtr gemfireKeys(pdxInstance->getFieldNames());

    if (gemfireKeys == NULLPTR) {
      return NanEscapeScope(NanNew<Object>());
    }

    Local<Object> v8Object(NanNew<Object>());
    int length = gemfireKeys->length();

    for (int i = 0; i < length; i++) {
      const char * key = gemfireKeys[i]->asChar();

      CacheablePtr value;
      try {
        pdxInstance->getField(key, value);
      }
      catch(const IllegalStateException & exception) {
        // Unfortunately, getting an object array field from Gemfire as a vanilla CacheablePtr
        // triggers an exception. We don't know a better way to detect that we are about to read in
        // an array, so for now we catch the exception and assume we are receiving an array.
        CacheableObjectArrayPtr valueArray;
        pdxInstance->getField(key, valueArray);
        value = valueArray;
      }

      v8Object->Set(NanNew(key), v8ValueFromGemfire(value));
    }

    return NanEscapeScope(v8Object);
  }
  catch(const gemfire::Exception & exception) {
    ThrowGemfireException(exception);
    return NanEscapeScope(NanUndefined());
  }
}

CacheablePtr gemfireValueFromV8(const Local<Value> & v8Value, const CachePtr & cachePtr) {
  NanScope();

  CacheablePtr gemfireValuePtr;

  if (v8Value->IsString() || v8Value->IsStringObject()) {
    gemfireValuePtr = CacheableString::create(wstringFromV8String(v8Value->ToString()).c_str());
  } else if (v8Value->IsBoolean()) {
    gemfireValuePtr = CacheableBoolean::create(v8Value->ToBoolean()->Value());
  } else if (v8Value->IsNumber() || v8Value->IsNumberObject()) {
    gemfireValuePtr = CacheableDouble::create(v8Value->ToNumber()->Value());
  } else if (v8Value->IsDate()) {
    uint64 millisecondsSinceEpoch = Date::Cast(*v8Value)->NumberValue();

    timeval timeSinceEpoch;
    timeSinceEpoch.tv_sec = millisecondsSinceEpoch / 1000;
    timeSinceEpoch.tv_usec = (millisecondsSinceEpoch % 1000) * 1000;

    gemfireValuePtr = CacheableDate::create(timeSinceEpoch);
  } else if (v8Value->IsArray()) {
    Local<Array> v8Array(Local<Array>::Cast(v8Value));
    unsigned int length = v8Array->Length();

    gemfireValuePtr = CacheableObjectArray::create();
    for (unsigned int i = 0; i < length; i++) {
      ((CacheableObjectArrayPtr) gemfireValuePtr)->push_back(
        gemfireValueFromV8(v8Array->Get(i), cachePtr));
    }
  } else if (v8Value->IsBooleanObject()) {
#if (NODE_MODULE_VERSION > 0x000B)
    gemfireValuePtr = CacheableBoolean::create(BooleanObject::Cast(*v8Value)->ValueOf());
#else
    gemfireValuePtr = CacheableBoolean::create(BooleanObject::Cast(*v8Value)->BooleanValue());
#endif
  } else if (v8Value->IsFunction()) {
    NanThrowError("Unable to serialize to GemFire; functions are not supported.");
    return NULLPTR;
  } else if (v8Value->IsObject()) {
    gemfireValuePtr = gemfireValueFromV8(v8Value->ToObject(), cachePtr);
  } else if (v8Value->IsUndefined()) {
    gemfireValuePtr = CacheableUndefined::create();
  } else if (v8Value->IsNull()) {
    gemfireValuePtr = NULLPTR;
  } else {
    std::string errorMessage("Unable to serialize to GemFire; unknown JavaScript object: ");
    errorMessage.append(*NanUtf8String(v8Value->ToDetailString()));
    NanThrowError(errorMessage.c_str());
    return NULLPTR;
  }

  return gemfireValuePtr;
}

void ConsoleWarn(const char * message) {
  NanScope();

  Local<Object> global(NanGetCurrentContext()->Global());
  Local<Function> warn(global->Get(NanNew("console"))->ToObject()->Get(NanNew("warn")).As<Function>());
  NanCallback callback(warn);

  static const int argc = 1;
  Local<Value> argv[argc] = { NanNew(message) };

  callback.Call(argc, argv);
}

Local<Value> v8ValueFromGemfire(const CacheablePtr & valuePtr) {
  NanEscapableScope();

  if (valuePtr == NULLPTR) {
    return NanEscapeScope(NanNull());
  }

  int typeId = valuePtr->typeId();
  if (typeId == GemfireTypeIds::CacheableASCIIString || typeId == GemfireTypeIds::CacheableASCIIStringHuge) {
    return NanEscapeScope(NanNew(((CacheableStringPtr) valuePtr)->asChar()));
  }
  if (typeId == GemfireTypeIds::CacheableString || typeId == GemfireTypeIds::CacheableStringHuge) {
    return NanEscapeScope(v8StringFromWstring(((CacheableStringPtr) valuePtr)->asWChar()));
  }
  if (typeId == GemfireTypeIds::CacheableBoolean) {
    return NanEscapeScope(NanNew(((CacheableBooleanPtr) valuePtr)->value()));
  }
  if (typeId == GemfireTypeIds::CacheableDouble) {
    return NanEscapeScope(NanNew(((CacheableDoublePtr) valuePtr)->value()));
  }
  if (typeId == GemfireTypeIds::CacheableFloat) {
    return NanEscapeScope(NanNew(((CacheableFloatPtr) valuePtr)->value()));
  }
  if (typeId == GemfireTypeIds::CacheableInt16) {
    return NanEscapeScope(NanNew(((CacheableInt16Ptr) valuePtr)->value()));
  }
  if (typeId == GemfireTypeIds::CacheableInt32) {
    return NanEscapeScope(NanNew(((CacheableInt32Ptr) valuePtr)->value()));
  }
  if (typeId == GemfireTypeIds::CacheableInt64) {
    static const int64_t maxSafeInteger = pow(2, 53) - 1;
    static const int64_t minSafeInteger = -1 * maxSafeInteger;

    int64_t value = ((CacheableInt64Ptr) valuePtr)->value();
    if (value > maxSafeInteger) {
      ConsoleWarn("Received 64 bit integer from GemFire greater than Number.MAX_SAFE_INTEGER (2^53 - 1)");
    } else if (value < minSafeInteger) {
      ConsoleWarn("Received 64 bit integer from GemFire less than Number.MIN_SAFE_INTEGER (-1 * 2^53 + 1)");
    }

    return NanEscapeScope(NanNew<Number>(value));
  }
  if (typeId == GemfireTypeIds::CacheableDate) {
    return NanEscapeScope(NanNew<Date>(
          static_cast<double>(((CacheableDatePtr) valuePtr)->milliseconds())));
  }
  if (typeId == GemfireTypeIds::CacheableUndefined) {
    return NanEscapeScope(NanUndefined());
  }
  if (typeId == GemfireTypeIds::Struct) {
    return NanEscapeScope(v8ValueFromGemfire((StructPtr) valuePtr));
  }
  if (typeId == GemfireTypeIds::CacheableObjectArray) {
    CacheableObjectArrayPtr gemfireArray(valuePtr);
    unsigned int length = gemfireArray->length();

    Local<Array> v8Array(NanNew<Array>(length));
    for (unsigned int i = 0; i < length; i++) {
      v8Array->Set(i, v8ValueFromGemfire((*gemfireArray)[i]));
    }

    return NanEscapeScope(v8Array);
  }
  if (typeId == GemfireTypeIds::CacheableVector) {
    return NanEscapeScope(v8ValueFromGemfire((CacheableVectorPtr) valuePtr));
  }
  if (typeId == 0) {
    try {
      UserFunctionExecutionExceptionPtr functionExceptionPtr =
        (UserFunctionExecutionExceptionPtr) valuePtr;

      return NanEscapeScope(NanError(gemfireExceptionMessage(functionExceptionPtr).c_str()));
    }
    catch (ClassCastException & exception) {
      // fall through to default error case
    }
  }
  if (typeId > GemfireTypeIds::CacheableStringHuge) {
    // We are assuming these are Pdx
    return NanEscapeScope(v8ValueFromGemfire((PdxInstancePtr) valuePtr));
  }

  std::stringstream errorMessageStream;
  errorMessageStream << "Unable to serialize value from GemFire; unknown typeId: " << typeId;
  NanThrowError(errorMessageStream.str().c_str());
  return NanEscapeScope(NanUndefined());
}

Local<Value> v8ValueFromGemfire(const CacheableKeyPtr & keyPtr) {
  return v8ValueFromGemfire(static_cast<CacheablePtr>(keyPtr));
}

Local<Object> v8ValueFromGemfire(const StructPtr & structPtr) {
  NanEscapableScope();

  Local<Object> v8Object(NanNew<Object>());

  unsigned int length = structPtr->length();
  for (unsigned int i = 0; i < length; i++) {
    v8Object->Set(NanNew(structPtr->getFieldName(i)),
                  v8ValueFromGemfire((*structPtr)[i]));
  }

  return NanEscapeScope(v8Object);
}

HashMapOfCacheablePtr gemfireHashMapFromV8(const Local<Object> & v8Object,
                                           const CachePtr & cachePtr) {
  NanScope();

  HashMapOfCacheablePtr hashMapPtr(new HashMapOfCacheable());

  Local<Array> v8Keys(v8Object->GetOwnPropertyNames());
  unsigned int length = v8Keys->Length();

  for (unsigned int i = 0; i < length; i++) {
    Local<Value> v8Key(v8Keys->Get(i));

    CacheablePtr keyPtr(gemfireValueFromV8(v8Key, cachePtr));
    CacheablePtr valuePtr(gemfireValueFromV8(v8Object->Get(v8Key), cachePtr));

    if (valuePtr == NULLPTR) {
      return NULLPTR;
    }

    hashMapPtr->insert(keyPtr, valuePtr);
  }

  return hashMapPtr;
}

CacheableVectorPtr gemfireVectorFromV8(const Local<Array> & v8Array, const CachePtr & cachePtr) {
  NanScope();

  unsigned int length = v8Array->Length();
  CacheableVectorPtr vectorPtr = CacheableVector::create();

  for (unsigned int i = 0; i < length; i++) {
    vectorPtr->push_back(gemfireValueFromV8(v8Array->Get(i), cachePtr));
  }

  return vectorPtr;
}

Local<Object> v8ValueFromGemfire(const HashMapOfCacheablePtr & hashMapPtr) {
  NanEscapableScope();

  Local<Object> v8Object(NanNew<Object>());

  for (HashMapOfCacheable::Iterator i = hashMapPtr->begin(); i != hashMapPtr->end(); i++) {
    CacheablePtr keyPtr(i.first());
    CacheablePtr valuePtr(i.second());

    v8Object->Set(v8ValueFromGemfire(keyPtr),
        v8ValueFromGemfire(valuePtr));
  }

  return NanEscapeScope(v8Object);
}

Local<Object> v8ValueFromGemfire(const SelectResultsPtr & selectResultsPtr) {
  NanEscapableScope();

  Local<Object> selectResults(SelectResults::NewInstance(selectResultsPtr));

  return NanEscapeScope(selectResults);
}

Local<Boolean> v8ValueFromGemfire(bool value) {
  NanEscapableScope();
  return NanEscapeScope(NanNew(value));
}

}  // namespace node_gemfire

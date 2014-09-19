#include <node.h>
#include <nan.h>
#include <v8.h>
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

std::string getClassName(const Handle<Object> & v8Object) {
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

  Local<String> v8String(NanNew(buffer));
  delete[] buffer;

  NanReturnValue(v8String);
}

PdxInstancePtr gemfireValueFromV8(const Handle<Object> & v8Object, const CachePtr & cachePtr) {
  try {
    NanScope();

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

gemfire::CacheableKeyPtr gemfireKeyFromV8(const Handle<Value> & v8Value, const CachePtr & cachePtr) {
  CacheableKeyPtr keyPtr;
  try {
    keyPtr = gemfireValueFromV8(v8Value, cachePtr);
  }
  catch(const ClassCastException & exception) {
    return NULLPTR;
  }

  return keyPtr;
}

gemfire::VectorOfCacheableKeyPtr gemfireKeysFromV8(const Handle<Array> & v8Value,
                                          const gemfire::CachePtr & cachePtr) {
  VectorOfCacheableKeyPtr vectorPtr(new VectorOfCacheableKey());

  for (unsigned int i = 0; i < v8Value->Length(); i++) {
    CacheableKeyPtr keyPtr = gemfireKeyFromV8(v8Value->Get(i), cachePtr);

    if (keyPtr != NULLPTR) {
      vectorPtr->push_back(keyPtr);
    }
  }

  return vectorPtr;
}

Handle<Value> v8ValueFromGemfire(const PdxInstancePtr & pdxInstance) {
  try {
    NanScope();

    CacheableStringArrayPtr gemfireKeys(pdxInstance->getFieldNames());

    if (gemfireKeys == NULLPTR) {
      NanReturnValue(NanNew<Object>());
    }

    Local<Object> v8Object(NanNew<Object>());
    int length = gemfireKeys->length();

    for (int i = 0; i < length; i++) {
      const char * key = gemfireKeys[i]->asChar();

      CacheablePtr value;
      try {
        pdxInstance->getField(key, value);
      }
      catch(const gemfire::IllegalStateException & exception) {
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
    Handle<Array> v8Array(Handle<Array>::Cast(v8Value));
    unsigned int length = v8Array->Length();

    gemfireValuePtr = CacheableObjectArray::create();
    for (unsigned int i = 0; i < length; i++) {
      ((CacheableObjectArrayPtr) gemfireValuePtr)->push_back(
        gemfireValueFromV8(v8Array->Get(i), cachePtr));
    }
  } else if (v8Value->IsObject()) {
    gemfireValuePtr = gemfireValueFromV8(v8Value->ToObject(), cachePtr);
  } else if (v8Value->IsUndefined()) {
    gemfireValuePtr = CacheableUndefined::create();
  } else if (v8Value->IsNull()) {
    gemfireValuePtr = NULLPTR;
  } else {
    std::string errorMessage("Unable to serialize value to GemFire; unknown JavaScript object: ");
    errorMessage.append(*NanUtf8String(v8Value->ToDetailString()));
    NanThrowError(errorMessage.c_str());
    return NULLPTR;
  }

  return gemfireValuePtr;
}

Handle<Value> v8ValueFromGemfire(const CacheablePtr & valuePtr) {
  NanScope();

  if (valuePtr == NULLPTR) {
    NanReturnNull();
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
    NanReturnUndefined();
  }
  if (typeId == GemfireTypeIds::Struct) {
    NanReturnValue(v8ValueFromGemfire((StructPtr) valuePtr));
  }
  if (typeId == GemfireTypeIds::CacheableObjectArray) {
    CacheableObjectArrayPtr gemfireArray(valuePtr);
    unsigned int length = gemfireArray->length();

    Handle<Array> v8Array(NanNew<Array>(length));
    for (unsigned int i = 0; i < length; i++) {
      v8Array->Set(i, v8ValueFromGemfire((*gemfireArray)[i]));
    }

    NanReturnValue(v8Array);
  }
  if (typeId == GemfireTypeIds::CacheableVector) {
    NanReturnValue(v8ValueFromGemfire((CacheableVectorPtr) valuePtr));
  }
  if (typeId == 0) {
    try {
      UserFunctionExecutionExceptionPtr functionExceptionPtr =
        (UserFunctionExecutionExceptionPtr) valuePtr;

      NanReturnValue(NanError(gemfireExceptionMessage(functionExceptionPtr).c_str()));
    }
    catch (ClassCastException & exception) {
      // fall through to default error case
    }
  }
  if (typeId > GemfireTypeIds::CacheableStringHuge) {
    // We are assuming these are Pdx
    NanReturnValue(v8ValueFromGemfire((PdxInstancePtr) valuePtr));
  }

  std::stringstream errorMessageStream;
  errorMessageStream << "Unable to serialize value from GemFire; unknown typeId: " << typeId;
  NanThrowError(errorMessageStream.str().c_str());
  NanReturnUndefined();
}

Handle<Object> v8ValueFromGemfire(const gemfire::StructPtr & structPtr) {
  NanScope();

  Local<Object> v8Object(NanNew<Object>());

  unsigned int length = structPtr->length();
  for (unsigned int i = 0; i < length; i++) {
    v8Object->Set(NanNew(structPtr->getFieldName(i)),
                  v8ValueFromGemfire((*structPtr)[i]));
  }

  NanReturnValue(v8Object);
}

gemfire::HashMapOfCacheablePtr gemfireHashMapFromV8(const Handle<Object> & v8Object,
                                           const gemfire::CachePtr & cachePtr) {
  HashMapOfCacheablePtr hashMapPtr(new HashMapOfCacheable());

  Local<Array> v8Keys(v8Object->GetOwnPropertyNames());
  unsigned int length = v8Keys->Length();

  for (unsigned int i = 0; i < length; i++) {
    Handle<Value> v8Key(v8Keys->Get(i));

    CacheablePtr keyPtr(gemfireValueFromV8(v8Key, cachePtr));
    CacheablePtr valuePtr(gemfireValueFromV8(v8Object->Get(v8Key), cachePtr));

    hashMapPtr->insert(keyPtr, valuePtr);
  }

  return hashMapPtr;
}

Handle<Object> v8ValueFromGemfire(const gemfire::HashMapOfCacheablePtr & hashMapPtr) {
  NanScope();

  Handle<Object> v8Object(NanNew<Object>());

  for (HashMapOfCacheable::Iterator i = hashMapPtr->begin(); i != hashMapPtr->end(); i++) {
    CacheablePtr keyPtr(i.first());
    CacheablePtr valuePtr(i.second());

    v8Object->Set(v8ValueFromGemfire(keyPtr),
        v8ValueFromGemfire(valuePtr));
  }

  NanReturnValue(v8Object);
}

Handle<Object> v8ValueFromGemfire(const SelectResultsPtr & selectResultsPtr) {
  NanScope();

  Handle<Object> selectResults(SelectResults::NewInstance(selectResultsPtr));

  NanReturnValue(selectResults);
}

Handle<Array> v8ValueFromGemfire(const gemfire::CacheableVectorPtr & vectorPtr) {
  NanScope();

  unsigned int length = vectorPtr->size();

  Local<Array> array(NanNew<Array>(length));
  for (unsigned int i = 0; i < length; i++) {
    array->Set(i, v8ValueFromGemfire((*vectorPtr)[i]));
  }

  NanReturnValue(array);
}

Handle<Boolean> v8ValueFromGemfire(bool value) {
  NanScope();
  NanReturnValue(NanNew(value));
}

}  // namespace node_gemfire

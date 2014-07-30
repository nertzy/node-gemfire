#ifndef __GEMFIRE_PDXSERIALIZABLE_H__
#define __GEMFIRE_PDXSERIALIZABLE_H__

/*=========================================================================
* Copyright (c) 2002-2014 Pivotal Software, Inc. All Rights Reserved.
 * This product is protected by U.S. and international copyright
 * and intellectual property laws. Pivotal products are covered by
 * more patents listed at http://www.gopivotal.com/patents.
*========================================================================
*/

#include "CacheableKey.hpp"

namespace gemfire
{

typedef PdxSerializable* ( *PdxTypeFactoryMethod ) ();

class CPPCACHE_EXPORT  PdxSerializable : public CacheableKey
{

public:

  PdxSerializable();
  virtual ~PdxSerializable();

  /**
   *@brief serialize this object in gemfire PDX format
   *@param PdxWriter to serialize the PDX object
   **/
  virtual void toData( PdxWriterPtr output ) /*const*/ = 0;

  /**
   *@brief Deserialize this object
   *@param PdxReader to Deserialize the PDX object
   **/
  virtual void fromData( PdxReaderPtr input ) = 0;

  /**
   *@brief return the typeId byte of the instance being serialized.
   * This is used by deserialization to determine what instance
   * type to create and deserialize into.
   *
   * Note that this should not be overridden by custom implementations
   * and is reserved only for builtin types.
   */
  virtual int8_t typeId( ) const;

  /** return true if this key matches other. */
  virtual bool operator==( const CacheableKey& other ) const;

  /** return the hashcode for this key. */
  virtual uint32_t hashcode( ) const;

  /**
   *@brief serialize this object
   **/
  virtual void toData( DataOutput& output ) const;

  /**
   *@brief deserialize this object, typical implementation should return
   * the 'this' pointer.
   **/
  virtual Serializable* fromData( DataInput& input );

  /**
   *@brief return the classId of the instance being serialized.
   * This is used by deserialization to determine what instance
   * type to create and derserialize into.
   */
  virtual int32_t classId( ) const { return 0x10; }

  /**
   * Display this object as 'string', which depends on the implementation in
   * the subclasses.
   * The default implementation renders the classname.
   *
   * The return value may be a temporary, so the caller has to ensure that
   * the SharedPtr count does not go down to zero by storing the result
   * in a variable or otherwise.
   */
  virtual CacheableStringPtr toString() const;

  /**
   * Get the Type for the Object. Equivalent to the C# Type->GetType() API.
   */
  virtual const char* getClassName()const = 0;
};
}

#endif /* PDXSERIALIZABLE_HPP_ */

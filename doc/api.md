# API Documentation

## Cache

A GemFire cache is an in-memory data store composed of many Regions. A cache is constructed with an XML configuration file:

```javascript
var gemfire = require('gemfire');
var cache = new gemfire.Cache('config/gemfire.xml');
```

For more information on cache configuration files, see [the documentation](http://gemfire.docs.pivotal.io/latest/userguide/gemfire_nativeclient/cache-init-file/chapter-overview.html#chapter-overview).

### cache.createRegion(regionName, options)

Adds a region to the GemFire cache. Once the region is created, it will remain in the client for the lifetime of the process. The `regionName` should be a string and the `options` object has a required type property. 

 * `options.type`: the type of GemFire region to create. The value should be the string name of one of the GemFire region shortcuts, such as "LOCAL", "PROXY", or "CACHING_PROXY". See the GemFire documentation for [Region Shortcuts](http://gemfire.docs.pivotal.io/latest/userguide/gemfire_nativeclient/client-cache/region-shortcuts.html) and the [gemfire::RegionShortcut C++ enumeration](http://gemfire.docs.pivotal.io/latest/cpp_api/cppdocs/namespacegemfire.html#596bc5edab9d1e7c232e53286b338183) for more details.  

Example:
```javascript
cache.getRegion("myRegion") // returns undefined

var myRegion = cache.createRegion("myRegion", {type: "LOCAL"});

cache.getRegion("myRegion") // returns the same region as myRegion
```

### cache.executeFunction(functionName, options)

Executes a Java function on a server in the cluster containing the cache. `functionName` is the full Java class name of the function that will be called. Options may be either an array of arguments, or an options object containing the optional field `arguments`.

 * `options.arguments`: the arguments to be passed to the Java function

> **Note**: Unlike region.executeFunction(), `options.filter` is not allowed.

cache.executeFunction returns an EventEmitter which emits the following events:

 * `data`: Emitted once for each result sent by the Java function.
 * `error`: Emitted if the function throws or returns an Exception.
 * `end`: Called after the Java function has finally returned.

> **Warning:** As of GemFire 8.0.0.0, there are some situations where the Java function can throw an uncaught Exception, but the node `error` callback never gets called. This is due to a known bug in how the GemFire 8.0.0.0 Native Client handles exceptions. This bug is only present for cache.executeFunction. region.executeFunction works as expected.

> **Warning:** Due to a workaround for a bug in Gemfire 8.0.0.0, all functions executed by cache.executeFunction() will be executed on exactly one server in the first pool defined in the XML configuration file.

Example:
```javascript
cache.executeFunction("com.example.FunctionName", 
    { arguments: [1, 2, 3] }
  )
  .on("error", function(error) { throw error; })
  .on("data", function(result) {
    // ...
  })
  .on("end", function() {
    // ...
  });
```

For more information, please see the [GemFire documentation for Function Execution](http://gemfire.docs.pivotal.io/latest/userguide/developing/function_exec/chapter_overview.html).

### cache.executeFunction(functionName, arguments)

Shorthand for `executeFunction` with an array of arguments. Equivalent to:

```javascript
cache.executeFunction(functionName, { arguments: arguments })
```

### cache.executeQuery(query, callback)

Executes an OQL query on the cluster. The callback will be called with an `error` argument and a `response` argument. The `response` argument is an object responding to `toArray` and `each`.

 * `results.toArray()`: Return the entire result set as an Array.
 * `results.each(callback)`: Call the callback with a `result` argument, once for each result.

Example:
```javascript
cache.executeQuery("SELECT DISTINCT * FROM /exampleRegion", function(error, response) {
  if(error) { throw error; }
  
  var results = response.toArray();
  // allResults could now be this:
  //   [ 'value1', 'value2', { foo: 'bar' } ]
  
  // alternately, you could use the `each` iterator:
  response.each(function(result) {
  	// this callback will be called with 'value1', then 'value2', then { foo: 'bar' }
  });
}
```

For more information on OQL, see [the documentation](http://gemfire.docs.pivotal.io/latest/userguide/developing/querying_basics/chapter_overview.html).

### cache.getRegion(regionName)

Retrieves a Region from the Cache. An error will be thrown if the region is not present.

Example:
```javascript
var region = cache.getRegion('exampleRegion');
```

### cache.rootRegions()

Retrieves an array of all root Regions from the Cache. 

Example:
```javascript
var regions = cache.rootRegions();
// if there are three Regions defined in your cache, regions could now be:
// [firstRegionName, secondRegionName, thirdRegionName]
```

## Region

A GemFire region is a collection of key-value pair entries. The most common way to get a region is to call `cache.getRegion('regionName')` on a Cache.

### region.attributes

Returns an object describing the attributes of the GemFire region. This can be useful for debugging your region configuration.

Several of these values are described in the [GemFire documentation for region attributes](http://gemfire.docs.pivotal.io/latest/userguide/gemfire_nativeclient/client-cache/region-attributes.html).

Example output of `region.attributes`:
```javascript
{
  cachingEnabled: true,
  clientNotificationEnabled: true,
  concurrencyChecksEnabled: true,
  concurrencyLevel: 16,
  diskPolicy: 'none',
  entryIdleTimeout: 0,
  entryTimeToLive: 0,
  initialCapacity: 10000,
  loadFactor: 0.75,
  lruEntriesLimit: 0,
  lruEvicationAction: 'LOCAL_DESTROY',
  poolName: 'default_gemfireClientPool',
  regionIdleTimeout: 0,
  regionTimeToLive: 0,
  scope: 'DISTRIBUTED_NO_ACK'
}
```

### region.clear([callback])

Removes all entries from the region. The callback will be called with an `error` argument. If the callback is not supplied, and an error occurs, the region will emit an `error` event.

Example:
```javascript
region.clear(function(error){
  if(error) { throw error; }
  // region is empty
});
```

### region.executeFunction(functionName, options)

Executes a Java function on any servers in the cluster containing the region. `functionName` is the full Java class name of the function that will be called. Options may be either an array of arguments, or an options object containing the optional fields `arguments` and `filters`.

 * `options.arguments`: the arguments to be passed to the Java function
 * `options.filter`: an array of keys to be sent to the Java function as the filter. 

region.executeFunction returns an EventEmitter which emits the following events:

 * `data`: Emitted once for each result sent by the Java function.
 * `error`: Emitted if the function throws or returns an Exception.
 * `end`: Called after the Java function has finally returned.

Example:
```javascript
region.executeFunction("com.example.FunctionName", 
    { arguments: [1, 2, 3], filters: ["key1", "key2"] }
  )
  .on("error", function(error) { throw error; })
  .on("data", function(result) {
    // ...
  })
  .on("end", function() {
    // ...
  });
```

For more information, please see the [GemFire documentation for Function Execution](http://gemfire.docs.pivotal.io/latest/userguide/developing/function_exec/chapter_overview.html).

### region.executeFunction(functionName, arguments)

Shorthand for `executeFunction` with an array of arguments. Equivalent to:

```javascript
region.executeFunction(functionName, { arguments: arguments })
```

### region.existsValue(predicate, callback)

Indicates whether or not a value matching the OQL predicate `predicate` is present in the region. The callback will be called with an `error` and the boolean `response`.

In the predicate, you can use `this` to refer to the entire value. If you use any other expression, it will be treated as a field name on object values.

Example:
```javascript
region.existsValue("this > 'value1'", function(error, response) {
  if(error) { throw error; }
  // response will be set to true if any value is the string 'value1'
});

region.existsValue("foo > 2", function(error, response) {
  if(error) { throw error; }
  // response will be set to true if any value is an object such
  // as { foo: 3 } where the value of foo is greater than 2.
});
```

See also `region.query` and `region.selectValue`.

### region.get(key, callback)

Retrieves the value of an entry in the Region. The callback will be called with an `error` and the `value`. If the key is not present in the Region, an error will be passed to the callback.

Example:
```javascript
region.get("key", function(error, value){
  if(error) { throw error; }
  // an entry was found and its value is now accessible
});
```

### region.getAll(keys, callback)

Retrieves the values of multiple keys in the Region. The keys should be passed in as an `Array`. The callback will be called with an `error` and a `values` object. If one or more keys are not present in the region, their values will be returned as null.

Example:
```javascript
region.getAll(["key1", "key2", "unknownKey"], function(error, values){
  if(error) { throw error; }
  // if key1 and key2 are present in the region, but unknownKey is not,
  // then values may look like this:
  // { key1: 'value1', key2: { foo: 'bar' }, unknownKey: null }
});
```

### region.keys(callback)

Retrieves all keys in the Region. The callback will be called with an `error` argument, and an Array of keys.

Example:
```javascript
region.keys(function(error, keys) {
  if(error) { throw error; }
  // keys is now an array of all keys in the region, for example:
  //   [ 'key1', 'key2', 'key3' ]
});
```

### region.name

Returns the name of the region.

### region.put(key, value, [callback])

Stores an entry in the region. The callback will be called with an `error` argument. If the callback is not supplied, an an error occurs, the region will emit an `error` event.

GemFire supports most JavaScript types for the value. Some types, such as `Function` and `null` cannot be stored. 

GemFire supports several JavaScript types for the key, but the safest choice is to always use a `String`.

Example:
```javascript
region.put('key', { foo: 'bar' }, function(error) {
  if(error) { throw error; }
  // the entry at key "key" now has value { foo: 'bar' }
});
```

### region.putAll(entries, [callback])

Stores multiple entries in the region. The callback will be called with an `error` argument. If the callback is not supplied, and an error occurs, the Region will emit an `error` event.

**NOTE**: Keys on a JavaScript object are always strings. Thus, all entries will have string keys.

Example:
```javascript
region.putAll(
  {
    key1: 'value1', 
    key2: { foo: 'bar' } 
    3: "three"
  }, 
  function(error) {
    if(error) { throw error; }
    // the entry at key "key1" now has value "value1"
    // the entry at key "key2" now has value { foo: 'bar' }
    // the entry at key 3 (Number) is unaffected
    // the entry at key "3" (String) now has value "three"
  }
);
```

### region.query(predicate, callback)

Retrieves all values from the Region matching the OQL `predicate`. The callback will be called with an `error` argument, and a `response` object. For more information on `response` objects, please see `cache.executeQuery`.

Example:
```javascript
region.query("this like '% Smith'", function(error, response) {
  if(error) { throw error; }
  
  var results = response.toArray();
  // results will now be an array of all values in the region ending in " Smith"
  
  response.each(function(result) {
    // this callback will be passed each value in the region ending in " Smith", one at a time
  });
});
```

See also `region.selectValue` and `region.existsValue`.

### region.registerAllKeys()

Tells the GemFire server to trigger events for entry operations that were triggered by other clients in the system. By default, region entry operations (`region.put`, `region.remove`, etc.) that happen within a single Node process trigger events *only* within that same process. After calling `region.registerAllKeys`, all entry operations on the region will trigger events. In other words, the GemFire server will push notifications back to the Node process.

Example:
```javascript
region.on("create", function(event) {
  // handle event
});

// another client creates an entry in the region, and the callback is not triggered

region.registerAllKeys();

// another client creates an entry in the region, and the callback is triggered
```

See also Events and `region.unregisterAllKeys`.

### region.remove(key, [callback])

Removes the entry specified by the indicated key from the Region, or, if no such entry is present, passes an `error` to the callback. If the argument is not supplied, and an error occurs, the Region will emit an `error` event.

Example:
```javascript
region.remove('key1', function(error) {
  if(error) { throw error; }
  // the entry with key 'key1' has been removed from the region
});
```

### region.selectValue(predicate, callback)

Retrieves exactly one entry from the Region matching the OQL `predicate`. The callback will be called with an `error` argument, and a `result`.

If more than one result matches the predicate, an error will be passed to the callback. If you want to select multiple results for a predicate, see `region.query`.

Example:
```javascript
region.selectValue("this = 'value1'", function(error, result) {
  if(error) { throw error; }
  // if there is exactly one entry with value 'value1' in the region,
  // result will now be set to 'value1'
});
```

See also `region.query` and `region.existsValue`.

### region.unregisterAllKeys()

Tells the GemFire server *not* to trigger events for entry operations that were triggered by other clients in the system. 

Example:
```javascript
region.registerAllKeys();

region.on("create", function(event) {
  // handle event
}

// another client creates an entry in the region, and the callback is triggered

region.unregisterAllKeys();

// another client creates an entry in the region, and the callback is not triggered
```

See also Events and `region.registerAllKeys`.

### Event: 'error'

* error: `Error` object.

Emitted when an error occurs and no callback was passed to the method that caused the error.

Example:
```javascript
region.on("error", function(error) {
  // handle errors
});

// emits an error because null is not a supported value for put()
region.put("foo", null);

// does not emit an error, because a callback was passed
region.put("foo", null, function(error) {});
```

### Event: 'create'

* event: GemFire event payload object.
  * event.key: The key that was inserted.
  * event.oldValue: Always `null` because there is no old value.
  * event.newValue: The value that was inserted.

Emitted when an entry is added to the region. Not emitted when an existing entry's value is updated.

Example:
```javascript
region.on("create", function(event) {
  // process event
});

// emits an event because "foo" is a new entry in the region
region.put("foo", "bar");

// does not emit an event because "foo" is already an entry in the region
region.put("foo", "baz");
```

See also `region.registerAllKeys`.

### Event: 'destroy'

* event: GemFire event payload object.
  * event.key: The key that was destroyed.
  * event.oldValue: The previous value before the destruction.
  * event.newValue: Always `null` because there is no new value.

Emitted when an existing entry is destroyed.

Example:
```javascript
region.on("destroy", function(event) {
  // process event
});

region.remove("foo");
```

See also `region.registerAllKeys`.

### Event: 'update'

* event: GemFire event payload object.
  * event.key: The key that was updated.
  * event.oldValue: The previous value before the update.
  * event.newValue: The new value after the update.

Emitted when an existing entry's value is updated. Not emitted when an entry is added to the region.

Example:
```javascript
region.on("update", function(event) {
  // process event
});

// does not emit an event because "foo" is a new entry in the region
region.put("foo", "bar");

// emits an event because "foo" is already an entry in the region
region.put("foo", "baz");
```

See also `region.registerAllKeys`.

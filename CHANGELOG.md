# v0.0.11
- Add `poolName` option to cache.createRegion()
- **Breaking Change**: cache.executeFunction and cache.executeQuery renamed their options from `pool` to `poolName` to be consistent with the rest of the API.

# v0.0.10
- Add ability to target a particular GemFire pool when executing a function using `cache.executeQuery`

# v0.0.9
- Add `gemfire.connected()`

# v0.0.8

- **Breaking Change**: `new gemfire.Cache()` constructor removed, replaced with `gemfire.configure()` and `gemfire.getCache()`
- **Breaking Change**: JavaScript Arrays are now sent to GemFire as the Java type `ArrayList` instead of `Object[]`.
- Add `cache.createRegion()`
- Add `region.destroyRegion()` and `region.localDestroyRegion()`
- Add ability to target a particular GemFire pool when executing a function using `cache.executeFunction`
- Add `region.attributes`

# v0.0.7

- Require GemFire Native Client 8.0.0.1 or later
- Add workaround for issue where `cache.executeFunction()` throws `gemfire::NullPointerException` if no regions have been defined in the XML configuration.
- Fix issue where arrays would sometimes come back from GemFire as corrupted non-array objects.

# v0.0.6

- Adjust workaround for the issue where arrays could not be consistently round-tripped when attached to an object retrieved from GemFire. Note that this workaround, while more reliable, likely degrades performance and most likely won't be necessary when the next release of GemFire comes out.

# v0.0.5

- Fix issue where some arrays would throw `gemfire::OutOfBoundsException` when attached to an object retrieved from GemFire.

# v0.0.4

- Fix issue where `region.putAll({"1": "one"}, callback)` would incorrectly use Number keys instead of String keys.

# v0.0.3

- Initial Cache API
- Initial Region API

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

# v0.0.5

- Fix issue where some arrays would throw `gemfire::OutOfBoundsException` when attached to an object retrieved from GemFire.

# v0.0.4

- Fix issue where `region.putAll({"1": "one"}, callback)` would incorrectly use Number keys instead of String keys.

# v0.0.3

- Initial Cache API
- Initial Region API

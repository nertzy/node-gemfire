const _ = require("lodash");

const cache = require("../factories.js").getCache();

cache.close();

if (!_.isUndefined(cache.getRegion('exampleRegion'))) {
  throw("cache.getRegion did not return undefined after the cache was closed.");
}

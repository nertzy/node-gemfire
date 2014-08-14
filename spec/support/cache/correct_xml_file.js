var cache = require("../factories.js").getCache();

if(!cache.getRegion('exampleRegion')) {
  throw("Region not found");
}

const _ = require("lodash");
const util = require("util");

const cache = require("../factories.js").getCache();

if(_.isEmpty(cache.rootRegions())) {
  throw new Error("cache.rootRegions didn't return regions before cache.close");
}

cache.close();

function expectErrorMessage(error, expectedMessage) {
  if(!error.message.match(expectedMessage)) {
    throw new Error(
      "Error message didn't match expectation:\n" +
      "Expected: " + util.inspect(expectedMessage) + "\n" +
      "  Actual: " + util.inspect(error.message)
    );
  }
}

try {
  cache.executeQuery("SELECT * FROM /exampleRegion", function(){});
  throw new Error("cache.executeQuery did not throw an exception after cache.close");
} catch (error) {
  expectErrorMessage(error, "Cannot execute query; cache is closed.");
}

if (!_.isUndefined(cache.getRegion('exampleRegion'))) {
  throw("cache.getRegion did not return undefined after the cache was closed.");
}

if(!_.isEmpty(cache.rootRegions())) {
  throw new Error("cache.rootRegions didn't return an empty array after cache.close");
}

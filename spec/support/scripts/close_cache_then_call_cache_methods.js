const _ = require("lodash");
const expectErrorMessage = require("./expect_error_message.js");

const cache = require("../factories.js").getCache();

if(_.isEmpty(cache.rootRegions())) {
  throw new Error("cache.rootRegions didn't return regions before cache.close");
}

cache.close();

try {
  cache.executeQuery("SELECT * FROM /exampleRegion", function(){});
  throw new Error("cache.executeQuery did not throw an exception after cache.close");
} catch (error) {
  expectErrorMessage(error, "Cannot execute query; cache is closed.");
}

try {
  cache.executeFunction("io.pivotal.node_gemfire.TestFunction", function(){});
  throw new Error("cache.executeFunction did not throw an exception after cache.close");
} catch (error) {
  expectErrorMessage(error, "Cannot execute function; cache is closed.");
}

if (!_.isUndefined(cache.getRegion('exampleRegion'))) {
  throw("cache.getRegion did not return undefined after the cache was closed.");
}

if(!_.isEmpty(cache.rootRegions())) {
  throw new Error("cache.rootRegions didn't return an empty array after cache.close");
}

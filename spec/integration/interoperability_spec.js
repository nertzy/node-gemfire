const errorMatchers = require("../support/error_matchers.js");

const cache = require("../support/factories.js").getCache();
const region = cache.getRegion("exampleRegion");

describe("Interoperability", function() {
  it("interprets Java null as JavaScript null", function(done) {
    this.addMatchers(errorMatchers);
    region.executeFunction("io.pivotal.node_gemfire.ReturnNull", function(error, response) {
      expect(error).not.toBeError();
      expect(response).toEqual([null]);
      done();
    });
  });
});

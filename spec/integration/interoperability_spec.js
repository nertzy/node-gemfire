const errorMatchers = require("../support/error_matchers.js");

const cache = require("../support/factories.js").getCache();
const region = cache.getRegion("exampleRegion");

describe("Interoperability", function() {
  beforeEach(function() {
    this.addMatchers(errorMatchers);
  });

  it("interprets Java null as JavaScript null", function(done) {
    region.executeFunction("io.pivotal.node_gemfire.ReturnNull", function(error, response) {
      expect(error).not.toBeError();
      expect(response).toEqual([null]);
      done();
    });
  });

  it("interprets Java float as JavaScript Number", function(done) {
    region.executeFunction("io.pivotal.node_gemfire.ReturnFloat", function(error, response) {
      expect(error).not.toBeError();
      expect(response).toEqual([1.0]);
      done();
    });
  });
});

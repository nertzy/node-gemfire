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

  it("interprets Java integer as JavaScript Number", function(done) {
    region.executeFunction("io.pivotal.node_gemfire.ReturnInteger", function(error, response) {
      expect(error).not.toBeError();
      expect(response).toEqual([1]);
      done();
    });
  });

  it("interprets Java short as JavaScript Number", function(done) {
    region.executeFunction("io.pivotal.node_gemfire.ReturnShort", function(error, response) {
      expect(error).not.toBeError();
      expect(response).toEqual([1]);
      done();
    });
  });

  it("interprets Java long as JavaScript Number", function(done) {
    region.executeFunction("io.pivotal.node_gemfire.ReturnLong", function(error, response) {
      expect(error).not.toBeError();
      expect(response).toEqual([1]);
      done();
    });
  });

  it("interprets Java Set as JavaScript Array", function(done) {
    region.executeFunction("io.pivotal.node_gemfire.ReturnSet", function(error, response) {
      expect(error).not.toBeError();
      expect(response).toBeTruthy();

      const array = response[0];
      expect(array.length).toEqual(2);
      expect(array).toContain("foo");
      expect(array).toContain("bar");
      done();
    });
  });

  it("provides a warning when a Java long greater than Number.MAX_SAFE_INTEGER is received", function(done) {
    spyOn(console, "warn");
    region.executeFunction("io.pivotal.node_gemfire.ReturnPositiveAmbiguousLong", function(error, response) {
      expect(error).not.toBeError();
      expect(response).toEqual([Math.pow(2, 53)]);
      expect(console.warn).toHaveBeenCalledWith(
        "Received 64 bit integer from GemFire greater than Number.MAX_SAFE_INTEGER (2^53 - 1)"
      );
      done();
    });
  });

  it("provides a warning when a Java long less than Number.MIN_SAFE_INTEGER is received", function(done) {
    spyOn(console, "warn");
    region.executeFunction("io.pivotal.node_gemfire.ReturnNegativeAmbiguousLong", function(error, response) {
      expect(error).not.toBeError();
      expect(response).toEqual([-1 * Math.pow(2, 53)]);
      expect(console.warn).toHaveBeenCalledWith(
        "Received 64 bit integer from GemFire less than Number.MIN_SAFE_INTEGER (-1 * 2^53 + 1)"
      );
      done();
    });
  });
});

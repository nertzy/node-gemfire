const async = require('async');
const errorMatchers = require("../support/error_matchers.js");

const cache = require("../support/factories.js").getCache();
const region = cache.getRegion("exampleRegion");

describe("Interoperability", function() {
  beforeEach(function() {
    jasmine.addMatchers(errorMatchers);
  });

  function expectFunctionToReturn(functionName, returnValue, done) {
    const dataCallback = jasmine.createSpy("dataCallback");
    region
      .executeFunction(functionName)
      .on("data", dataCallback)
      .on("end", function(){
        expect(dataCallback.calls.count()).toEqual(1);
        expect(dataCallback).toHaveBeenCalledWith(returnValue);
        done();
      });
  }

  it("interprets Java null as JavaScript null", function(done) {
    expectFunctionToReturn("io.pivotal.node_gemfire.ReturnNull", null, done);
  });

  it("interprets Java float as JavaScript Number", function(done) {
    expectFunctionToReturn("io.pivotal.node_gemfire.ReturnFloat", 1.0, done);
  });

  it("interprets Java integer as JavaScript Number", function(done) {
    expectFunctionToReturn("io.pivotal.node_gemfire.ReturnInteger", 1, done);
  });

  it("interprets Java short as JavaScript Number", function(done) {
    expectFunctionToReturn("io.pivotal.node_gemfire.ReturnShort", 1, done);
  });

  it("interprets Java long as JavaScript Number", function(done) {
    expectFunctionToReturn("io.pivotal.node_gemfire.ReturnLong", 1, done);
  });

  it("interprets Java HashMap as JavaScript Object", function(done) {
    expectFunctionToReturn("io.pivotal.node_gemfire.ReturnHashMap", { foo: 'bar' }, done);
  });

  it("interprets Java Date as JavaScript Date", function(done) {
    const millisSinceEpoch = Date.parse("2001-07-04 12:34:56.789");
    const date = new Date(millisSinceEpoch);
    expectFunctionToReturn("io.pivotal.node_gemfire.ReturnDate", date, done);
  });

  it("interprets Java Set as JavaScript Array", function(done) {
    const dataCallback = jasmine.createSpy("dataCallback");
    region
      .executeFunction("io.pivotal.node_gemfire.ReturnSet")
      .on("data", dataCallback)
      .on("end", function(){
        expect(dataCallback.calls.count()).toEqual(1);

        const args = dataCallback.calls.argsFor(0);
        const array = args[0];
        expect(array.length).toEqual(2);
        expect(array).toContain("foo");
        expect(array).toContain("bar");
        done();
      });
  });

  it("interprets Java List as JavaScript Array", function(done) {
    expectFunctionToReturn("io.pivotal.node_gemfire.ReturnList",
                           ["foo", "bar", 1, 2.2, [3, 4.4]],
                           done);
  });

  it("interprets Java Object[] as JavaScript Array", function(done) {
    expectFunctionToReturn("io.pivotal.node_gemfire.ReturnObjectArray",
                           ["foo", "bar", 1, 2.2, [3, 4.4]],
                           done);
  });

  // see https://www.pivotaltracker.com/story/show/81671186
  it("interprets Object[] that does not throw OutOfRangeException", function(done) {
    expectFunctionToReturn("io.pivotal.node_gemfire.ReturnObjectArrayTriggersError",
                           { someArray: [
                            '000000000000000000000000',
                            '000000000000000000000000',
                            '000000000000000000000000',
                            '000000000000000000000000',
                            '000000000000000000000000',
                            '000000000000000000000000',
                            '000000000000000000000000',
                            '000000000000000000000000',
                            '000000000000000000000000',
                            '000000000000000000000000',
                            '000000000000000000000000',
                          ]}, done);
  });

  it("provides a warning when a Java long greater than Number.MAX_SAFE_INTEGER is received", function(done) {
    spyOn(console, "warn");

    async.series([
      function(next) {
        expectFunctionToReturn("io.pivotal.node_gemfire.ReturnPositiveAmbiguousLong",
                               Math.pow(2, 53),
                               next);
      },
      function(next) {
        expect(console.warn).toHaveBeenCalledWith(
          "Received 64 bit integer from GemFire greater than Number.MAX_SAFE_INTEGER (2^53 - 1)"
        );
        next();
      }
    ], done);
  });

  it("provides a warning when a Java long less than Number.MIN_SAFE_INTEGER is received", function(done) {
    spyOn(console, "warn");

    async.series([
      function(next) {
        expectFunctionToReturn("io.pivotal.node_gemfire.ReturnNegativeAmbiguousLong",
                               -Math.pow(2, 53),
                               next);
      },
      function(next) {
        expect(console.warn).toHaveBeenCalledWith(
          "Received 64 bit integer from GemFire less than Number.MIN_SAFE_INTEGER (-1 * 2^53 + 1)"
        );
        next();
      }
    ], done);
  });
});

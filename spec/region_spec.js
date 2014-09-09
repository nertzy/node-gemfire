const childProcess = require('child_process');
const factories = require('./support/factories.js');
const _ = require("lodash");
const util = require("util");
const async = require('async');

const invalidKeys = [null, undefined, []];

describe("gemfire.Region", function() {
  var region, cache;

  beforeEach(function() {
    cache = factories.getCache();
    region = cache.getRegion("exampleRegion");
    region.clear();
  });

  afterEach(function(done) {
    setTimeout(done, 0);
  });

  describe(".get", function() {
    it("throws an error if a key is not passed to .get", function() {
      function getWithoutKey() {
        region.get();
      }
      expect(getWithoutKey).toThrow("You must pass a key and callback to get().");
    });

    it("throws an error if a callback is not passed to .get", function() {
      function getWithoutCallback() {
        region.get("foo");
      }

      expect(getWithoutCallback).toThrow("You must pass a callback to get().");
    });

    it("throws an error if a non-function is passed as a callback", function() {
      function getWithNonFunctionCallback() {
        region.get("foo", "bar");
      }

      expect(getWithNonFunctionCallback).toThrow("The second argument to get() must be a callback.");
    });

    it("returns the region object to support chaining", function(done) {
      var returnValue = region.get("foo", function(error, value) {
        done();
      });

      expect(returnValue).toEqual(region);
    });

    it("passes an error to the callback when called for a nonexistent key", function(done) {
      region.get("baz", function(error, value) {
        expect(error).toBeTruthy();
        expect(error.message).toEqual("Key not found in region.");
        expect(value).toBeUndefined();
        done();
      });
    });

    _.each(invalidKeys, function(invalidKey) {
      it("passes an error to the callback when passed the invalid key " + util.inspect(invalidKey), function(done) {
        region.get(invalidKey, function(error, value) {
          expect(error).toBeTruthy();
          expect(error.message).toEqual("Invalid GemFire key.");
          expect(value).toBeUndefined();
          done();
        });
      });
    });

    it("passes the value into the callback", function(done) {
      region.put('foo', 'bar', function (error) {
        expect(error).toBeFalsy();
        region.get("foo", function(error, value) {
          expect(error).toBeFalsy();
          expect(value).toEqual("bar");
          done();
        });
      });
    });
  });

  describe(".put", function() {
    it("throws an error when no key is passed", function() {
      function putWithNoArgs() {
        region.put();
      }

      expect(putWithNoArgs).toThrow("You must pass a key, value, and callback to put().");
    });

    it("throws an error when no value is passed", function() {
      function putWithOnlyKey() {
        region.put('foo');
      }

      expect(putWithOnlyKey).toThrow("You must pass a key, value, and callback to put().");

    });

    it("throws an error if a non-function is passed as the callback", function() {
      function putWithNonFunctionCallback() {
        region.put('foo', 'bar', 'not a function');
      }

      expect(putWithNonFunctionCallback).toThrow("You must pass a callback to put().");
    });

    it("returns the region object to support chaining", function(done) {
      var returnValue = region.put("foo", "bar", function(error, value) {
        done();
      });

      expect(returnValue).toEqual(region);
    });

    it("passes the value to the callback", function(done) {
      region.put("foo", "bar", function(error, value) {
        expect(error).toBeFalsy();
        expect(value).toEqual("bar");
        done();
      });
    });

    it("passes an error to the callback when called with an unsupported value", function(done) {
      region.put("foo", undefined, function(error, value) {
        expect(error).toBeTruthy();
        expect(error.message).toEqual("Unable to put value undefined");
        expect(value).toBeUndefined();
        done();
      });
    });

    _.each(invalidKeys, function(invalidKey) {
      it("passes an error to the callback when passed invalid key " + util.inspect(invalidKey), function(done) {
        region.put(invalidKey, "foo", function(error, value) {
          expect(error).toBeTruthy();
          expect(error.message).toEqual("Invalid GemFire key.");
          expect(value).toBeUndefined();
          done();
        });
      });
    });
  });

  describe(".get/.put", function() {
    function testRoundTrip(value, done) {
      const key = "foo";
      region.clear();
      region.put(key, value, function(error){
        expect(error).toBeFalsy();
        region.get(key, function(error, getValue) {
          expect(error).toBeFalsy();
          expect(getValue).toEqual(value);
          done();
        });
      });
    }

    it("stores and retrieves values in the correct region", function(done) {
      var region1 = cache.getRegion("exampleRegion");
      var region2 = cache.getRegion("anotherRegion");

      region1.clear();
      region2.clear();

      async.series([
        function(callback) { region1.put("foo", "bar", callback); },
        function(callback) { region2.put("foo", 123, callback); },
        function(callback) {
          region1.get("foo", function(error, value) {
            expect(error).toBeFalsy();
            expect(value).toEqual("bar");
            callback();
          });
        },
        function(callback) {
          region2.get("foo", function(error, value) {
            expect(error).toBeFalsy();
            expect(value).toEqual(123);
            callback();
          });
        },
      ], done);
    });

    _.each([
      "",
      "foo",
      "\u0123\u4567\u89AB\uCDEF\uabcd\uef4A",
    ], function(string) {
      it("stores and retrieves strings like " + util.inspect(string), function(done) {
        testRoundTrip(string, done);
      });
    });

    it("stores and retrieves true", function(done) {
      testRoundTrip(true, done);
    });

    it("stores and retrieves false", function(done) {
      testRoundTrip(false, done);
    });

    _.each([
      [],
      ['a string', 2, true, null, false, { an: 'object' }, new Date(), ['another array', true]],
      [[[[[[[[[[[[[[[[[[["Not too deep"]]]]]]]]]]]]]]]]]]]
    ], function(array) {
      it("stores and retrieves arrays like " + util.inspect(array), function(done) {
        testRoundTrip(array, done);
      });
    });

    it("stores and retrieves sparse arrays", function(done) {
      var sparseArray = [];
      sparseArray[10] = 'an element';
      testRoundTrip(sparseArray, done);
    });

    _.each([
      1,
      1.23,
      0.0,
      Number.MAX_VALUE,
      Number.MIN_VALUE,
      Number.NEGATIVE_INFINITY,
      Number.POSITIVE_INFINITY,
    ], function(number) {
      it("stores and retrieves numbers like " + util.inspect(number), function(done) {
        testRoundTrip(number, done);
      });
    });

    it("stores and retrieves NaN", function(done) {
      region.put("foo", NaN, function(error){
        expect(error).toBeFalsy();
        region.get("foo", function(error, value) {
          expect(error).toBeFalsy();
          expect(value).toBeNaN();
          done();
        });
      });
    });

    it("can use the empty string as a key", function(done) {
      region.put("", "value", function(error){
        expect(error).toBeFalsy();
        region.get("", function(error, value) {
          expect(error).toBeFalsy();
          expect(value).toEqual("value");
          done();
        });
      });
    });

    it("stores and retrieves null", function(done) {
      testRoundTrip(null, done);
    });


    describe("for objects", function() {
      it("stores and retrieves empty objects", function(done) {
        testRoundTrip({}, done);
      });

      it("stores and retrieves objects containing strings", function(done) {
        testRoundTrip({ baz: 'quuux' }, done);
      });

      it("stores and retrieves objects with wide string keys and values", function(done) {
        testRoundTrip({ '日': '本' }, done);
      });

      it("stores and retrieves nested objects", function(done) {
        testRoundTrip({ foo: { bar: "baz" } }, done);
      });

      it("stores and retrieves objects containing arrays", function(done) {
        testRoundTrip({ foo: [] }, done);
      });

      it("stores and retrieves objects containing booleans", function(done) {
        testRoundTrip({ foo: true }, done);
      });

      it("stores and retrieves objects containing integers", function(done) {
        testRoundTrip({ foo: 123 }, done);
      });

      it("stores and retrieves objects containing floats", function(done) {
        testRoundTrip({ foo: 123.456 }, done);
      });

      it("stores and retrieves objects containing dates", function(done) {
        const date = new Date();
        testRoundTrip({ foo: date }, done);
      });

      it("stores and retrieves objects containing null", function(done) {
        testRoundTrip({ foo: null }, done);
      });

      it("stores and retrieves a stress test object", function(done) {
        const object = require("./fixtures/stress_test.json");
        testRoundTrip(object, done);
      });

      it("stores and retrieves objects with numerical keys", function(done) {
        async.series([
          function(callback) { region.put(1, "number key", callback); },
          function(callback) { region.put("1", "string key", callback); },
          function(callback) {
            region.get(1, function(error, value){
              expect(error).toBeFalsy();
              expect(value).toEqual("number key");
              callback();
            });
          },
          function(callback) {
            region.get("1", function(error, value){
              expect(error).toBeFalsy();
              expect(value).toEqual("string key");
              callback();
            });
          },
        ], done);
      });

      it("stores and retrieves objects with date keys", function(done) {
        var date = new Date();
        region.put(date, "date key", function(error){
          expect(error).toBeFalsy();
          region.get(date, function(error, value) {
            expect(error).toBeFalsy();
            expect(value).toEqual("date key");
            done();
          });
        });
      });

      it("stores and retrieves objects with boolean keys", function(done) {
        async.series([
          function(callback) {
            region.put(true, "true key", function(error) {
              expect(error).toBeFalsy();
              callback();
            });
          },
          function(callback) {
            region.put(false, "false key", function(error) {
              expect(error).toBeFalsy();
              callback();
            });
          },
          function(callback) {
            region.put("true", "true string key", function(error) {
              expect(error).toBeFalsy();
              callback();
            });
          },
          function(callback) {
            region.put("false", "false string key", function(error) {
              expect(error).toBeFalsy();
              callback();
            });
          },
          function(callback) {
            region.get(true, function(error, value) {
              expect(error).toBeFalsy();
              expect(value).toEqual("true key");
              callback();
            });
          },
          function(callback) {
            region.get(false, function(error, value) {
              expect(error).toBeFalsy();
              expect(value).toEqual("false key");
              callback();
            });
          },
          function(callback) {
            region.get("true", function(error, value) {
              expect(error).toBeFalsy();
              expect(value).toEqual("true string key");
              callback();
            });
          },
          function(callback) {
            region.get("false", function(error, value) {
              expect(error).toBeFalsy();
              expect(value).toEqual("false string key");
              callback();
            });
          },
        ], done);
      });
    });

    it("allows dynamic schema", function(done) {
      async.series([
        function(callback) { region.put('foo', {}, callback); },
        function(callback) { region.put('foo', { baz: 'qux' }, callback); },
        function(callback) {
          region.get('foo', function(error, value) {
            expect(error).toBeFalsy();
            expect(value).toEqual({ baz: "qux" });
            callback();
          });
        },
        function(callback) { region.put('foo', { baz: [] }, callback); },
        function(callback) {
          region.get('foo', function(error, value) {
            expect(error).toBeFalsy();
            expect(value).toEqual({ baz: [] });
            callback();
          });
        },
      ], done);
    });
  });

  describe(".clear", function(){
    it("removes all keys", function(done){
      async.series([
        function(callback) { region.put('key', 'value', callback); },
        function(callback) {
          region.clear();
          callback();
        },
        function(callback) {
          region.get('key', function(error) {
            expect(error).toBeTruthy();
            callback();
          });
        }
      ], done);
    });

    it("only removes keys for the region, not for other regions", function(done) {
      var region1 = cache.getRegion("exampleRegion");
      var region2 = cache.getRegion("anotherRegion");

      region1.clear();
      region2.clear();

      async.series([
        function(callback) { region1.put('key', 'value', callback); },
        function(callback) { region2.put('key', 'value', callback); },
        function(callback) {
          region1.clear();
          callback();
        },
        function(callback) {
          region1.get("key", function(error) {
            expect(error).toBeTruthy();
            callback();
          });
        },
        function(callback) { region2.get("key", callback); },
      ], done);
    });
  });

  describe(".executeFunction", function() {
    const testFunctionName = "io.pivotal.node_gemfire.TestFunction";

    it("gives the function access to the region", function(done) {
      const functionName = "io.pivotal.node_gemfire.SumRegion";

      region.put("one", 1, function(error){
        expect(error).toBeFalsy();

        region.put("two", 2, function(error){
          expect(error).toBeFalsy();

          const anotherRegion = cache.getRegion("anotherRegion");
          anotherRegion.clear();
          anotherRegion.put("thousand", 1000, function(){
            expect(error).toBeFalsy();

            async.parallel(
              [
                function(callback) {
                  region.executeFunction(functionName, function(error, results){
                    expect(results).toEqual([3]);
                    callback();
                  });
                },
                function(callback) {
                  anotherRegion.executeFunction(functionName, function(error, results){
                    expect(results).toEqual([1000]);
                    callback();
                  });
                },
              ],
              done
            );
          });
        });
      });
    });

    it("returns the region object to support chaining", function(done) {
      var returnValue = region.executeFunction(testFunctionName, function(error, value) {
        done();
      });

      expect(returnValue).toEqual(region);
    });

    it("runs a function on the GemFire cluster and passes its result to the callback", function(done) {
      region.executeFunction(testFunctionName, function(error, results) {
        expect(error).toBeFalsy();
        expect(results).toEqual(["TestFunction succeeded."]);
        done();
      });
    });

    it("runs a function with arguments on the GemFire cluster and passes its result to the callback", function(done) {
      region.executeFunction("io.pivotal.node_gemfire.Sum", [1, 2, 3], function(error, results) {
        expect(error).toBeFalsy();
        expect(results).toEqual([6]);
        done();
      });
    });

    it("throws an error when no function name is passed in", function(){
      function callWithoutArgs() {
        region.executeFunction();
      }
      expect(callWithoutArgs).toThrow("You must provide the name of a function to execute.");
    });

    it("throws an error when no callback is passed in", function(){
      function callWithoutCallback() {
        region.executeFunction(testFunctionName);
      }
      expect(callWithoutCallback).toThrow("You must pass a callback to executeFunction().");
    });

    it("throws an error when no callback is passed in, but arguments are", function(){
      function callWithoutCallback() {
        region.executeFunction(testFunctionName, ["arguments"]);
      }
      expect(callWithoutCallback).toThrow("You must pass a callback to executeFunction().");
    });

    it("throws an error when a non-function is passed as the callback", function(){
      function callWithNonFunction() {
        region.executeFunction(testFunctionName, ["arguments"], "not a function");
      }
      expect(callWithNonFunction).toThrow("You must pass a function as the callback to executeFunction().");
    });

    it("throws an error when no function name is passed in, but a callback is", function() {
      function callWithoutArgs() {
        region.executeFunction(function(){});
      }

      expect(callWithoutArgs).toThrow("You must provide the name of a function to execute.");
    });

    it("passes an error into the callback when the function is not found", function(done){
      region.executeFunction("com.example.Nonexistent", function(error, results) {
        expect(error).toBeTruthy();
        expect(error.message).toEqual(
          "gemfire::MessageException: Execute::GET_FUNCTION_ATTRIBUTES: message from server could not be handled"
        );
        expect(results).toBeUndefined();
        done();
      });
    });

    it("passes an error into the callback when the function throws an exception", function(done) {
      region.executeFunction("io.pivotal.node_gemfire.TestFunctionException", function(error, results) {
        expect(error).toBeTruthy();
        expect(error.message).toMatch(
          /com.gemstone.gemfire.cache.execute.FunctionException: Test exception message thrown by server./
        );
        expect(results).toBeUndefined();
        done();
      });
    });

    it("passes the results and an error when the function sends an exception with the results", function(done) {
      region.executeFunction("io.pivotal.node_gemfire.TestFunctionExceptionResult", function(error, results) {
        expect(error).toBeTruthy();
        expect(error.message).toMatch("java.lang.Exception: Test exception message sent by server.");
        expect(results.length).toEqual(1);
        expect(results[0]).toEqual("First result");
        done();
      });
    });

    it("supports objects as input and output", function(done) {
      const results = region.executeFunction("io.pivotal.node_gemfire.Passthrough", { foo: 'bar' }, function(error, results){
        expect(error).toBeFalsy();
        expect(results).toEqual([{ foo: 'bar' }]);
        done();
      });
    });
  });

  describe(".inspect", function() {
    it("returns a user-friendly display string describing the region", function() {
      expect(region.inspect()).toEqual('[Region name="exampleRegion"]');
    });
  });

  describe(".name", function() {
    it("returns the name of the region", function() {
      expect(region.name).toEqual("exampleRegion");
    });
  });

  describe(".remove", function() {
    it("throws an error if no key is given", function() {
      function callNoArgs() {
        region.remove();
      }

      expect(callNoArgs).toThrow("You must pass a key and a callback to remove().");
    });

    it("throws an error if no callback is given", function() {
      function callNoCallback() {
        region.remove('foo');
      }

      expect(callNoCallback).toThrow("You must pass a key and a callback to remove().");
    });

    it("throws an error if a non-function is passed as the callback", function() {
      function callNoCallback() {
        region.remove('foo', 'not a function');
      }

      expect(callNoCallback).toThrow("You must pass a function as the callback to remove().");
    });

    it("removes the entry at the given key", function(done) {
      region.put("foo", "bar", function(error){
        expect(error).toBeFalsy();
        region.remove("foo", function(error) {
          expect(error).toBeFalsy();
          region.get("foo", function(error) {
            expect(error).toBeTruthy();
            done();
          });
        });
      });
    });

    it("passes no error when it succeeds", function(done) {
      region.put("foo", "bar", function(error){
        expect(error).toBeFalsy();
        region.remove("foo", function(error, result) {
          expect(error).toBeFalsy();
          done();
        });
      });
    });

    it("returns itself for chaining", function(done) {
      region.put("foo", "bar", function(error){
        expect(error).toBeFalsy();
        expect(region.remove("foo", done)).toEqual(region);
      });
    });

    it("passes an error to the callback if the entry is not present", function(done) {
      region.remove("foo", function(error, result) {
        expect(error).toBeTruthy();
        expect(error.message).toEqual("Key not found in region.");
        expect(result).toBeUndefined();
        done();
      });
    });

    _.each(invalidKeys, function(invalidKey) {
      it("passes an error to the callback when passed invalid key " + util.inspect(invalidKey), function(done) {
        region.remove(invalidKey, function(error, value) {
          expect(error).toBeTruthy();
          expect(error.message).toEqual("Invalid GemFire key.");
          expect(value).toBeUndefined();
          done();
        });
      });
    });
  });
});

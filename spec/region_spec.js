const childProcess = require('child_process');
const _ = require("lodash");
const util = require("util");
const async = require('async');
const randomString = require("random-string");
const factories = require('./support/factories.js');
const errorMatchers = require("./support/error_matchers.js");
const until = require("./support/until.js");

const invalidKeys = [null, undefined, []];
const invalidValues = [null];

describe("gemfire.Region", function() {
  var region, cache;

  beforeEach(function(done) {
    this.addMatchers(errorMatchers);
    cache = factories.getCache();
    region = cache.getRegion("exampleRegion");
    region.clear(done);
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
        expect(error).toBeError("Key not found in region.");
        expect(value).toBeUndefined();
        done();
      });
    });

    _.each(invalidKeys, function(invalidKey) {
      it("passes an error to the callback when passed the invalid key " + util.inspect(invalidKey), function(done) {
        region.get(invalidKey, function(error, value) {
          expect(error).toBeError("Invalid GemFire key.");
          expect(value).toBeUndefined();
          done();
        });
      });
    });

    it("throws an error when passed a function as a key", function() {
      function callWithFunctionKey() {
        region.get(function(){}, function(){});
      }

      expect(callWithFunctionKey).toThrow("Unable to serialize to GemFire; functions are not supported.");
    });

    it("passes the value into the callback", function(done) {
      region.put('foo', 'bar', function (error) {
        expect(error).not.toBeError();
        region.get("foo", function(error, value) {
          expect(error).not.toBeError();
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

      expect(putWithNoArgs).toThrow("You must pass a key and value to put().");
    });

    it("throws an error when no value is passed", function() {
      function putWithOnlyKey() {
        region.put('foo');
      }

      expect(putWithOnlyKey).toThrow("You must pass a key and value to put().");

    });

    it("throws an error if a non-function is passed as the callback", function() {
      function putWithNonFunctionCallback() {
        region.put('foo', 'bar', 'not a function');
      }

      expect(putWithNonFunctionCallback).toThrow("You must pass a function as the callback to put().");
    });

    it("returns the region object to support chaining", function(done) {
      var returnValue = region.put("foo", "bar", function(error) {
        done();
      });

      expect(returnValue).toEqual(region);
    });

    it("succeeds when the value is put into the cache", function(done) {
      region.put("foo", "bar", function(error) {
        expect(error).not.toBeError();
        done();
      });
    });

    _.each(invalidValues, function(invalidValue) {
      it("passes an error to the callback when passed invalid value " + util.inspect(invalidValue), function(done) {
        region.put("foo", invalidValue, function(error) {
          expect(error).toBeError("Invalid GemFire value.");
          done();
        });
      });
    });

    it("throws an error when passed a function as a value", function() {
      function callWithFunctionValue() {
        region.put("foo", function(){}, function(){});
      }

      expect(callWithFunctionValue).toThrow("Unable to serialize to GemFire; functions are not supported.");
    });

    _.each(invalidKeys, function(invalidKey) {
      it("passes an error to the callback when passed invalid key " + util.inspect(invalidKey), function(done) {
        region.put(invalidKey, "foo", function(error) {
          expect(error).toBeError("Invalid GemFire key.");
          done();
        });
      });
    });

    it("throws an error when passed a function as a key", function() {
      function callWithFunctionKey() {
        region.put(function(){}, "foo", function(){});
      }

      expect(callWithFunctionKey).toThrow("Unable to serialize to GemFire; functions are not supported.");
    });

    it("emits an event when an error occurs and there is no callback", function(done) {
      const errorHandler = jasmine.createSpy("errorHandler").andCallFake(function(error){
        expect(error).toBeError();
        done();
      });

      region.on("error", errorHandler);
      region.put("foo", null);

      _.delay(function(){
        expect(errorHandler).toHaveBeenCalled();
        done();
      }, 1000);
    });

    it("does not emit an event when an error occurs and there is a callback", function(done) {
      const errorHandler = jasmine.createSpy("errorHandler");

      region.on("error", errorHandler);

      region.put("foo", null, function(){
        expect(errorHandler).not.toHaveBeenCalled();
        done();
      });
    });

    it("does not emit an event when no error occurs and there is no callback", function(done) {
      region.put("foo", "bar");

      until(
        function(test) { region.get("foo", test); },
        function(error, result) { return !error && result == "bar"; },
        done
      );
    });
  });

  describe(".get/.put", function() {
    function testRoundTrip(value, done) {
      const key = "foo";

      async.series([
        function(next) { region.clear(next); },
        function(next) {
          region.put(key, value, function(error) {
            expect(error).not.toBeError();
            next();
          });
        },
        function(next) {
          region.get(key, function(error, getValue) {
            expect(error).not.toBeError();
            expect(getValue).toEqual(value);
            next();
          });
        }
      ], done);
    }

    it("stores and retrieves values in the correct region", function(done) {
      var region1 = cache.getRegion("exampleRegion");
      var region2 = cache.getRegion("anotherRegion");

      async.series([
        function(next) { region1.clear(next); },
        function(next) { region2.clear(next); },
        function(next) { region1.put("foo", "bar", next); },
        function(next) { region2.put("foo", 123, next); },
        function(next) {
          region1.get("foo", function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual("bar");
            next();
          });
        },
        function(next) {
          region2.get("foo", function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual(123);
            next();
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

    it("stores and retrieves large strings", function(done) {
      const largeString = randomString({length: 65536});
      testRoundTrip(largeString, done);
    });

    it("stores and retrieves large wide strings", function(done) {
      const largeString = randomString({length: 65533}) + "☃";
      testRoundTrip(largeString, done);
    });

    it("stores and retrieves string objects", function(done) {
      /* jshint -W053 */
      region.put("foo", new String("string object"), function(error) {
        expect(error).not.toBeError();
        region.get("foo", function(error, value) {
          expect(error).not.toBeError();
          expect(value).toEqual("string object");
          done();
        });
      });
    });

    it("stores and retrieves number objects", function(done) {
      /* jshint -W053 */
      region.put("foo", new Number(123), function(error) {
        expect(error).not.toBeError();
        region.get("foo", function(error, value) {
          expect(error).not.toBeError();
          expect(value).toEqual(123);
          done();
        });
      });
    });

    it("stores and retrieves true", function(done) {
      testRoundTrip(true, done);
    });

    it("stores and retrieves false", function(done) {
      testRoundTrip(false, done);
    });

    it("stores and retrieves boolean objects", function(done) {
      /* jshint -W053 */
      region.putAll({
        "true": new Boolean(true),
        "false": new Boolean(false)
      }, function(error) {
        expect(error).not.toBeError();

        region.getAll(["true", "false"], function(error, response) {
          expect(error).not.toBeError();

          expect(response).toEqual({
            "true": true,
            "false": false
          });

          done();
        });
      });
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
        expect(error).not.toBeError();
        region.get("foo", function(error, value) {
          expect(error).not.toBeError();
          expect(value).toBeNaN();
          done();
        });
      });
    });

    it("can use the empty string as a key", function(done) {
      region.put("", "value", function(error){
        expect(error).not.toBeError();
        region.get("", function(error, value) {
          expect(error).not.toBeError();
          expect(value).toEqual("value");
          done();
        });
      });
    });

    it("can use objects as a key", function(done) {
      const interestingObjects = [
        {},
        { foo: 'bar' },
        { qux: 'bar' },
        { foo: null },
        { foo: [] },
        { foo: ['bar'] },
        { foo: ['bar', 'baz'] },
        { foo: { bar: 'baz' } },
        { foo: { bar: 'qux' } },
        { foo: [{ bar: 'baz' }] },
        { foo: [{ bar: 'qux' }] },
        { foo: [{ bar: 'baz' }, { bar: 'qux' }] },
      ];

      const puts = _.map(interestingObjects, function(object, index){
        return function(next) { region.put(object, index, next); };
      });

      const expectations = _.map(interestingObjects, function(object, index){
        return function(next) {
          region.get(object, function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual(index);
            next();
          });
        };
      });

      async.series(puts.concat(expectations), done);
    });

    it("treats the same object as the same key in the region", function(done) {
      async.series([
        function(next) { region.put({foo: 'bar'}, 'old value', next); },
        function(next) { region.put({foo: 'bar'}, 'new value', next); },
        function(next) {
          region.get({foo: 'bar'}, function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual('new value');
            next();
          });
        },
      ], done);
    });

    it("stores and retrieves undefined", function(done) {
      testRoundTrip(undefined, done);
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

      it("stores and retrieves objects containing undefined", function(done) {
        testRoundTrip({ foo: undefined }, done);
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
              expect(error).not.toBeError();
              expect(value).toEqual("number key");
              callback();
            });
          },
          function(callback) {
            region.get("1", function(error, value){
              expect(error).not.toBeError();
              expect(value).toEqual("string key");
              callback();
            });
          },
        ], done);
      });

      it("stores and retrieves objects with date keys", function(done) {
        var date = new Date();
        region.put(date, "date key", function(error){
          expect(error).not.toBeError();
          region.get(date, function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual("date key");
            done();
          });
        });
      });

      it("stores and retrieves objects with boolean keys", function(done) {
        async.series([
          function(callback) {
            region.put(true, "true key", function(error) {
              expect(error).not.toBeError();
              callback();
            });
          },
          function(callback) {
            region.put(false, "false key", function(error) {
              expect(error).not.toBeError();
              callback();
            });
          },
          function(callback) {
            region.put("true", "true string key", function(error) {
              expect(error).not.toBeError();
              callback();
            });
          },
          function(callback) {
            region.put("false", "false string key", function(error) {
              expect(error).not.toBeError();
              callback();
            });
          },
          function(callback) {
            region.get(true, function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual("true key");
              callback();
            });
          },
          function(callback) {
            region.get(false, function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual("false key");
              callback();
            });
          },
          function(callback) {
            region.get("true", function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual("true string key");
              callback();
            });
          },
          function(callback) {
            region.get("false", function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual("false string key");
              callback();
            });
          },
        ], done);
      });

      it("allows dynamic schema", function(done) {
        async.series([
          function(callback) { region.put('foo', {}, callback); },
          function(callback) {
            region.get('foo', function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual({});
              callback();
            });
          },
          function(callback) { region.put('foo', { baz: 'qux' }, callback); },
          function(callback) {
            region.get('foo', function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual({ baz: "qux" });
              callback();
            });
          },
          function(callback) { region.put('foo', { baz: [] }, callback); },
          function(callback) {
            region.get('foo', function(error, value) {
              expect(error).not.toBeError();
              expect(value).toEqual({ baz: [] });
              callback();
            });
          },
        ], done);
      });
    });
  });

  describe(".clear", function(){
    it("removes all keys, then calls the callback", function(done){
      async.series([
        function(next) { region.put('key', 'value', next); },
        function(next) { region.clear(next); },
        function(next) {
          region.get('key', function(error) {
            expect(error).toBeError();
            next();
          });
        }
      ], done);
    });

    it("returns the region to support chaining", function(done) {
      expect(region.clear(done)).toEqual(region);
    });

    it("only removes keys for the region, not for other regions", function(done) {
      var region1 = cache.getRegion("exampleRegion");
      var region2 = cache.getRegion("anotherRegion");

      async.series([
        function(next) { region1.clear(next); },
        function(next) { region2.clear(next); },
        function(next) { region1.put('key', 'value', next); },
        function(next) { region2.put('key', 'value', next); },
        function(next) { region1.clear(next); },
        function(next) {
          region1.get("key", function(error) {
            expect(error).toBeError();
            next();
          });
        },
        function(next) { region2.get("key", next); },
      ], done);
    });

    it("throws an error if the callback is not a function", function() {
      function callWithNonFunction() {
        region.clear("this is not a function");
      }

      expect(callWithNonFunction).toThrow("You must pass a function as the callback to clear().");
    });

    // Pending us figuring out how to cause an error in clear()
    xit("emits an event when an error occurs and there is no callback", function(){});

    it("does not emit an event when no error occurs and there is no callback", function(done) {
      // if an error event is emitted, the test suite will crash here
      region.put("foo", "bar", function(error) {
        expect(error).not.toBeError();

        region.clear();

        until(
          function(test) { region.get("foo", test); },
          function(error) { return error && error.message === "Key not found in region."; },
          done
        );
      });
    });
  });

  describe(".executeFunction", function() {
    const testFunctionName = "io.pivotal.node_gemfire.TestFunction";

    it("gives the function access to the region", function(done) {
      const functionName = "io.pivotal.node_gemfire.SumRegion";
      const anotherRegion = cache.getRegion("anotherRegion");

      async.series([
        function(next) { region.put("one", 1, next); },
        function(next) { region.put("two", 2, next); },
        function(next) { anotherRegion.clear(next); },
        function(next) { anotherRegion.put("thousand", 1000, next); },
        function(next) {
          region.executeFunction(functionName, function(error, results){
            expect(results).toEqual([3]);
            next();
          });
        },
        function(next) {
          anotherRegion.executeFunction(functionName, function(error, results){
            expect(results).toEqual([1000]);
            next();
          });
        }
      ], done);
    });

    it("returns the region object to support chaining", function(done) {
      var returnValue = region.executeFunction(testFunctionName, function(error, value) {
        done();
      });

      expect(returnValue).toEqual(region);
    });

    it("runs a function on the GemFire cluster and passes its result to the callback", function(done) {
      region.executeFunction(testFunctionName, function(error, results) {
        expect(error).not.toBeError();
        expect(results).toEqual(["TestFunction succeeded."]);
        done();
      });
    });

    it("runs a function with arguments on the GemFire cluster and passes its result to the callback", function(done) {
      region.executeFunction("io.pivotal.node_gemfire.Sum", [1, 2, 3], function(error, results) {
        expect(error).not.toBeError();
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

    it("throws an error when a function is passed in as an argument", function() {
      function callWithBadArgs() {
        region.executeFunction(testFunctionName, [function(){}], function(){});
      }

      expect(callWithBadArgs).toThrow("Unable to serialize to GemFire; functions are not supported.");
    });

    it("passes an error into the callback when the function is not found", function(done){
      region.executeFunction("com.example.Nonexistent", function(error, results) {
        expect(error).toBeError(
          "gemfire::MessageException: Execute::GET_FUNCTION_ATTRIBUTES: message from server could not be handled"
        );
        expect(results).toBeUndefined();
        done();
      });
    });

    it("passes an error into the callback when the function throws an exception", function(done) {
      region.executeFunction("io.pivotal.node_gemfire.TestFunctionException", function(error, results) {
        expect(error).toBeError();
        expect(error.message).toMatch(
          /com.gemstone.gemfire.cache.execute.FunctionException: Test exception message thrown by server./
        );
        expect(results).toBeUndefined();
        done();
      });
    });

    it("passes the results and an error when the function sends an exception with the results", function(done) {
      region.executeFunction("io.pivotal.node_gemfire.TestFunctionExceptionResult", function(error, results) {
        expect(error).toBeError();
        expect(error.message).toMatch("java.lang.Exception: Test exception message sent by server.");
        expect(results.length).toEqual(1);
        expect(results[0]).toEqual("First result");
        done();
      });
    });

    it("supports objects as input and output", function(done) {
      const results = region.executeFunction("io.pivotal.node_gemfire.Passthrough", { foo: 'bar' }, function(error, results){
        expect(error).not.toBeError();
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
        expect(error).not.toBeError();
        region.remove("foo", function(error) {
          expect(error).not.toBeError();
          region.get("foo", function(error) {
            expect(error).toBeError();
            done();
          });
        });
      });
    });

    it("passes no error when it succeeds", function(done) {
      region.put("foo", "bar", function(error){
        expect(error).not.toBeError();
        region.remove("foo", function(error, result) {
          expect(error).not.toBeError();
          done();
        });
      });
    });

    it("returns itself for chaining", function(done) {
      region.put("foo", "bar", function(error){
        expect(error).not.toBeError();
        expect(region.remove("foo", done)).toEqual(region);
      });
    });

    it("passes an error to the callback if the entry is not present", function(done) {
      region.remove("foo", function(error, result) {
        expect(error).toBeError("Key not found in region.");
        expect(result).toBeUndefined();
        done();
      });
    });

    _.each(invalidKeys, function(invalidKey) {
      it("passes an error to the callback when passed invalid key " + util.inspect(invalidKey), function(done) {
        region.remove(invalidKey, function(error, value) {
          expect(error).toBeError("Invalid GemFire key.");
          expect(value).toBeUndefined();
          done();
        });
      });
    });

    it("throws an error when passed a function as a key", function() {
      function callWithFunctionKey() {
        region.remove(function(){}, function(){});
      }

      expect(callWithFunctionKey).toThrow("Unable to serialize to GemFire; functions are not supported.");
    });

  });

  describe(".query", function() {
    it("passes the results into the callback for the passed-in predicate", function(done) {
      async.series([
        function (callback) { region.put("key1", 1, callback); },
        function (callback) { region.put("key2", 2, callback); },
        function (callback) { region.put("key3", 3, callback); },
        function (callback) {
          region.query("this > 1", function(error, response) {
            expect(error).not.toBeError();
            const results = response.toArray();

            expect(results.length).toEqual(2);
            expect(results).toContain(2);
            expect(results).toContain(3);

            callback();
          });
        },
      ], done);
    });

    it("requires a query predicate string", function() {
      function queryWithoutPredicate(){
        region.query();
      }

      expect(queryWithoutPredicate).toThrow("You must pass a query predicate string and a callback to query().");
    });

    it("requires a callback", function() {
      function queryWithoutCallback(){
        region.query("true");
      }

      expect(queryWithoutCallback).toThrow("You must pass a query predicate string and a callback to query().");
    });

    it("requires the callback to be a function", function() {
      function queryWithNonCallback(){
        region.query("true", "not a function");
      }

      expect(queryWithNonCallback).toThrow("You must pass a function as the callback to query().");
    });

    it("passes along errors from an invalid query", function(done) {
      region.query("Invalid query", function(error, response) {
        expect(error).toBeError();
        expect(error.message).toMatch(/gemfire::QueryException/);
        done();
      });
    });

    it("returns the region for chaining", function() {
      expect(region.query("true", function(){})).toEqual(region);
    });
  });

  describe(".selectValue", function() {
    it("passes the result into the callback for the passed-in predicate", function(done) {
      async.series([
        function (callback) { region.put("key1", { foo: 1 }, callback); },
        function (callback) { region.put("key2", { foo: 2 }, callback); },
        function (callback) { region.put("key3", { foo: 3 }, callback); },
        function (callback) {
          region.selectValue("foo = 2", function(error, result) {
            expect(error).not.toBeError();
            expect(result).toEqual({ foo: 2 });

            callback();
          });
        },
      ], done);
    });

    it("passes an error to the callback when there is more than one result", function(done) {
      async.series([
        function (next) {
          region.putAll({
            key1: { foo: 'bar', baz: 'qux' },
            key2: { foo: 'bar' }
          }, next);
        },

        function (next) {
          region.selectValue("foo = 'bar'", function(error, response) {
            expect(error).toBeError("gemfire::QueryException: selectValue has more than one result");
            next();
          });
        }
      ], done);
    });

    it("requires a query predicate string", function() {
      function queryWithoutPredicate(){
        region.selectValue();
      }

      expect(queryWithoutPredicate).toThrow("You must pass a query predicate string and a callback to selectValue().");
    });

    it("requires a callback", function() {
      function queryWithoutCallback(){
        region.selectValue("true");
      }

      expect(queryWithoutCallback).toThrow("You must pass a query predicate string and a callback to selectValue().");
    });

    it("requires the callback to be a function", function() {
      function queryWithNonCallback(){
        region.selectValue("true", "not a function");
      }

      expect(queryWithNonCallback).toThrow("You must pass a function as the callback to selectValue().");
    });

    it("passes along errors from an invalid query", function(done) {
      region.selectValue("Invalid query", function(error, response) {
        expect(error).toBeError();
        expect(error.message).toMatch(/gemfire::QueryException/);
        done();
      });
    });

    it("returns the region for chaining", function() {
      expect(region.selectValue("true", function(){})).toEqual(region);
    });
  });

  describe(".existsValue", function() {
    it("passes the result into the callback for the passed-in predicate", function(done) {
      async.series([
        function (callback) { region.put("key1", { foo: 1 }, callback); },
        function (callback) { region.put("key2", { foo: 2 }, callback); },
        function (callback) { region.put("key3", { foo: 3 }, callback); },
        function (callback) {
          region.existsValue("foo = 2", function(error, result) {
            expect(error).not.toBeError();
            expect(result).toEqual(true);
            callback();
          });
        },
        function (callback) {
          region.existsValue("foo = 4", function(error, result) {
            expect(error).not.toBeError();
            expect(result).toEqual(false);
            callback();
          });
        },
      ], done);
    });

    it("requires a query predicate string", function() {
      function queryWithoutPredicate(){
        region.existsValue();
      }

      expect(queryWithoutPredicate).toThrow("You must pass a query predicate string and a callback to existsValue().");
    });

    it("requires a callback", function() {
      function queryWithoutCallback(){
        region.existsValue("true");
      }

      expect(queryWithoutCallback).toThrow("You must pass a query predicate string and a callback to existsValue().");
    });

    it("requires the callback to be a function", function() {
      function queryWithNonCallback(){
        region.existsValue("true", "not a function");
      }

      expect(queryWithNonCallback).toThrow("You must pass a function as the callback to existsValue().");
    });

    it("passes along errors from an invalid query", function(done) {
      region.existsValue("Invalid query", function(error, response) {
        expect(error).toBeError();
        expect(error.message).toMatch(/gemfire::QueryException/);
        done();
      });
    });

    it("returns the region for chaining", function() {
      expect(region.existsValue("true", function(){})).toEqual(region);
    });
  });

  describe(".putAll", function() {
    it("sets multiple values at once", function(done) {
      async.series([
        function(next) {
          region.putAll({ key1: 'foo', key2: 'bar' }, next);
        },
        function(next) {
          region.get('key1', function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual('foo');

            next();
          });
        },
        function(next) {
          region.get('key2', function(error, value) {
            expect(error).not.toBeError();
            expect(value).toEqual('bar');

            next();
          });
        },
      ], done);
    });

    _.each(invalidValues, function(invalidValue) {
      it("passes an error to the callback when passed invalid value " + util.inspect(invalidValue), function(done) {
        region.putAll({"foo": "bar", "baz": invalidValue}, function(error) {
          expect(error).toBeError("Invalid GemFire value.");
          done();
        });
      });
    });

    it("throws an error when passed a function as a value", function() {
      function callWithFunctionValue() {
        region.putAll({"foo": "bar", "baz": function(){}}, function(){});
      }

      expect(callWithFunctionValue).toThrow("Unable to serialize to GemFire; functions are not supported.");
    });

    it("returns the region for chaining", function() {
      expect(region.putAll({ key: 'value' }, function(){})).toEqual(region);
    });

    it("requires a map argument", function() {
      function callWithNoArgs() {
        region.putAll();
      }

      expect(callWithNoArgs).toThrow("You must pass an object and a callback to putAll().");
    });

    it("requires the callback to be a function", function() {
      function callWithNonFunction() {
        region.putAll({ foo: 'foo' }, 'this thing is not a function');
      }

      expect(callWithNonFunction).toThrow("You must pass a function as the callback to putAll().");
    });

    it("requires the object argument to be an object", function() {
      function callWithNull() {
        region.putAll(null, function (){});
      }

      function callWithString() {
        region.putAll('foobar', function (){});
      }

      errorMessage = "You must pass an object and a callback to putAll().";
      expect(callWithNull).toThrow(errorMessage);
      expect(callWithString).toThrow(errorMessage);
    });

    it("emits an event when an error occurs and there is no callback", function(done) {
      const errorHandler = jasmine.createSpy("errorHandler").andCallFake(function(error){
        expect(error).toBeError();
        done();
      });

      region.on("error", errorHandler);
      region.putAll({ foo: null, bar: "baz" });

      _.delay(function(){
        expect(errorHandler).toHaveBeenCalled();
        done();
      }, 1000);
    });

    it("does not emit an event when an error occurs and there is a callback", function(done) {
      const errorHandler = jasmine.createSpy("errorHandler");

      region.on("error", errorHandler);

      region.putAll({ foo: null, bar: "baz" }, function(){
        expect(errorHandler).not.toHaveBeenCalled();
        done();
      });
    });

    it("does not emit an event when no error occurs and there is no callback", function(done) {
      // if it fails, an uncaught event will blow up the suite
      region.putAll({ foo: "bar" });

      until(
        function(test) { region.get("foo", test); },
        function(error, value) { return value === "bar"; },
        done
      );
    });
  });

  describe(".getAll", function() {
    it("passes the results as an array to the callback", function(done) {
      async.series([
        function(next) { region.put('key1', 'value1', next); },
        function(next) { region.put('key2', 'value2', next); },
        function(next) { region.put('key3', 'value3', next); },
        function(next) {
          region.getAll(['key1', 'key3'], function(error, response){
            expect(error).not.toBeError();

            expect(response.key1).toEqual('value1');
            expect(response.key3).toEqual('value3');
            expect(response.key2).toBeUndefined();

            next();
          });
        }
      ], done);
    });

    it("returns the region for chaining", function() {
      expect(region.getAll(['key'], function(){})).toEqual(region);
    });

    it("requires a keys argument", function() {
      function callWithNoArgs() {
        region.getAll();
      }

      expect(callWithNoArgs).toThrow("You must pass an array of keys and a callback to getAll().");
    });

    it("requires a callback", function() {
      function callWithNoCallback() {
        region.getAll(['foo']);
      }

      expect(callWithNoCallback).toThrow("You must pass a callback to getAll().");
    });

    it("requires the callback to be a function", function() {
      function callWithNonFunction() {
        region.getAll(['foo'], 'not a function');
      }

      expect(callWithNonFunction).toThrow("You must pass a function as the callback to getAll().");
    });

    it("requires the keys argument to be an array", function() {
      function callWithNonArray() {
        region.getAll('not an array', function (){});
      }

      expect(callWithNonArray).toThrow("You must pass an array of keys and a callback to getAll().");
    });

    it("passes an empty object to the callback when provided with no keys", function(done){
      region.getAll([], function(error, response) {
        expect(error).not.toBeError();
        expect(response).toEqual({});

        done();
      });
    });

    _.each(invalidKeys, function(invalidKey) {
      it("passes an error to the callback when passed the invalid key " + util.inspect(invalidKey), function(done) {
        region.getAll(["foo", invalidKey], function(error, value) {
          expect(error).toBeError("Invalid GemFire key.");
          expect(value).toBeUndefined();
          done();
        });
      });
    });

    it("throws an error when passed a function as a key", function() {
      function callWithFunctionKey() {
        region.getAll([function(){}], function(){});
      }

      expect(callWithFunctionKey).toThrow("Unable to serialize to GemFire; functions are not supported.");
    });
  });

  describe(".keys", function() {
    it("passes an array of key names in the region to the callback", function(done) {
      async.series([
        function(next) { region.putAll({"foo": 1, "bar": 2, "baz": 3}, next); },
        function(next) {
          region.keys(function(error, keys) {
            expect(error).not.toBeError();
            expect(keys.length).toEqual(3);
            expect(keys).toContain("foo");
            expect(keys).toContain("bar");
            expect(keys).toContain("baz");
            next();
          });
        },
      ], done);
    });

    it("throws an exception when no callback is passed", function() {
      function callWithNoArgs() {
        region.keys();
      }

      expect(callWithNoArgs).toThrow("You must pass a callback to keys().");
    });

    it("throws an exception when a non-function is passed as the callback", function() {
      function callWithNonFunction() {
        region.keys("this is some crazy string");
      }

      expect(callWithNonFunction).toThrow("You must pass a function as the callback to keys().");
    });
  });
});

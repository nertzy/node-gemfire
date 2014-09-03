var childProcess = require('child_process');
var factories = require('./support/factories.js');

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

  describe(".put/.get", function() {
    it("returns undefined for unknown keys", function() {
      expect(region.get('foo')).toBeUndefined();
    });

    it("throws an error if a key is not passed to .get", function() {
      function getWithoutKey() {
        region.get();
      }
      expect(getWithoutKey).toThrow("You must pass a key to get()");
    });

    describe("async get", function() {
      beforeEach(function(){
        region.put('foo', 'bar');
      });

      it("returns the region object to support chaining", function(done) {
        var returnValue = region.get("foo", function(error, value) {
          done();
        });

        expect(returnValue).toEqual(region);
      });

      it("supports async get via a callback", function(done) {
        region.get("foo", function(error, value) {
          expect(error).toBeNull();
          expect(value).toEqual("bar");
          done();
        });
      });

      it("returns an error when async get is called for a nonexistent key", function(done) {
        region.get("baz", function(error, value) {
          expect(error).not.toBeNull();
          expect(error.message).toEqual("Key not found in region.");
          expect(value).toBeUndefined();
          done();
        });
      });
    });

    describe("async put", function() {
      it("returns the region object to support chaining", function(done) {
        var returnValue = region.put("foo", "bar", function(error, value) {
          done();
        });

        expect(returnValue).toEqual(region);
      });

      it("supports async put via a callback", function(done) {
        region.put("foo", "bar", function(error, value) {
          expect(error).toBeNull();
          expect(value).toEqual("bar");
          expect(region.get("foo")).toEqual("bar");
          done();
        });
      });

      it("returns an error when async put is called with an unsupported value", function(done) {
        region.put("foo", undefined, function(error, value) {
          expect(error).not.toBeNull();
          expect(error.message).toEqual("Unable to put value undefined");
          expect(value).toBeUndefined();
          done();
        });
      });
    });

    it("stores and retrieves values in the correct region", function() {
      var region1 = cache.getRegion("exampleRegion");
      var region2 = cache.getRegion("anotherRegion");

      region1.clear();
      region2.clear();

      region1.put("foo", "bar");
      expect(region1.get("foo")).toEqual("bar");
      expect(region2.get("foo")).toBeUndefined();

      region2.put("foo", 123);
      expect(region1.get("foo")).toEqual("bar");
      expect(region2.get("foo")).toEqual(123);
    });

    it("stores and retrieves strings", function() {
      expect(region.put("foo", "bar")).toEqual("bar");
      expect(region.get("foo")).toEqual("bar");

      expect(region.put("empty string", "")).toEqual("");
      expect(region.get("empty string")).toEqual("");

      var wideString =  "\u0123\u4567\u89AB\uCDEF\uabcd\uef4A";
      expect(region.put("wide string", wideString)).toEqual(wideString);
      expect(region.get("wide string")).toEqual(wideString);
    });

    it("stores and retrieves booleans", function() {
      expect(region.put("true", true)).toEqual(true);
      expect(region.get("true")).toEqual(true);

      expect(region.put("false", false)).toEqual(false);
      expect(region.get("false")).toEqual(false);
    });

    it("stores and retrieves dates", function() {
      var now = new Date();
      expect(region.put('now', now)).toEqual(now);
      expect(region.get('now')).toEqual(now);
    });

    it("stores and retrieves arrays", function() {
      var emptyArray = [];
      expect(region.put('empty array', emptyArray)).toEqual(emptyArray);
      expect(region.get('empty array')).toEqual(emptyArray);

      var complexArray = ['a string', 2, true, null, false, { an: 'object' }, new Date(), ['another array', true]];
      expect(region.put('empty array', complexArray)).toEqual(complexArray);
      expect(region.get('empty array')).toEqual(complexArray);

      var sparseArray = [];
      sparseArray[10] = 'an element';
      expect(region.put('sparse array', sparseArray)).toEqual(sparseArray);
      expect(region.get('sparse array')).toEqual(sparseArray);

      var deepArray = [[[[[[[[[[[[[[[[[[["Not too deep"]]]]]]]]]]]]]]]]]]];
      expect(region.put('deep array', deepArray)).toEqual(deepArray);
      expect(region.get('deep array')).toEqual(deepArray);
    });

    it("stores and retrieves numbers", function() {
      expect(region.put("foo", 1.23)).toEqual(1.23);
      expect(region.get("foo")).toEqual(1.23);

      expect(region.put("one", 1)).toEqual(1);
      expect(region.get("one")).toEqual(1);

      expect(region.put("zero", 0.0)).toEqual(0.0);
      expect(region.get("zero")).toEqual(0.0);

      expect(region.put("MAX_VALUE", Number.MAX_VALUE)).toEqual(Number.MAX_VALUE);
      expect(region.get("MAX_VALUE")).toEqual(Number.MAX_VALUE);

      expect(region.put("MIN_VALUE", Number.MIN_VALUE)).toEqual(Number.MIN_VALUE);
      expect(region.get("MIN_VALUE")).toEqual(Number.MIN_VALUE);

      expect(region.put("NaN", Number.NaN)).toBeNaN();
      expect(region.get("NaN")).toBeNaN();

      expect(region.put("NaN", NaN)).toBeNaN();
      expect(region.get("NaN")).toBeNaN();

      expect(region.put("NEGATIVE_INFINITY", Number.NEGATIVE_INFINITY)).toEqual(Number.NEGATIVE_INFINITY);
      expect(region.get("NEGATIVE_INFINITY")).toEqual(Number.NEGATIVE_INFINITY);

      expect(region.put("POSITIVE_INFINITY", Number.POSITIVE_INFINITY)).toEqual(Number.POSITIVE_INFINITY);
      expect(region.get("POSITIVE_INFINITY")).toEqual(Number.POSITIVE_INFINITY);
    });

    it("stores and retrieves null", function() {
      expect(region.put("foo", null)).toBeNull();
      expect(region.get("foo")).toBeNull();
    });

    it("can use the empty string as a key", function() {
      expect(region.put('', 'value')).toEqual('value');
      expect(region.get('')).toEqual('value');
    });

    describe("for objects", function() {
      it("stores and retrieves empty objects", function() {
        expect(region.put('empty object', {})).toEqual({});
        expect(region.get('empty object')).toEqual({});
      });

      it("stores and retrieves objects containing strings", function() {
        expect(region.put('object', { baz: 'quuux' })).toEqual({ baz: 'quuux' });
        expect(region.get('object')).toEqual({ baz: 'quuux' });
      });

      it("stores and retrieves objects with wide string keys and values", function() {
        expect(region.put('object', { '日': '本' })).toEqual({ '日': '本' });
        expect(region.get('object')).toEqual({ '日': '本' });
      });

      it("stores and retrieves nested objects", function() {
        expect(region.put('nested object', { foo: { bar: 'baz' } })).toEqual( { foo: { bar: 'baz' } } );
        expect(region.get('nested object')).toEqual( { foo: { bar: 'baz' } } );
      });

      it("stores and retrieves objects containing arrays", function() {
        expect(region.put('object containing array', { foo: [] })).toEqual( { foo: [] });
        expect(region.get('object containing array')).toEqual( { foo: [] });
      });

      it("stores and retrieves objects containing booleans", function() {
        expect(region.put('object containing boolean', { foo: true })).toEqual( { foo: true });
        expect(region.get('object containing boolean')).toEqual( { foo: true });
      });

      it("stores and retrieves objects containing integers", function() {
        expect(region.put('object containing integer', { foo: 123 })).toEqual( { foo: 123 });
        expect(region.get('object containing integer')).toEqual( { foo: 123 });
      });

      it("stores and retrieves objects containing floats", function() {
        expect(region.put('object containing float', { foo: 123.456 })).toEqual( { foo: 123.456 });
        expect(region.get('object containing float')).toEqual( { foo: 123.456 });
      });

      it("stores and retrieves objects containing dates", function() {
        var date = new Date();
        expect(region.put('object containing number', { foo: date })).toEqual( { foo: date });
        expect(region.get('object containing number')).toEqual( { foo: date });
      });

      it("stores and retrieves objects", function() {
        expect(region.put('object containing null', { foo: null })).toEqual( { foo: null });
        expect(region.get('object containing null')).toEqual( { foo: null });
      });

      it("stores and retrieves a stress test object", function() {
        var object = require("./fixtures/stress_test.json");
        expect(region.put('stress test', object)).toEqual(object);
        expect(region.get('stress test')).toEqual(object);
      });

      it("stores and retrieves objects with numerical keys", function() {
        region.put(1, "number key");
        region.put("1", "string key");

        expect(region.get(1)).toEqual("number key");
        expect(region.get("1")).toEqual("string key");
      });

      it("stores and retrieves objects with date keys", function() {
        var date = new Date();
        region.put(date, "date key");
        expect(region.get(date)).toEqual("date key");
      });

      it("stores and retrieves objects with boolean keys", function() {
        region.put(true, "true key");
        region.put(false, "false key");
        region.put("true", "true string key");
        region.put("false", "false string key");

        expect(region.get(true)).toEqual("true key");
        expect(region.get(false)).toEqual("false key");
        expect(region.get("true")).toEqual("true string key");
        expect(region.get("false")).toEqual("false string key");
      });
    });

    it("throws an error for unsupported values", function() {
      function putUndefined(){
        region.put("undefined", undefined);
      }

      expect(putUndefined).toThrow("Unable to put value undefined");
    });

    it("allows dynamic schema", function() {
      region.put('foo', {});
      expect(region.get('foo')).toEqual({});

      region.put('foo', { baz: 'qux' });
      expect(region.get('foo')).toEqual({ baz: 'qux' });

      region.put('foo', { baz: [] });
      expect(region.get('foo')).toEqual({ baz: [] });
    });
  });

  describe(".clear", function(){
    it("removes all keys", function(){
      region.put('key', 'value');
      expect(region.get('key')).toBeDefined();
      region.clear();
      expect(region.get('key')).toBeUndefined();
    });

    it("only removes keys for the region, not for other regions", function() {
      var region1 = cache.getRegion("exampleRegion");
      var region2 = cache.getRegion("anotherRegion");

      region1.clear();
      region2.clear();

      region1.put('key', 'value');
      region2.put('key', 'value');

      expect(region1.get('key')).toBeDefined();
      expect(region2.get('key')).toBeDefined();

      region1.clear();

      expect(region1.get('key')).not.toBeDefined();
      expect(region2.get('key')).toBeDefined();
    });
  });

  describe(".executeFunction", function() {
    const testFunctionName = "io.pivotal.node_gemfire.TestFunction";

    it("runs a function on the GemFire cluster and returns the results", function() {
      const results = region.executeFunction(testFunctionName);
      expect(results).toEqual(["TestFunction succeeded."]);
    });

    it("gives the function access to the region", function() {
      const functionName = "io.pivotal.node_gemfire.SumRegion";

      region.put("one", 1);
      region.put("two", 2);

      const anotherRegion = cache.getRegion("anotherRegion");
      anotherRegion.clear();
      anotherRegion.put("thousand", 1000);

      var results = region.executeFunction(functionName);
      expect(results).toEqual([3]);

      results = anotherRegion.executeFunction(functionName);
      expect(results).toEqual([1000]);
    });

    it("supports objects as input and output", function() {
      const results = region.executeFunction("io.pivotal.node_gemfire.Passthrough", { foo: 'bar' });
      expect(results).toEqual([{ foo: 'bar' }]);
    });

    it("runs a function on the GemFire cluster with arguments and returns the results", function() {
      const functionName = "io.pivotal.node_gemfire.Sum";

      var results = region.executeFunction(functionName, [1, 2]);
      expect(results).toEqual([3]);

      results = region.executeFunction(functionName, [1, 2, 3, 4]);
      expect(results).toEqual([10]);
    });

    it("throws an exception when no function name is passed in", function(){
      function callWithoutArgs() {
        region.executeFunction();
      }
      expect(callWithoutArgs).toThrow("You must provide the name of a function to execute.");
    });

    it("throws an exception when the function is not found", function(){
      function callNonexistentFunction() {
        region.executeFunction("com.example.Nonexistent");
      }
      expect(callNonexistentFunction).toThrow();
    });

    it("throws an exception when the function throws an exception", function() {
      var exception;

      try {
        region.executeFunction("io.pivotal.node_gemfire.TestFunctionException");
      } catch(e) {
        exception = e;
      }

      expect(exception).toBeDefined();
      expect(exception.message).toMatch(/Test exception message thrown by server./);
    });

    it("returns the results and an error when the function sends an exception with the results", function() {
      const results = region.executeFunction("io.pivotal.node_gemfire.TestFunctionExceptionResult");
      expect(results.length).toEqual(2);
      expect(results[0]).toEqual("First result");
      expect(results[1].message).toMatch("java.lang.Exception: Test exception message sent by server.");
    });

    describe("async", function() {
      it("returns the region object to support chaining", function(done) {
        var returnValue = region.executeFunction(testFunctionName, function(error, value) {
          done();
        });

        expect(returnValue).toEqual(region);
      });

      it("runs a function on the GemFire cluster and passes its result to the callback", function(done) {
        region.executeFunction(testFunctionName, function(error, results) {
          expect(error).toBeNull();
          expect(results).toEqual(["TestFunction succeeded."]);
          done();
        });
      });

      it("runs a function on the GemFire cluster and passes its result to the callback", function(done) {
        region.executeFunction("io.pivotal.node_gemfire.Sum", [1, 2, 3], function(error, results) {
          expect(error).toBeNull();
          expect(results).toEqual([6]);
          done();
        });
      });

      it("throws an exception when no function name is passed in, but a callback is", function() {
        function callWithoutArgs() {
          region.executeFunction(function(){});
        }

        expect(callWithoutArgs).toThrow("You must provide the name of a function to execute.");
      });

      it("passes an error into the callback when the function is not found", function(done){
        region.executeFunction("com.example.Nonexistent", function(error, results) {
          expect(error).not.toBeNull();
          expect(error.message).toEqual(
            "gemfire::Exception: Execute::GET_FUNCTION_ATTRIBUTES: message from server could not be handled"
          );
          expect(results).toBeUndefined();
          done();
        });
      });

      it("passes an error into the callback when the function throws an exception", function(done) {
        region.executeFunction("io.pivotal.node_gemfire.TestFunctionException", function(error, results) {
          expect(error).not.toBeNull();
          expect(error.message).toMatch(
            /com.gemstone.gemfire.cache.execute.FunctionException: Test exception message thrown by server./
          );
          expect(results).toBeUndefined();
          done();
        });
      });

      it("passes the results and an error when the function sends an exception with the results", function(done) {
        region.executeFunction("io.pivotal.node_gemfire.TestFunctionExceptionResult", function(error, results) {
          expect(error).not.toBeNull();
          expect(error.message).toMatch("java.lang.Exception: Test exception message sent by server.");
          expect(results.length).toEqual(1);
          expect(results[0]).toEqual("First result");
          done();
        });
      });
    });
  });
});

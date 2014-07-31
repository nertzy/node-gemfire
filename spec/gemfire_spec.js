describe("pivotal-gemfire", function() {
  var gemfire;

  beforeEach(function() {
    gemfire = require("../gemfire.js");
    gemfire.clear();
  });

  describe(".version", function() {
    it("returns the version string", function() {
      expect(gemfire.version()).toEqual("7.0.2.0");
    });
  });

  describe(".put/.get", function() {
    describe("for a key that is not present", function () {
      it("returns undefined", function() {
        expect(gemfire.get('foo')).toBeUndefined();
      });
    });

    describe("with objects", function() {
      it("returns the value and stores the object in the cache", function() {
        expect(gemfire.put("foo", { foo: "bar" })).toEqual({ foo: "bar" });
        expect(gemfire.get("foo")).toEqual({ foo: "bar" });
      });
    });

    describe("with strings", function () {
      it("returns the value and stores the object in the cache", function() {
        expect(gemfire.put("foo", "bar")).toEqual("bar");
        expect(gemfire.get("foo")).toEqual("bar");
      });
    });
  });

  describe(".onPut", function() {
    it("fires the callback function when a put occurs", function() {
      var key = Date.now().toString();

      var callback = jasmine.createSpy();

      pivotalGemfire.onPut(callback);
      expect(callback).not.toHaveBeenCalled();

      pivotalGemfire.put(key, "bar");
      expect(callback).toHaveBeenCalledWith(key, "bar");

      pivotalGemfire.put(key, "baz");
      expect(callback).toHaveBeenCalledWith(key, "baz");
    });
  });

  describe("cleanup", function(){
    it("is not actually a test", function(){
      gemfire.close();
    });
  });
});

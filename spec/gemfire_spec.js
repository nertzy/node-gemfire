describe("pivotal-gemfire", function() {
  var pivotalGemfire;

  beforeEach(function() {
    pivotalGemfire = require("../gemfire.js");
    pivotalGemfire.clear();
  });

  describe(".version", function() {
    it("returns the version string", function() {
      expect(pivotalGemfire.version()).toEqual("7.0.2.0");
    });
  });

  describe(".put/.get", function() {
    describe("with objects", function() {
      it("returns the value and stores the object in the cache", function() {
        expect(pivotalGemfire.put("foo", { foo: "bar" })).toEqual({ foo: "bar" });
        expect(pivotalGemfire.get("foo")).toEqual({ foo: "bar" });
      });
    });

    describe("with strings", function () {
      it("returns the value and stores the object in the cache", function() {
        expect(pivotalGemfire.put("foo", "bar")).toEqual("bar");
        expect(pivotalGemfire.get("foo")).toEqual("bar");
      });
    });
  });

  describe("cleanup", function(){
    it("is not actually a test", function(){
      pivotalGemfire.close();
    });
  });
});

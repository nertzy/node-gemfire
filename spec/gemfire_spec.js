var childProcess = require('child_process');

describe("pivotal-gemfire", function() {
  var gemfire;

  beforeEach(function() {
    gemfire = require("../gemfire.js");
    gemfire.clear();
  });

  afterEach(function(done) {
    setTimeout(done, 0);
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
    describe("for puts triggered locally", function() {
      var key = Date.now().toString();

      var callback1 = jasmine.createSpy();
      var callback2 = jasmine.createSpy();

      beforeEach(function(done) {
        gemfire.onPut(callback1);
        gemfire.onPut(callback2);
        setTimeout(done, 0);
      });

      beforeEach(function() {
        expect(callback1).not.toHaveBeenCalled();
        expect(callback2).not.toHaveBeenCalled();
      });

      beforeEach(function(done) {
        gemfire.put(key, "bar");
        setTimeout(done, 0);
      });

      beforeEach(function() {
        expect(callback1).toHaveBeenCalledWith(key, "bar");
        expect(callback2).toHaveBeenCalledWith(key, "bar");
      });

      beforeEach(function(done) {
        gemfire.put(key, "baz");
        setTimeout(done, 0);
      });

      it("fires the callback function when a put occurs", function() {
        expect(callback1).toHaveBeenCalledWith(key, "baz");
        expect(callback2).toHaveBeenCalledWith(key, "baz");
      });
    });

    describe("for puts triggered externally", function() {
      var firedCallback;

      beforeEach(function(done) {
        firedCallback = false;

        gemfire.onPut(function(key, value){
          expect([key, value]).toEqual(["async", "test"]);
          firedCallback = true;

          done();
        });

        childProcess.execFile("node", ["spec/support/external_put.js"], function(error, stdout, stderr) {
          if(error) {
            console.error(stderr);
            expect(error).toBeNull();
            done();
          }
        });
      });

      it("fires the callback function", function() {
        expect(firedCallback).toBeTruthy();
      });
    });
  });

  describe("cleanup", function(){
    it("is not actually a test", function(){
      gemfire.close();
    });
  });
});

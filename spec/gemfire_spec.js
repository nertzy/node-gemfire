const gemfire = require("./support/gemfire.js");

describe("gemfire", function() {
  describe(".version", function() {
    it("returns the version string", function() {
      expect(gemfire.version()).toEqual("7.0.2.0");
    });
  });
});

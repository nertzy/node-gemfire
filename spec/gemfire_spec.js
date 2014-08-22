const gemfire = require("./support/gemfire.js");

describe("gemfire", function() {
  describe(".version", function() {
    it("returns the version from package.json", function() {
      var package = require("../package.json");
      expect(gemfire.version).toEqual(package.version);
    });
  });

  describe(".gemfireVersion", function() {
    it("returns the version string from the GemFire native client", function() {
      expect(gemfire.gemfireVersion).toEqual("7.0.2.0");
    });
  });
});

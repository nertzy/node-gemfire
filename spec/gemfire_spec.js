const gemfire = require("./support/gemfire.js");
const cache = require("./support/factories.js").getCache();
const expectExternalSuccess = require("./support/external_scripts.js").expectExternalSuccess;

describe("gemfire", function() {
  describe(".version", function() {
    it("returns the version from package.json", function() {
      var package = require("../package.json");
      expect(gemfire.version).toEqual(package.version);
    });
  });

  describe(".gemfireVersion", function() {
    it("returns the version string from the GemFire native client", function() {
      expect(gemfire.gemfireVersion).toEqual("8.0.0.1");
    });
  });

  describe(".connected", function() {
    it("returns true if the client is connected to the GemFire system", function() {
      expect(gemfire.connected()).toBeTruthy();
    });

    it("returns false if the client is not connected to the GemFire system", function(done) {
      expectExternalSuccess("not_connected", function(error, stdout) {
        expect(stdout.trim()).toEqual("false");
        done();
      });
    });
  });
});

const childProcess = require('child_process');

const errorMatchers = require("./error_matchers");

function runExternalTest(name, callback) {
  if(!callback) { throw("You must pass a callback");  }

  var filename = "spec/support/scripts/" + name + ".js";

  childProcess.execFile("node", [filename], callback);
}

exports.expectExternalSuccess = function expectExternalSuccess(name, callback){
  jasmine.addMatchers(errorMatchers);

  runExternalTest(name, function(error, stdout, stderr) {
    expect(error).not.toBeError();
    expect(stderr).toEqual('');
    callback(error, stdout);
  });
};

exports.expectExternalFailure = function expectExternalFailure(name, callback, message){
  jasmine.addMatchers(errorMatchers);

  runExternalTest(name, function(error, stdout, stderr) {
    expect(error).toBeError();
    expect(stderr).toContain(message);
    callback(error, stdout, stderr);
  });
};

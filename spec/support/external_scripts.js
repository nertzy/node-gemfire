const childProcess = require('child_process');

function runExternalTest(name, done, callback) {
  if(!done) { throw("You must run this test asynchronously and pass done");  }
  if(!callback) { throw("You must pass a callback");  }

  var filename = "spec/support/scripts/" + name + ".js";

  childProcess.execFile("node", [filename], function(error, stdout, stderr) {
    callback(error, stdout, stderr);
    done();
  });
}

exports.expectExternalSuccess = function expectExternalSuccess(name, done){
  runExternalTest(name, done, function(error, stdout, stderr) {
    expect(error).not.toBeError();
    expect(stderr).toEqual('');
  });
};

exports.expectExternalFailure = function expectExternalFailure(name, done, message){
  runExternalTest(name, done, function(error, stdout, stderr) {
    expect(error).toBeError();
    expect(stderr).toContain(message);
  });
};

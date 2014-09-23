const nodePreGyp = require('node-pre-gyp');
const path = require('path');
const EventEmitter = require('events').EventEmitter;

function inherits(target, source) {
  for (var key in source.prototype) {
    target.prototype[key] = source.prototype[key];
  }
}

module.exports = function binding(options) {
  const bindingPath = nodePreGyp.find(
    path.resolve(path.join(__dirname,'../package.json')),
    options || {}
  );
  const gemfire = require(bindingPath);

  inherits(gemfire.Region, EventEmitter);

  return gemfire;
};

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
  const initialize = require(bindingPath).initialize;

  const gemfire = initialize({
    EventEmitter: EventEmitter,
    process: process
  });

  var xmlFilePath;
  gemfire.configure = function configure(path) {
    xmlFilePath = path;
  };

  var cacheSingleton;
  const Cache = gemfire.Cache;

  gemfire.getCache = function getCache() {
    if(!xmlFilePath) {
      throw "gemfire: You must call configure() before calling getCache().";
    }

    if(!cacheSingleton) {
      cacheSingleton = new Cache(xmlFilePath);
    }
    return cacheSingleton;
  };

  delete gemfire.Cache;

  inherits(gemfire.Region, EventEmitter);
  delete gemfire.Region;

  return gemfire;
};

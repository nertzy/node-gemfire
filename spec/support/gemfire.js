var nodePreGyp = require('node-pre-gyp');
var path = require('path');
var bindingPath = nodePreGyp.find(
  path.resolve(path.join(__dirname,'../../package.json')),
  { "debug": true }
);
module.exports = require(bindingPath);

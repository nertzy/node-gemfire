var nodePreGyp = require('node-pre-gyp');
var path = require('path');
var pivotalGemfirePath = nodePreGyp.find(
  path.resolve('package.json')
);

module.exports = require(pivotalGemfirePath);

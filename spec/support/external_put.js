var nodePreGyp = require('node-pre-gyp');
var path = require('path')
var pivotalGemfirePath = nodePreGyp.find(
  path.resolve(path.join(__dirname,'../../package.json'))
);
var pivotalGemfire = require(pivotalGemfirePath);

pivotalGemfire.put("async", "test");

pivotalGemfire.close();

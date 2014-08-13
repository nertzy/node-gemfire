var gemfire = require('../../gemfire.js');
var cache = new gemfire.Cache();
var region = cache.getRegion("exampleRegion");

region.put("async", "test");

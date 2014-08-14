var cache = require("./factories.js").getCache();
var region = cache.getRegion("exampleRegion");

region.put("async", "test");

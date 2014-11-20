var cache = require("../factories.js").getCache();
var region = cache.createRegion("shadow");
region.put("proxy", true);

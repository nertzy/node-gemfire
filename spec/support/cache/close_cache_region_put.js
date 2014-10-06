const _ = require("lodash");

const cache = require("../factories.js").getCache();
const region = cache.getRegion('exampleRegion');

cache.close();

region.put("foo", "bar");

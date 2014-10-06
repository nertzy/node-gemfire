#!/usr/bin/env node

const _ = require("lodash");
const repl = require("repl");

(function(){
  const gemfire = require("../spec/support/gemfire.js");
  const cache = new gemfire.Cache("benchmark/xml/BenchmarkClient.xml");
  const region = cache.getRegion("exampleRegion");

  region.on('error', console.log);

  const consoleRepl = repl.start({
    prompt: "node_gemfire> ",
    terminal: true
  });

  _.extend(consoleRepl.context, {
    gemfire: gemfire,
    cache: cache,
    region: region
  });

  consoleRepl.on('exit', process.exit);
})();

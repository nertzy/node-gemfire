#!/usr/bin/env node

gemfire = require("..");
cache = new gemfire.Cache("benchmark/xml/BenchmarkClient.xml");
region = cache.getRegion("exampleRegion");

var repl = require("repl");
repl.start({
  prompt: "node_gemfire> ",
  terminal: true
}).on('exit', process.exit);


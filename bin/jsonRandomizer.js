#!/usr/bin/env node

var _ = require('lodash');
var randomString = require('random-string');

var inputFileName = process.argv[2];
var inputJson = require(require('path').resolve(inputFileName));

var outputJson = randomizeJson(inputJson);
var outputFileName = inputFileName.replace(".json", ".randomized.json");

var fs = require('fs');

fs.writeFile(outputFileName, JSON.stringify(outputJson, null, 4), function(error) {
  if(error) {
    console.log("Failed to write output.", error);
  } else {
    console.log("Randomized JSON written to " + outputFileName);
  }
});

function randomizeJson(inputJson) {
  var outputJson = {};

  for(key in inputJson) {
    var randomKey = randomString({
      length: key.length,
      numeric: false,
      letters: true,
      special: false
    });

    outputJson[randomKey] = randomizeValue(inputJson[key]);
  }

  return outputJson;
}

function randomizeValue(input) {
  if(input === null) {
    return null;
  }
  if(input === undefined) {
    return undefined;
  }
  if(input === true) {
    return true;
  }
  if(input === false) {
    return false;
  }
  if(typeof input == 'number') {
    // TODO: randomize number
    return input;
  }
  if(typeof input == 'string') {
    return randomString({
      length: input.length,
      numeric: false,
      letters: true,
      special: false
    });
  }
  if(typeof input == 'object') {
    if(input instanceof Array) {
      return _.map(input, function(subInput){
        return randomizeValue(subInput);
      });
    }
    else {
      return randomizeJson(input);
    }
  }

  throw "Can't randomize value: " + input;
}

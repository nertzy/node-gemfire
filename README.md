gemfire NodeJS addon
====================

Proof-of-concept NodeJS addon for connecting to Pivotal Gemfire

## Usage

```javascript
var gemfire = require('gemfire');
var cache = new gemfire.Cache('config/cache.xml');
var region = cache.getRegion('myRegion');

region.put('foo', { bar: ['baz', 'qux'] });
region.get('foo');

cache.executeQuery("SELECT DISTINCT * FROM /exampleRegion");
region.clear();
```

## Prerequisites 

* node 0.10.x http://nodejs.org/
* vagrant 1.6.x http://www.vagrantup.com/

## Setup
Build out the virtualbox and install nodejs, Gemfire, etc.

    $ vagrant up

## Developer workflow

This directory is mounted on the VM as `/project`. You can make edits here or on the VM.

    $ vagrant ssh
    $ cd /project

## Rebuild the node module and run Jasmine tests

    $ vagrant ssh
    $ cd /project
    $ grunt

## Run every benchmark

    $ vagrant ssh
    $ cd /project
    $ grunt benchmark

## Run benchmarks individually

    $ vagrant ssh
    $ cd /project
    $ grunt benchmark:node
    $ grunt benchmark:java

## Server Management

The gemfire server should be automatically started for you as part of the above tasks. If you
need to restart it manually, use the following:

    $ vagrant ssh
    $ grunt server:restart # or server:start or server:stop

## License

See LICENSE file for details

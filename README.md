pivotal-gemfire
===============

Proof-of-concept node module connecting to Pivotal Gemfire

## Prerequisites 

* node 0.10.x http://nodejs.org/
* vagrant 1.6.x http://www.vagrantup.com/

## Setup
Build out the virtualbox and install nodejs, Gemfire, etc.

    $ vagrant up

## Developer workflow

You will need the Gemfire server running while you run tests and develop.

    $ vagrant ssh
    $ grunt server:start # or server:stop or server:restart

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

## License

See LICENSE file for details

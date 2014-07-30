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

This directory is mounted on the VM as `/project`. You can make edits here or on the VM.

    $ vagrant ssh
    $ cd /project

## Running the node.js benchmarks

    $ vagrant ssh
    $ grunt
    $ cd /project/benchmark/node
    $ node benchmark.js

## Running the java benchmarks

    $ vagrant ssh
    $ cd /project/benchmark/java
    $ gfsh start server --cache-xml-file=xml/BenchmarkServer.xml --name=exampleServer
    $ ./gradlew clean run
    $ gfsh stop server --dir=/project/benchmark/java/exampleServer

## License

See LICENSE file for details

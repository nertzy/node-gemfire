module.exports = function(grunt) {
  var startServer = 'cd tmp/gemfire && gfsh start server --dir=server --log-level=warning --cache-xml-file=../../benchmark/xml/BenchmarkServer.xml --name=server';
  var ensureServerRunning = 'test -e tmp/gemfire/server/vf.gf.server.pid && ps ax | grep `cat tmp/gemfire/server/vf.gf.server.pid` | grep -qv grep && echo "Server already running..." || (' + startServer + ')';

  var nodeCommand = "node";

  function runNode(args) {
    return(function() { return nodeCommand + " " + args; });
  }

  grunt.initConfig(
    {
      pkg: grunt.file.readJSON('package.json'),
      shell: {
        build: {
          command: './node_modules/.bin/node-pre-gyp build'
        },
        rebuild: {
          command: './node_modules/.bin/node-pre-gyp rebuild'
        },
        benchmarkNode: {
          command: runNode('benchmark/node/benchmark.js')
        },
        ensureServerRunning: {
          command: ensureServerRunning
        },
        startServer: {
          command: startServer
        },
        stopServer: {
          command: 'cd tmp/gemfire && gfsh stop server --dir=server',
        },
        benchmarkJava: {
          command: [
            'cd benchmark/java',
            './gradlew clean run -q'
          ].join(" && ")
        },
        lint: {
          command: "cpplint.py --verbose=1 --linelength=110 --extensions=cpp,hpp src/*"
        },
        console: {
          command: runNode("bin/console.js")
        },
        cppUnitTests: {
          command: runNode("spec/cpp/runner.js")
        }
      },
      jasmine_node: {
        all: ['spec/']
      },
      jshint: {
        options: {
          jshintrc: true
        },
        all: ['Gruntfile.js', './*.js', 'spec/**/*.js']
      }
    }
  );

  grunt.loadNpmTasks('grunt-contrib-jshint');
  grunt.loadNpmTasks('grunt-jasmine-node');
  grunt.loadNpmTasks('grunt-shell');

  grunt.registerTask('build', ['shell:build']);
  grunt.registerTask('rebuild', ['shell:rebuild']);
  grunt.registerTask('test', ['shell:cppUnitTests', 'server:ensure', 'jasmine_node:all']);
  grunt.registerTask('lint', ['shell:lint']);
  grunt.registerTask('console', ['shell:console']);

  grunt.registerTask('server:start', ['shell:startServer']);
  grunt.registerTask('server:stop', ['shell:stopServer']);
  grunt.registerTask('server:restart', ['server:stop', 'server:start']);
  grunt.registerTask('server:ensure', ['shell:ensureServerRunning']);

  grunt.registerTask('benchmark:node', ['build', 'server:ensure', 'shell:benchmarkNode']);
  grunt.registerTask('benchmark:java', ['server:ensure', 'shell:benchmarkJava']);

  grunt.registerTask('valgrind', ['shell:rebuild'], function() {
    grunt.log.writeln('Running with valgrind...');
    nodeCommand = "valgrind --suppressions=spec/valgrind/suppressions-gemfire-7.0.2.supp node";
  });

  grunt.registerTask('benchmark', ['benchmark:node', 'benchmark:java']);

  grunt.registerTask('default', ['build', 'test', 'lint', 'jshint']);
};


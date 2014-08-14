module.exports = function(grunt) {
  var startServer = 'cd tmp/gemfire && gfsh start server --dir=server --log-level=warning --cache-xml-file=../../benchmark/xml/BenchmarkServer.xml --name=server';
  var ensureServerRunning = 'test -e tmp/gemfire/server/vf.gf.server.pid && ps ax | grep `cat tmp/gemfire/server/vf.gf.server.pid` | grep -qv grep && echo "Server already running..." || (' + startServer + ')';

  grunt.initConfig(
    {
      pkg: grunt.file.readJSON('package.json'),
      shell: {
        rebuild: {
          command: 'npm install --build-from-source'
        },
        benchmarkNode: {
          command: 'node benchmark/node/benchmark.js'
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
          command: "node bin/console.js"
        }
      },
      jasmine_node: {
        all: ['spec/']
      }
    }
  );

  grunt.loadNpmTasks('grunt-jasmine-node');
  grunt.loadNpmTasks('grunt-shell');

  grunt.registerTask('build', ['shell:rebuild']);
  grunt.registerTask('test', ['server:ensure', 'jasmine_node:all']);
  grunt.registerTask('lint', ['shell:lint']);
  grunt.registerTask('console', ['shell:console']);

  grunt.registerTask('server:start', ['shell:startServer']);
  grunt.registerTask('server:stop', ['shell:stopServer']);
  grunt.registerTask('server:restart', ['server:stop', 'server:start']);
  grunt.registerTask('server:ensure', ['shell:ensureServerRunning']);

  grunt.registerTask('benchmark:node', ['build', 'server:ensure', 'shell:benchmarkNode']);
  grunt.registerTask('benchmark:java', ['server:ensure', 'shell:benchmarkJava']);

  grunt.registerTask('benchmark', ['benchmark:node', 'benchmark:java']);

  grunt.registerTask('default', ['build', 'test', 'lint']);
};


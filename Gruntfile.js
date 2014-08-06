module.exports = function(grunt) {

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
        startServer: {
          command: 'cd tmp/gemfire && gfsh start server --dir=server --log-level=warning --cache-xml-file=../../benchmark/xml/BenchmarkServer.xml --name=server'
        },
        stopServer: {
          command: 'cd tmp/gemfire && gfsh stop server --dir=server',
        },
        benchmarkJava: {
          command: [
            'cd benchmark/java',
            './gradlew clean run -q'
          ].join(" && ")
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
  grunt.registerTask('test', ['jasmine_node:all']);

  grunt.registerTask('server:start', ['shell:startServer']);
  grunt.registerTask('server:stop', ['shell:stopServer']);
  grunt.registerTask('server:restart', ['server:stop', 'server:start']);

  grunt.registerTask('benchmark:node', ['build', 'shell:benchmarkNode']);
  grunt.registerTask('benchmark:java', ['shell:benchmarkJava']);

  grunt.registerTask('benchmark', ['benchmark:node', 'benchmark:java']);

  grunt.registerTask('default', ['build', 'test']);
};


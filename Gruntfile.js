module.exports = function(grunt) {
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    shell: {
      rebuild: {
        command: 'npm install --build-from-source'
      },
      benchmarkNode: {
        command: 'node benchmark/node/benchmark.js'
      },
      startServer: {
        command: 'cd tmp/gemfire && gfsh start server --dir=server --cache-xml-file=../../benchmark/xml/BenchmarkServer.xml --name=server'
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
      benchmarkCpp: {
        command: 'cd benchmark/cpp && make test'
      }
    },
    jasmine_node: {
      all: ['spec/']
    }
  });
  grunt.loadNpmTasks('grunt-jasmine-node');
  grunt.loadNpmTasks('grunt-shell');

  grunt.registerTask('build', ['shell:rebuild']);
  grunt.registerTask('test', ['server:restart', 'jasmine_node:all', 'shell:stopServer']);

  grunt.registerTask('server:restart', ['shell:stopServer', 'shell:startServer']);

  grunt.registerTask('benchmark:node', ['build', 'server:restart', 'shell:benchmarkNode', 'shell:stopServer']);
  grunt.registerTask('benchmark:java', ['server:restart', 'shell:benchmarkJava', 'shell:stopServer']);
  grunt.registerTask('benchmark:cpp', ['server:restart', 'shell:benchmarkCpp', 'shell:stopServer']);
  grunt.registerTask('benchmark', ['benchmark:node', 'benchmark:java', 'benchmark:cpp']);

  grunt.registerTask('default', ['build', 'test']);
};


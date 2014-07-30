module.exports = function(grunt) {
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    shell: {
      rebuild: {
        command: 'npm install --build-from-source'
      },
      benchmarkNode: {
        command: 'node /project/benchmark/node/benchmark.js'
      },
      startServer: {
        command: 'gfsh start server --dir=/project/exampleServer --cache-xml-file=/project/benchmark/xml/BenchmarkServer.xml --name=exampleServer'
      },
      stopServer: {
        command: 'gfsh stop server --dir=/project/exampleServer',
      },
      benchmarkJava: {
        command: [
          'cd /project/benchmark/java',
          './gradlew clean run'
        ].join(" && ")
      },
      benchmarkCpp: {
        command: 'cd /project/benchmark/cpp && make test'
      }
    },
    jasmine_node: {
      all: ['spec/']
    }
  });
  grunt.loadNpmTasks('grunt-jasmine-node');
  grunt.loadNpmTasks('grunt-shell');

  grunt.registerTask('build', ['shell:rebuild']);
  grunt.registerTask('test', ['jasmine_node:all']);

  grunt.registerTask('benchmark:node', ['build', 'shell:stopServer', 'shell:startServer', 'shell:benchmarkNode', 'shell:stopServer']);
  grunt.registerTask('benchmark:java', ['shell:stopServer', 'shell:startServer', 'shell:benchmarkJava', 'shell:stopServer']);
  grunt.registerTask('benchmark:cpp', ['shell:benchmarkCpp']);
  grunt.registerTask('benchmark', ['benchmark:node', 'benchmark:java', 'benchmark:cpp']);

  grunt.registerTask('default', ['build', 'test']);
};


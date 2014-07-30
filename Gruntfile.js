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
      benchmarkJava: {
        command: [
          'cd /project/benchmark/java',
          'gfsh start server --cache-xml-file=xml/BenchmarkServer.xml --name=exampleServer',
          './gradlew clean run',
          'gfsh stop server --dir=/project/benchmark/java/exampleServer',
        ].join(" && ")
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

  grunt.registerTask('benchmark:node', ['build', 'shell:benchmarkNode']);
  grunt.registerTask('benchmark:java', ['shell:benchmarkJava']);
  grunt.registerTask('benchmark', ['benchmark:node', 'benchmark:java']);

  grunt.registerTask('default', ['build', 'test']);
};


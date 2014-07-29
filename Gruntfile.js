module.exports = function(grunt) {
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    shell: {
      rebuild: {
        command: 'npm install --build-from-source'
      },
      benchmark: {
        command: 'node benchmark/node/benchmark.js'
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
  grunt.registerTask('benchmark', ['shell:benchmark']);

  grunt.registerTask('default', ['build', 'test', 'benchmark']);
};


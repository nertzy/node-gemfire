module.exports = function(grunt) {
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    shell: {
      rebuild: {
        command: 'npm install --build-from-source'
      },
      test: {
        command: 'node test.js'
      }
    },
  });
  grunt.loadNpmTasks('grunt-shell');
  grunt.registerTask('default', ['shell:rebuild', 'shell:test']);
};


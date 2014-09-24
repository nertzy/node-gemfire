# Contributing to Node Gemfire

## Contributor License Agreement

Follow these steps to make a contribution to any of our open source repositories:

1. Ensure that you have completed our CLA Agreement (coming soon).
1. Set your name and email. These should match the information on your submitted CLA.

        git config --global user.name "Firstname Lastname"
        git config --global user.email "your_email@example.com"

## General Workflow

Follow these steps to make a contribution to any of our open source repositories:

1. Fork the repository
1. Create a feature branch (`git checkout -b my_branch_name`)
1. Run the quick test suite to ensure that your local environment is working (`grunt`)
1. Make changes on the branch:
    * Adding a feature
      1. Add specs for the new feature
      1. Make the specs pass
      1. Update documentation to reflect any changes to user-facing features
    * Fixing a bug
      1. Add specs to exercise the bug
      1. Fix the bug, making the specs pass
    * Refactoring existing functionality
      1. Change the implementation
      1. Ensure that specs still pass
        * If you find yourself changing specs after a refactor, consider
          refactoring the specs first
1. Run the full test suite (`grunt ci`)
1. Push to your fork (`git push origin my_branch_name`) and submit a pull request

We favor pull requests with very small, single commits with a single purpose.

Your pull request is much more likely to be accepted if:

* Your pull request includes tests
* Your pull request is small and focused with a clear message that conveys the intent of your change.

## Code Style

To increase the chances of a pull request being merged, we recommend that you follow our style for any changes:

* Follow the conventions suggested by Joyent's [NodeJS Production Practices](https://www.joyent.com/developers/node/design).

* Use [Native Abstractions for Node.js](https://github.com/rvagg/nan) where possible to improve compatibility between versions of NodeJS.

* Add test cases for new or improved behavior. We use the folowing test libraries:
  * [Jasmine](https://github.com/pivotal/jasmine) for feature specs in JavaScript
  * [GoogleTest](https://code.google.com/p/googletest/) for unit tests in C++

* Check code style using `grunt lint`. Make sure your changes don't add any warnings or errors. We use the following tools for automated code style enforcement:
  * [JSHint](http://www.jshint.com/) for JavaScript
  * Google's [cpplint.py](http://google-styleguide.googlecode.com/svn/trunk/cpplint/cpplint.py) to follow the [Google C++ Style Guide](http://google-styleguide.googlecode.com/svn/trunk/cppguide.html)

* Minimize dependencies, especially those required by end users. Also don't introduce dependencies with restrictive licenses.

* Avoid introducing memory leaks or unnecessarily inefficient operations. We use the following tools to detect or debug issues like these:
  * Valgrind (Memcheck) (`grunt valgrind benchmark:node`) 
  * Callgrind (`grunt callgrind benchmark:node`)
  * gperftools (`grunt gperftools benchmark:node`)

* If you allocate V8 handles, you should do so inside of a HandleScope using NanScope()/NanEscapableScope() so that the handles will be freed as soon as possible.

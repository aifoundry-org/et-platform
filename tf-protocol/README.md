# tf-protocol

Esperanto Test Framework Project

## Versioning

This project is versioned using semantic versioning (see https://semver.org/). 

The version is specified in the top-level `CMakeLists.txt`. Here:
```
cmake_minimum_required(VERSION 3.5)
project(tf-protocol VERSION 0.1.0 DESCRIPTION "Esperanto Test Framework Project" LANGUAGES C CXX)
                            ^^^^^
```

### Changelog

When raising the project version, do not forget to update `CHANGELOG.md`. Rename `Unreleased` section to the new version, and create a new "Unreleased" section that will contain comments for next version.

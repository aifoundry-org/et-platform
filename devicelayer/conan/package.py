#!/usr/bin/env python
# -*- coding: utf-8 -*-
from cpt.packager import ConanMultiPackager
import os

if __name__ == "__main__":
    test_folder = os.path.join("conan", "test_package")
    builder = ConanMultiPackager(test_folder=test_folder)
    builder.add_common_builds(pure_c=False)
    builder.remove_build_if(lambda build: build.settings["compiler"] == "clang" and build.options["deviceLayer:shared"] == True)
    builder.run()
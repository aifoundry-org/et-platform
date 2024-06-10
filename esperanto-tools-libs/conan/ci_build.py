#!/usr/bin/env python3
import os

from ecpt.packager import Packager


def main():
    conanfile_path = os.path.join(os.path.dirname(__file__), "..", "conanfile.py")
    
    build = Packager(ci_build=True)
    build.add_package(conanfile_path)
    c3 = build.add_configuration("default", "linux-ubuntu22.04-x86_64-gcc11-release")
    # build.add_consumer("rt/0.12.0@", config_ids=[c1])
    build.report()

    build.run()


if __name__ == '__main__':
    main()

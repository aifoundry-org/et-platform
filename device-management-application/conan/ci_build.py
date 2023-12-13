#!/usr/bin/env python3
import os

from ecpt.packager import Packager


def main():
    conanfile_path = os.path.join(os.path.dirname(__file__), "..", "conanfile.py")
    
    build = Packager(ci_build=True)
    build.add_package(conanfile_path)
    #build.add_consumer("ss/0.11.0@")
    build.add_configuration("default", "linux-ubuntu18.04-x86_64-gcc7-release")
    build.add_configuration("default", "linux-ubuntu22.04-x86_64-gcc11-release")
    build.report()

    build.run()


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
import os

from ecpt.packager import Packager


def main():
    conanfile_path = os.path.join(os.path.dirname(__file__), "..", "conanfile.py")

    build = Packager(ci_build=True)
    build.add_package(conanfile_path)
    c1 = build.add_configuration("default", "linux-ubuntu18.04-x86_64-gcc7-release")
    # TODO: Enable this when issue regarding lockfiles is resolved.
    #c2 = build.add_configuration("default", "baremetal-rv64-gcc8.2-release")
    #build.add_consumer("rt/0.12.0@", config_ids=[c1])
    #build.add_consumer("ss/0.11.0@", config_ids=[c1, c2])
    #build.add_consumer("ss/0.11.0@", config_ids=[c1])
    build.report()

    build.run()


if __name__ == '__main__':
    main()

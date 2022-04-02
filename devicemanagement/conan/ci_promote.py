#!/usr/bin/env python3
import os

from ecpt.packager import Packager


def main():
    conanfile_path = os.path.join(os.path.dirname(__file__), "..", "conanfile.py")
    
    build = Packager()
    build.promote("lockfiles_info.json")


if __name__ == '__main__':
    main()

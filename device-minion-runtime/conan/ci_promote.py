#!/usr/bin/env python3
from ecpt.packager import Packager


def main():
    build = Packager()
    build.promote("lockfiles_info.json")


if __name__ == '__main__':
    main()

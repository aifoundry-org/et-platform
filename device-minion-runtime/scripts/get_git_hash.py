#!/usr/bin/python3

import sys, os, subprocess

# the maximum length of the git hash is 32 bytes
# which corresponds to the length of a SHA2-256 hash
max_git_hash_length = 32

def make_array(strval):
    result = "{ "
    hexbytes = bytes.fromhex(strval)
    for n in range(max_git_hash_length):
        if n > 0:
            result = result + ", "
        
        if n < len(hexbytes):
            result = result + "0x{0:02x}".format(hexbytes[n])
        else:
            result = result + "0x00"

    result = result + " }"
    return result

def usage():
    print("Usage:", sys.argv[0], "[-a] <git_repository_path>")
    sys.exit(-1)

if len(sys.argv) < 2:
    usage()

if "-a" == sys.argv[1]:
    if len(sys.argv) < 3:
        usage()
    array_mode = True
    git_repository_path = sys.argv[2]
else:
    array_mode = False
    git_repository_path = sys.argv[1]

old_dir = os.getcwd()
os.chdir(git_repository_path)

git_hash = subprocess.run(['git', 'rev-parse', 'HEAD'], stdout=subprocess.PIPE).stdout.decode('utf-8').strip()

os.chdir(old_dir)

if array_mode:
    print(make_array(git_hash), end='')
else:
    print("\"{0}\"".format(git_hash), end='')

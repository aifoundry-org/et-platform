#!/usr/bin/python3

import sys, os, subprocess
from datetime import datetime, timezone

# the maximum array length must match the MAX_GIT_VERSION_LENGTH defined
# in firmware-tools/code-signing-tools/crypto-library/include/esperanto-executable-image.h
max_git_version_length = 112 

def make_array(strval, length):
    result = "{ "
    for n in range(length):
        if n > 0:
            result = result + ", "
        
        if n < len(strval):
            result = result + "'" + strval[n] + "'"
        else:
            result = result + "0"
    
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

git_version = subprocess.run(['git', 'describe', '--dirty', '--always', '--tags'], stdout=subprocess.PIPE).stdout.decode('utf-8').strip()

os.chdir(old_dir)

if git_version.endswith("-dirty"):
    local_time = datetime.now(timezone.utc).astimezone()
    git_version = "{0}_{1}".format(git_version, local_time.isoformat(sep='_', timespec='seconds'))
 
if array_mode:
    print(make_array(git_version, max_git_version_length), end='')
else:
    print("\"{0}\"".format(git_version), end='')

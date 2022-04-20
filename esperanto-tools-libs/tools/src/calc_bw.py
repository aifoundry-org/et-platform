#!/usr/bin/python3

import subprocess as proc
import sys
import os
import locale

transfers = [
(602112,	4000),
(1204224,	8000),
(2408448,	16000),
(4816896,	32000),
(301056,	4000),
(602112,	8000),
(1204224,	16000),
(2408448,	32000),
(150528,	4000),
(301056,	8000),
(602112,	16000),
(1204224,	32000),
(2408448,	64000),
(9216,	3072),
(18432,	6144),
(27648,	9216),
(36864,	12288)]

h2d53 = [
(424,	4),
(54272,	512),
(434176,	4096),
(868352,	8192)]


execPath = ""

def bench(h2d, d2h, dmask=1, th=2, wl=10, numh2d=1, numd2h=1, cma_size=1<<30, dl=2):
  return proc.run(env={'ET_CMA_SIZE': str(cma_size)}, executable=execPath, encoding=locale.getpreferredencoding(), stdout=proc.PIPE,
  args='-json -wl {wl} -th {th} -h2d {h2d} -d2h {d2h} -th {th} -numh2d {numh2d} -numd2h {numd2h} -dmask {dmask} -deviceLayer {dl}'.format(dl=dl, wl=wl, th=th, dmask=dmask, h2d=h2d, d2h=d2h, numh2d=numh2d, numd2h=numd2h))
  


def main():
  global execPath
  execPath = sys.argv[1]
  os.access(execPath, os.X_OK) or sys.exit('provide bench tool executable')

  alltransfers = list(map(lambda pair: bench(pair[0], pair[1]), transfers))
  othertransfers = list(map(lambda pair: bench(pair[0], pair[1], 53), transfers))
  pass




if __name__ == "__main__":
  main()
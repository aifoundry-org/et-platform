#!/usr/bin/python3

import subprocess as proc
import sys
import os
import json 
import re
import time
import click
import tempfile

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

transfers = [
  1<<24
]

execPath = ""
daemon = None

def calcCma():
    tmp = proc.getoutput('cat /proc/meminfo | grep -i cma')
    res = {}
    if r:=re.search(r'CmaTotal:[^\d]+(\d+) kB', ""+tmp):
        res['CmaTotal'] = r.group(1)
    if r:=re.search(r'CmaFree:[^\d]+(\d+) kB', ""+tmp):
        res['CmaFree'] = r.group(1)
    return res

def bench(h2d, d2h, wl, th, cma_size, compute_op_stats, add_traces_to_json, enable_traces, dmask=0xFFFFFFFF, numh2d=1, numd2h=1, dl=2):
  savedArgs = locals()
  prevCma = calcCma()
  argos='dummy -json -wl {wl} -h2d {h2d} -d2h {d2h} -th {th} -numh2d {numh2d} -numd2h {numd2h} -dmask {dmask} -deviceLayer {dl}'.format(dl=dl, wl=wl, th=th, dmask=dmask, h2d=h2d, d2h=d2h, numh2d=numh2d, numd2h=numd2h)
  if compute_op_stats:
    argos += ' -computeOpStats true'
  if enable_traces:
    temp = tempfile.NamedTemporaryFile()
    tracePath = temp.name
    temp.close()
    argos += ' -tracePath {tracePath}'.format(tracePath=tracePath)
  env = {}
  if cma_size > 0:
    env = {'ET_CMA_SIZE': str(cma_size)}
  res = json.loads(proc.run(env=env, executable=execPath, stdout=proc.PIPE, args=argos.split(), encoding='utf-8').stdout)
  afterCma = calcCma()
  res['beforeCma'] = prevCma
  res['afterCma'] = afterCma
  res['cmaSize'] = cma_size
  res['deviceLayer'] = dl

  if compute_op_stats:
    for w in res['execution']['WorkersResults']:
      #compute min, average, median and max
      ops = sorted(w['OpStats'], key=lambda o: o['duration_us'])
      avg = 0.0
      for o in ops:
        avg += o['duration_us']
      avg = avg/len(ops)
      w['OpStatsSummary'] = {'minLatencyUs': ops[0]['duration_us'], 'maxLatencyUs': ops[-1]['duration_us'], 'medianLatencyUs': ops[int(len(ops)/2)]['duration_us'], 'avgLatencyUs': avg}

  if enable_traces:
    if add_traces_to_json:
      f = open(tracePath)
      trace = json.load(f)
      res['trace'] = trace
      os.remove(str(tracePath))
    else:
      os.rename(str(tracePath), 'trace' + '.'.join(str(val) for val in savedArgs.values()) + '.json')
  return res

@click.command()
@click.argument('bench_path')
@click.argument('server_path')
@click.argument('output_path')
@click.option('--wl', default=100, help='Number of workloads per thread. Defaults 100.')
@click.option('--th', default=2, help='Number of threads per device. Defaults 2.')
@click.option('--cma_size', default=0, help='Override default CMA size that runtime will use. 0 will let the runtime choose (default). Defaults 0.')
@click.option('--compute_op_stats', default=False, help='Get latency numbers between operations (could slowdown a bit the execution). Defaults False.')
@click.option('--add_traces_to_json', default=False, help='If true and traces enabled, then these traces will be embedded into the final json instead of being separate files. Defaults False.')
@click.option('--enable_traces', default=False, help='Enable trace generation. Note trace generation for daemon will be only on client side. Defaults False.')
def main(bench_path, server_path, output_path, wl, th, cma_size, compute_op_stats, add_traces_to_json, enable_traces):
  global execPath
  execPath = bench_path
  serverPath = server_path
  os.getuid() == 0 or sys.exit("You need to have root privileges to run this script.\nPlease try again, this time using 'sudo'. Exiting.")
  os.access(execPath, os.X_OK) or sys.exit('provide bench tool executable')
  os.access(serverPath, os.X_OK) or sys.exit('provide runtime server executable')
  
  finalJson = []

  #1 card or 4 cards
  for run in range(100):
    for dmask in [1, 2, 4, 8, 15]:
      for bytesMult in [[0, 1], [0.5, 0.5], [1, 0], [2, 2]]:
        eprint('run {} dmask {} bytesMult {}'.format(run, dmask, bytesMult))
        finalJson += list(map(lambda x: bench(dmask=dmask, h2d=int(x*bytesMult[0]), d2h=int(x*bytesMult[1]), wl=wl, th=th, cma_size=cma_size, compute_op_stats=compute_op_stats, add_traces_to_json=add_traces_to_json, enable_traces=enable_traces), transfers))

        #defaults are what we need, so no need to pass parameters
        daemon = proc.Popen([serverPath], preexec_fn=os.setsid, stdout=proc.DEVNULL)

        #sleep for a while just to have the daemon running
        time.sleep(2)

        finalJson += list(map(lambda x: bench(dmask=dmask, h2d=int(x*bytesMult[0]), d2h=int(x*bytesMult[1]), dl=3, wl=wl, th=th, cma_size=cma_size, compute_op_stats=compute_op_stats, add_traces_to_json=add_traces_to_json, enable_traces=enable_traces), transfers))

        daemon.terminate()
        daemon.wait()

    #store results after a full run
    with open(output_path, 'w') as outfile:
      json.dump(finalJson, outfile, indent=2)
      outfile.close()



if __name__ == "__main__":
  main()

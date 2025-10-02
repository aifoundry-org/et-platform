#!/usr/bin/python3

# Import seaborn
import seaborn as sns
import pandas as pd
import sys
import json
import matplotlib.pyplot as plt

# Apply the default theme
sns.set_theme()


def plotMbpsSPvsMP(data):
  #get MBPs
  MBps = [{
    'TotalMBps': item['execution']['TotalMBpsReceived'] + item['execution']['TotalMBpsSent'],
    'TotalBytesWorkload' : item['options']['bytesH2D'] + item['options']['bytesD2H'],
    'SentMBps' :item['execution']['TotalMBpsSent'],
    'ReceivedMBps': item['execution']['TotalMBpsReceived'],
    'bytesH2D': item['options']['bytesH2D'],
    'bytesD2H': item['options']['bytesD2H'],
    'mode': 'standalone' if item['deviceLayer'] == 2 else 'multiprocess'
    } for item in data]

  df = pd.json_normalize(MBps)
 
  sns.relplot(
    data=df, kind="line",
    x="TotalBytesWorkload", y="TotalMBps", col="mode",    
  )
  plt.xscale('log')
  plt.show()

def plotMbpsMPnormalized(data, h2d=None, d2h=None, maxBytes=None):
#get MBPs
  MBps = [{
    'TotalMBps': item['execution']['TotalMBpsReceived'] + item['execution']['TotalMBpsSent'],
    'TotalBytesWorkload' : item['options']['bytesH2D'] + item['options']['bytesD2H'],
    'SentMBps' :item['execution']['TotalMBpsSent'],
    'ReceivedMBps': item['execution']['TotalMBpsReceived'],
    'bytesH2D': item['options']['bytesH2D'],
    'bytesD2H': item['options']['bytesD2H'],
    'mode': 'standalone' if item['deviceLayer'] == 2 else 'multiprocess'
    } for item in data if (h2d==None or item['options']['bytesH2D'] == h2d) and (d2h==None or item['options']['bytesD2H'] == d2h) and (maxBytes==None or (item['options']['bytesH2D'] + item['options']['bytesD2H'] <= maxBytes))]

  df = pd.json_normalize(MBps)
 
  g = sns.FacetGrid(df, col="TotalBytesWorkload", height=4, aspect=.5)
  g.map(sns.barplot, "mode", "TotalMBps", order=["standalone", "multiprocess"])
  plt.show()

def popFromList(list, key, value):
  for i in range(len(list)):
    if list[i][key] == value:
      return list.pop(i)

def getCommandSentPairs(trace):
  complete = []
  uncomplete = []
  def getFromExtra(item, key):
    for e in item['extra']:
        if e['key'] == key:
          res = e['value']['data']
          break
    assert(res)
    return res

  def getEvent(item):
    return getFromExtra(item, 'event')

  for item in trace:
    if item['class'] == 'CommandSent':
      uncomplete.append({'id': getEvent(item), 'start': item['timeStamp']['time_since_epoch']['count']})
    elif item['class'] == 'ResponseReceived':
      prevEvent = popFromList(uncomplete, 'id', getEvent(item))
      prevEvent['dev_exec_time'] = getFromExtra(item, 'device_cmd_exec_dur')
      prevEvent['dev_wait_time'] = getFromExtra(item, 'device_cmd_wait_dur')
      prevEvent['end': item['timeStamp']['time_since_epoch']['count']]
      prevEvent['latency': prevEvent['end'] - prevEvent['start']]
      complete.append(prevEvent)
  assert(len(uncomplete)==0)
  return complete


def main():
  len(sys.argv) > 0 or sys.exit('usage: {} <input.json>'.format(sys.argv[0]))
  file = open(sys.argv[1])
  data = json.load(file)
  #plotMbpsSPvsMP(data)
  maxBytes = None
  plotMbpsMPnormalized(data, maxBytes=maxBytes)
  plotMbpsMPnormalized(data, h2d=0, maxBytes=maxBytes)
  plotMbpsMPnormalized(data, d2h=0, maxBytes=maxBytes)




if __name__ == "__main__":
  main()

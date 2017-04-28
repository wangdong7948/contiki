#!/usr/bin/env python

import re
import os
import fileinput
import math
from sets import Set
from pylab import *
import pandas as pd
from pandas import *
from datetime import *
from collections import OrderedDict

pd.set_option('display.width', None)
pd.set_option('display.max_columns', None)

def parseTxData(log):
    res = re.compile('^\TSCH: {asn-([a-f\d]+).([a-f\d]+) link-(\d+)-(\d+)-(\d+)-(\d+) [\s\d-]*ch-(\d+)\} bc-([01])-0 (\d*) tx').match(log)
    if res:
        asn_ms = int(res.group(1), 16)
        asn_ls = int(res.group(2), 16)
        return {'asn': asn_ls, # ignore MSB
              'radio': "cc1200" if res.group(3) == "0" else "cc2538",
              #'slotframe_len': int(res.group(4)),
              #'timeslot': int(res.group(5)),
              #'channel_offset': int(res.group(6)),
              'channel': int(res.group(7)),
              #'is_eb': res.group(8) == "0",
              #'packet_len': int(res.group(9)),
            }
            
def parseRxData(log):
    res = re.compile('^\TSCH: {asn-([a-f\d]+).([a-f\d]+) link-(\d+)-(\d+)-(\d+)-(\d+) [\s\d-]*ch-(\d+)\} bc-([01])-0 (\d*) rx (\d*) rssi ([-\d]*),.* edr ([-\d]*)').match(log)
    if res:
        asn_ms = int(res.group(1), 16)
        asn_ls = int(res.group(2), 16)
        return {'asn': asn_ls, # ignore MSB
              'radio': "cc1200" if res.group(3) == "0" else "cc2538",
              #'slotframe_len': int(res.group(4)),
              #'timeslot': int(res.group(5)),
              #'channel_offset': int(res.group(6)),
              'channel': int(res.group(7)),
              #'is_eb': res.group(8) == "0",
              #'packet_len': int(res.group(9)),
              'source': int(res.group(10)),
              'rssi': int(res.group(11)),
              'edr': int(res.group(12))
            }

def parseLine(line):
    res = re.compile('^(\d+)\\tID:(\d+)\\t(.*)$').match(line)
    if res:
        return int(res.group(1)), int(res.group(2)), res.group(3)
    return None, None, None

def doParse(file):
    baseTime = None
    time = None
    lastPrintedTime = 0
    data = []

    for line in open(file, 'r').readlines():
        # match time, id, module, log; The common format for all log lines
        t, id, log = parseLine(line)

        time = t   
        if not baseTime:
            baseTime = time
        if time - lastPrintedTime >= 60*1000000:
            print "%u,"%((time-baseTime) / (60*1000000)),
            sys.stdout.flush()
            lastPrintedTime = time
        time -= baseTime
        
        res = parseRxData(log)
        if res != None:
            res["time"] = timedelta(microseconds=time);
            res["destination"] = id;
            res["isTx"] = False;4
            if res["source"] != 0:
                data.append(res)
        else:
            res = parseTxData(log)
            if res != None:
                res["time"] = timedelta(microseconds=time);
                res["source"] = id;
                res["isTx"] = True;
                data.append(res)

    df = DataFrame(data)
    df = df.set_index("time")
    return df

def computePrrBack(x):
    try:
        return stats.loc[x.radio].loc[x.channel].loc[x.destination].loc[x.source]
    except:
        return 0

def getPrrBack(df):
    # group by radio, channel, source and destination
    groupAll = df.groupby(['radio','channel','source','destination'])['rssi','isTx']
    # compute txCount, rxCount and mean RSSI
    stats = groupAll.agg({'rssi': ["count", "mean"], 'isTx': 'sum'})
    stats.columns = ["txCount", "rxCount", "rssi"]
    # only select links A->B where A<B
    statsFlat = stats.reset_index()
    statsFlat = statsFlat[(statsFlat.source < statsFlat.destination)]
    # compute PRR of the reverse link
#    stats["prr-back"] = 
    # get B->A link from A-?B
    
    statsBackFlat = statsFlat.apply(computePrrBack, axis=1)
    statsFlat = statsBackFlat.join(statsFlat, rsuffix="Back")
    return statsFlat

def getChannelStats(df):
    # group by radio, channel and source
    groupAll = df.groupby(['radio','channel','source'])['rssi','isTx']
    # compute txCount, rxCount and mean RSSI
    stats = groupAll.agg({'rssi': ["count", "mean"], 'isTx': 'sum'})
    stats.columns = ["txCount", "rxCount", "rssi"]
    # compute PRR from rxCount and TxConut
    stats["prr"] = stats["rxCount"] / stats["txCount"]
    return stats.reset_index()

def getTimeSeries(df, radio):
    # select logs for the target radtio
    dfRadio = df[(df.radio == radio)]
    # create a timeseries, grouped by rssi and isTx
    groupAll = dfRadio.groupby([pd.TimeGrouper("5Min"), 'channel'])[['rssi','isTx']]
    # compute txCount, rxCount and mean RSSI
    tsStats = groupAll.agg({'rssi': ["count", "mean"], 'isTx': 'sum'})
    tsStats.columns = ["txCount", "rxCount", "rssi"]
    # compute PRR
    tsStats["prr"] = tsStats["rxCount"] / tsStats["txCount"]
    # keep only prr and unstack
    tsPrr = tsStats[['prr']].unstack()
    tsPrr.columns = tsPrr.columns.droplevel()
    # keep only rssi and unstack
    tsRssi = tsStats[['rssi']].unstack()
    tsRssi.columns = tsRssi.columns.droplevel()
    return tsPrr, tsRssi
    
def main():
    if len(sys.argv) < 2:
        return
    else:
        dir = sys.argv[1].rstrip('/')

    file = os.path.join(dir, "logs", "log.txt")
    print "\nProcessing %s" %(file)
    df = doParse("jobs/3087_node/logs/log.txt")
    
    stats = getChannelStats(df)
    stats.groupby("radio").boxplot(column='prr', by='channel')
    stats.groupby("radio").boxplot(column='prr', by='source')
    stats.groupby("radio").boxplot(column='rssi', by='channel')
    stats.groupby("radio").boxplot(column='rssi', by='source')
    
    for radio in ["cc1200", "cc2538"]:
        tsPrr, tsRssi = getTimeSeries(df, radio)
        tsPrr.reset_index().plot()
        tsRssi.reset_index().plot()
        
    prrBack = getPrrBack(df)
    prrBack.groupby("radio").plot.scatter(x='rxCount',y='rxCountBack')
    
main()

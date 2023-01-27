#!/usr/bin/env python

import plotly.express as px
import plotly.graph_objects as go
import numpy as np
import random as rd
import argparse
import sys

import vfd_parse

def strip_n_funcs (n, s):
    for i in range(n):
        pos = s.find("<")
        if pos == -1:
            return s
        else:
            s = s[(pos+1):]
    return s

def get_level_func (level, stack):
   n = stack.count("<")
   if level <= n:
       s = stack.split("<")
       return s[n-level]
   else:
       return ""

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="Visualize vfd files.")
  parser.add_argument("-f", "--filename", dest="filename", type=str, default=None,
                      help="The vfd file to visualize")
  parser.add_argument("-r", "--renderer", dest="renderer", type=str, default="chrome",
                      help="The stack renderer program.")
  parser.add_argument("-n", "--nreadout", dest="nreadout", type=int, default=-1,
                      help="The number of samples to read from the vfd file.")
  args = parser.parse_args()
  if args.filename is None:
      print ("Error: A filename is requred!")
      sys.exit(-1)

  if args.renderer == "firefox":
      print ("Warning: You chose firefox as renderer.")
      print ("         It is known to not be able to display large number of stack names")

  vfdParser = vfd_parse.vfdParser(args.filename)
  vfdParser.show_header()
  
  timestamps = vfdParser.get_timestamps (merge=False, n_readout=args.nreadout)
  stacks = vfdParser.get_stackstrings (args.filename)
  
  stackcolors = {}
  for s in stacks:
      stackcolors[s] = [rd.randint(0,255), rd.randint(0,255), rd.randint(0,255)]
  
  nstacks = len(stacks)
  n_funcs_in_stacks = [s.count("<") + 1 for s in stacks]
  
  rows=[]
  y=[]
  for i in range(max(n_funcs_in_stacks), 0, -1):
      row = []
      yy = []
      for sid in timestamps["stackid"]:
         s = stacks[sid]
         n = n_funcs_in_stacks[sid]
         if i <= n:
             ss = strip_n_funcs (n - i, s)
             row.append(stackcolors[ss])
             yy.append("foo")
         else:
             row.append([255,255,255])
             yy.append("bar")
      rows.append(row)
      y.append(yy)
  
  np_rows = np.array(rows, dtype=np.uint8)
  
  ny = np_rows.shape[0]
  nx = np_rows.shape[1]
  
  funcnames = []
  for level in range(ny-1, -1, -1): #Level
      tmp = []
      for i, stackid in enumerate(timestamps["stackid"]):
          ts = timestamps["start"][i] / 1e9
          level_func = get_level_func(level, stacks[stackid])
          tmp.append(str(ts) + ": " + level_func)
      funcnames.append(tmp)
           
  fig = px.imshow (np_rows)
  fig.update(data=[{'customdata': funcnames,
      'hovertemplate': '%{customdata}'}])
  fig.add_trace(go.Scatter(x=[i for i in range(args.nreadout)], y=[rd.randint(0,10) for i in range(args.nreadout)]))
  fig.update_layout(xaxis=dict(rangeslider=dict(visible=True), type="linear"))
  fig.update_xaxes (side="top")
  fig.show(renderer=args.renderer)

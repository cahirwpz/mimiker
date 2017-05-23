#!/usr/bin/env python3

import os.path
import sys
import subprocess

if __name__ == "__main__":
  print('digraph mimiker_modules {')
  for folder in ['sys','mips']:
    command='grep {}/* -e SYSINIT_ADD'.format(folder)
    out=subprocess.check_output(command,shell=True)
    out=out.decode('UTF-8')
    for line in out.split('\n'):
      if not 'SYSINIT_ADD' in line:
        continue
      line=line.partition('(')[2]
      line=line.rpartition(')')[0]
      name,fname,deps=line.split(', ',2)
      # print('{} [label={}];'.format(name,fname))
      print('{};'.format(name,fname))
      if deps =='NODEPS':
        continue
      deps=deps.partition('(')[2]
      deps=deps.rpartition(')')[0]
      for dependency in deps.split(', '):
        d=dependency.strip('"').rstrip('"')
        print('{} -> {};'.format(name,d))
  print("}")


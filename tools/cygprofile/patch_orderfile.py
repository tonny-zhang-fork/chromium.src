#!/usr/bin/python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import commands
import os
import sys

orderfile = sys.argv[1]
uninstrumented_shlib = sys.argv[2]

nmlines_uninstrumented = commands.getoutput ('nm -S -n ' +
   uninstrumented_shlib + '  | egrep "( t )|( W )|( T )"').split('\n')

nmlines = []
for nmline in nmlines_uninstrumented:
  if (len(nmline.split()) == 4):
    nmlines.append(nmline)

# Map addresses to list of functions at that address.  There are multiple
# functions at an address because of aliasing.
nm_index = 0
uniqueAddrs = []
addressMap = {}
while nm_index < len(nmlines):
  if (len(nmlines[nm_index].split()) == 4):
    nm_int = int (nmlines[nm_index].split()[0], 16)
    size = int (nmlines[nm_index].split()[1], 16)
    fnames = [nmlines[nm_index].split()[3]]
    nm_index = nm_index + 1
    while nm_index < len(nmlines) and nm_int == int (
        nmlines[nm_index].split()[0], 16):
      fnames.append(nmlines[nm_index].split()[3])
      nm_index = nm_index + 1
    addressMap[nm_int] = fnames
    uniqueAddrs.append((nm_int, size))
  else:
    nm_index = nm_index + 1

def binary_search (search_addr, start, end):
  if start >= end or start == end - 1:
    (nm_addr, sym_size) = uniqueAddrs[start]
    if not (search_addr >= nm_addr and search_addr < nm_addr + sym_size):
      error_message = ('ERROR: did not find function in binary: addr: ' +
                       hex(addr) + ' nm_addr: ' + str(nm_addr) + ' start: ' +
                       str(start) + ' end: ' + str(end))
      sys.stderr.write(error_message + "\n")
      raise Exception(error_message)
    return (addressMap[nm_addr], sym_size)
  else:
    halfway = start + ((end - start) / 2)
    nm_addr = uniqueAddrs[halfway][0]
    if (addr >= nm_addr and addr < nm_addr + sym_size):
      return (addressMap[nm_addr], sym_size)
    elif (addr < nm_addr):
      return binary_search (addr, start, halfway)
    elif (addr >= nm_addr + sym_size):
      return binary_search (addr, halfway, end)
    else:
      raise Exception("ERROR: did not expect this case")

f = open (orderfile)
lines = f.readlines()
profiled_list = []
prefixes = ['.text.', '.text.startup.', '.text.hot.', '.text.unlikely.']
for line in lines:
  for prefix in prefixes:
    line = line.replace(prefix, '')
  functionName = line.split('.clone.')[0].strip()
  if (functionName == ''):
    continue
  profiled_list.append(functionName)

# Symbol names are not unique.  Since the order file uses symbol names, the
# patched order file pulls in all symbols with the same name.  Multiple function
# addresses for the same function name may also be due to ".clone" symbols,
# since the substring is stripped.
functions = []
functionAddressMap = {}
for line in nmlines:
  try:
    functionName = line.split()[3]
  except Exception:
    functionName = line.split()[2]
  functionName = functionName.split('.clone.')[0]
  functionAddress = int (line.split()[0].strip(), 16)
  try:
    functionAddressMap[functionName].append(functionAddress)
  except Exception:
    functionAddressMap[functionName] = [functionAddress]
    functions.append(functionName)

sys.stderr.write ("profiled list size: " + str(len(profiled_list)) + "\n")
addresses = []
symbols_found = 0
for function in profiled_list:
   try:
     addrs = functionAddressMap[function]
     symbols_found = symbols_found + 1
   except Exception:
     addrs = []
     # sys.stderr.write ("WARNING: could not find symbol " + function + "\n")
   for addr in addrs:
     if not (addr in addresses):
       addresses.append(addr)
sys.stderr.write ("symbols found: " + str(symbols_found) + "\n")

sys.stderr.write ("number of addresses: " + str(len(addresses)) + "\n")
total_size = 0
for addr in addresses:
  (functions, size) = binary_search (addr, 0, len(uniqueAddrs))
  total_size = total_size + size
  for function in functions:
    for prefix in prefixes:
      print prefix + function

# The following is needed otherwise Gold only applies a partial sort.
print '.text'    # gets methods not in a section, such as assembly
print '.text.*'  # gets everything else
sys.stderr.write ("total_size: " + str(total_size) + "\n")

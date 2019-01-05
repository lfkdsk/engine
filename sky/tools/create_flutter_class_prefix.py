#!/usr/bin/env python
# Copyright 2013 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import subprocess
import sys
import os
import time

def MakeStamp(stamp_path, content):
  dir_name = os.path.dirname(stamp_path)

  if not os.path.isdir(dir_name):
    os.makedirs()

  file = open(stamp_path, 'w')
  file.write(content)
  file.close()

def main():
    parser = argparse.ArgumentParser(
      description='Create Flutter Class Prefix for iOS Framework')

    parser.add_argument('--tpl', action='store', type=str, required=True)
    parser.add_argument('--dist', action='store', type=str, required=True)
    parser.add_argument('--placeholder', action='store', type=str)
    parser.add_argument('--prefix', action='store', type=str)
    parser.add_argument('--stamp', action='store', type=str, required=True)
    args = parser.parse_args()

    tpl_file = open(args.tpl)
    tpl_content = tpl_file.read()
    tpl_file.close()

    tpl_content = tpl_content.replace(args.placeholder, args.prefix)

    dist_file = open(args.dist, 'w')
    dist_file.write(tpl_content)
    dist_file.close()

    MakeStamp(args.stamp, str(args))

if __name__ == '__main__':
  sys.exit(main())

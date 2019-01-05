#!/usr/bin/env python
# Copyright 2013 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import subprocess
import sys
import os

def main():
    parser = argparse.ArgumentParser(
      description='Create Flutter Class Prefix for iOS Framework')

    parser.add_argument('--tpl', action='store', type=str, required=True)
    parser.add_argument('--dist', action='store', type=str, required=True)
    parser.add_argument('--placeholder', action='append', type=str)
    parser.add_argument('--replace', action='append', type=str)
    args = parser.parse_args()

    tpl_file = open(args.tpl, 'r')
    content = tpl_file.read()
    tpl_file.close()

    for i, placeholder in enumerate(args.placeholder):
      content = content.replace(placeholder, args.replace[i])

    dist_file = open(args.dist, 'w')
    dist_file.write(content)
    dist_file.close()

if __name__ == '__main__':
  sys.exit(main())

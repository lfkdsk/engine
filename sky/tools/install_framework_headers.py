#!/usr/bin/env python
# Copyright 2013 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import errno
import os
import shutil
import subprocess
import sys
import re


def filter_header(file_path, dist_path, prefix, classes):
    ''' Replace class names in header file and remove FlutterClassDefine.h including.
    '''

    header_file = open(file_path, 'r')
    content = header_file.read()
    header_file.close()

    for cls in classes:
        content = content.replace(cls, prefix+cls)

    # Remove duplicated prefix.
    content = re.sub(r'(%s)+' % prefix, prefix, content)
    # Fix replaced include.
    content = content.replace('#include "%s' % prefix, '#include "')
    content = content.replace(
        '#include "Flutter', '#include "%sFlutter' % prefix)
    content = content.replace(
        '#import "Flutter', '#import "%sFlutter' % prefix)
    content = content.replace('FLUTTER_', '%sFLUTTER_' % prefix.upper())

    dist_file = open(dist_path, 'w')
    dist_file.write(content)
    dist_file.close()


def main():
    parser = argparse.ArgumentParser(
        description='Removes existing files and installs the specified headers' +
                    'at the given location.')

    parser.add_argument('--headers',
                        nargs='+', help='The headers to install at the location.', required=True)
    parser.add_argument('--classes',
                        nargs='+', help='The prefixed classes to replace.', required=True)
    parser.add_argument('--location', type=str, required=True)
    parser.add_argument('--prefix', type=str, required=True)

    args = parser.parse_args()

    classes = args.classes

    # Remove old headers.
    try:
        shutil.rmtree(os.path.normpath(args.location))
    except OSError as e:
        # Ignore only "not found" errors.
        if e.errno != errno.ENOENT:
            raise e

    # Create the directory to copy the files to.
    if not os.path.isdir(args.location):
        os.makedirs(args.location)

    # Copy all files specified in the args.
    for header_file in args.headers:
        filter_header(header_file, os.path.join(
            args.location, args.prefix + os.path.basename(header_file)), args.prefix, classes)


if __name__ == '__main__':
    sys.exit(main())

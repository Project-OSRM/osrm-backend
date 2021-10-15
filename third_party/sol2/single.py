#!/usr/bin/env python

import argparse
import os, sys
import re
import datetime as dt

# python 3 compatibility
try:
	import cStringIO as sstream
except ImportError:
	from io import StringIO

description = "Converts sol to a single file for convenience."

# command line parser
parser = argparse.ArgumentParser(
    usage='%(prog)s [options...]', description=description)
parser.add_argument(
    '--output',
    '-o',
    nargs='+',
    help=
    'name and location of where to place file (and forward declaration file)',
    metavar='file',
    default=['sol.hpp'])
parser.add_argument('--quiet', help='suppress all output', action='store_true')
args = parser.parse_args()

single_file = ''
forward_single_file = ''
single_file = args.output[0]

if len(args.output) > 1:
	forward_single_file = args.output[1]
else:
	a, b = os.path.splitext(single_file)
	forward_single_file = a + '_forward' + b

single_file_dir = os.path.dirname(single_file)
forward_single_file_dir = os.path.dirname(forward_single_file)

script_path = os.path.normpath(os.path.dirname(os.path.realpath(__file__)))
working_dir = os.getcwd()
os.chdir(script_path)

# If the user didn't provide absolute paths then construct them based on the current working dir.
if not os.path.isabs(single_file):
	single_file = os.path.join(working_dir, single_file)
	single_file_dir = os.path.join(working_dir, single_file_dir)

if not os.path.isabs(forward_single_file):
	forward_single_file = os.path.join(working_dir, forward_single_file)
	forward_single_file_dir = os.path.join(working_dir, forward_single_file_dir)

intro = """// The MIT License (MIT)

// Copyright (c) 2013-2018 Rapptz, ThePhD and contributors

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// This file was generated with a script.
// Generated {time} UTC
// This header was generated with sol {version} (revision {revision})
// https://github.com/ThePhD/sol2

#ifndef {guard}
#define {guard}

"""

includes = set([])
standard_include = re.compile(r'#include <(.*?)>')
local_include = re.compile(r'#(\s*?)include "(.*?)"')
project_include = re.compile(r'#(\s*?)include <sol/(.*?)>')
pragma_once_cpp = re.compile(r'(\s*)#(\s*)pragma(\s+)once')
ifndef_cpp = re.compile(r'#ifndef SOL_.*?_HPP')
define_cpp = re.compile(r'#define SOL_.*?_HPP')
endif_cpp = re.compile(r'#endif // SOL_.*?_HPP')


def get_include(line, base_path):
	local_match = local_include.match(line)
	if local_match:
		# local include found
		full_path = os.path.normpath(
		    os.path.join(base_path, local_match.group(2))).replace(
		        '\\', '/')
		return full_path
	project_match = project_include.match(line)
	if project_match:
		# local include found
		full_path = os.path.normpath(
		    os.path.join(base_path, project_match.group(2))).replace(
		        '\\', '/')
		return full_path
	return None


def is_include_guard(line):
	return ifndef_cpp.match(line) or define_cpp.match(
	    line) or endif_cpp.match(line) or pragma_once_cpp.match(line)


def get_revision():
	return os.popen('git rev-parse --short HEAD').read().strip()


def get_version():
	return os.popen('git describe --tags --abbrev=0').read().strip()


def process_file(filename, out):
	global includes
	filename = os.path.normpath(filename)
	relativefilename = filename.replace(script_path + os.sep, "").replace(
	    "\\", "/")

	if filename in includes:
		return

	includes.add(filename)

	if not args.quiet:
		print('processing {}'.format(filename))

	out.write('// beginning of {}\n\n'.format(relativefilename))
	empty_line_state = True

	with open(filename, 'r', encoding='utf-8') as f:
		for line in f:
			# skip comments
			if line.startswith('//'):
				continue

			# skip include guard non-sense
			if is_include_guard(line):
				continue

			# get relative directory
			base_path = os.path.dirname(filename)

			# check if it's a standard file
			std = standard_include.search(line)
			if std:
				std_file = os.path.join('std', std.group(0))
				if std_file in includes:
					continue
				includes.add(std_file)

			# see if it's an include file
			name = get_include(line, base_path)

			if name:
				process_file(name, out)
				continue

			empty_line = len(line.strip()) == 0

			if empty_line and empty_line_state:
				continue

			empty_line_state = empty_line

			# line is fine
			out.write(line)

	out.write('// end of {}\n\n'.format(relativefilename))


version = get_version()
revision = get_revision()
include_guard = 'SOL_SINGLE_INCLUDE_HPP'
forward_include_guard = 'SOL_SINGLE_INCLUDE_FORWARD_HPP'

processed_files = [os.path.join(script_path, x) for x in ['sol.hpp']]
forward_processed_files = [
    os.path.join(script_path, x) for x in ['sol/forward.hpp']
]
result = ''
forward_result = ''

if not args.quiet:
	print('Current version: {version} (revision {revision})\n'.format(
	    version=version, revision=revision))
	print('Creating single header for sol')

ss = StringIO()
ss.write(
    intro.format(
        time=dt.datetime.utcnow(),
        revision=revision,
        version=version,
        guard=include_guard))
for processed_file in processed_files:
	process_file(processed_file, ss)

ss.write('#endif // {}\n'.format(include_guard))
result = ss.getvalue()
ss.close()

if not args.quiet:
	print('finished creating single header for sol\n')

if not args.quiet:
	print('Creating single forward declaration header for sol')

includes = set([])
forward_ss = StringIO()
forward_ss.write(
    intro.format(
        time=dt.datetime.utcnow(),
        revision=revision,
        version=version,
        guard=forward_include_guard))
for forward_processed_file in forward_processed_files:
	process_file(forward_processed_file, forward_ss)

forward_ss.write('#endif // {}\n'.format(forward_include_guard))
forward_result = forward_ss.getvalue()
forward_ss.close()

if not args.quiet:
	print('finished creating single forward declaration header for sol\n')

# Create the output directories if they don't already exist.
os.makedirs(single_file_dir, exist_ok=True)
os.makedirs(forward_single_file_dir, exist_ok=True)

with open(single_file, 'w', encoding='utf-8') as f:
	if not args.quiet:
		print('writing {}...'.format(single_file))
	f.write(result)

with open(forward_single_file, 'w', encoding='utf-8') as f:
	if not args.quiet:
		print('writing {}...'.format(forward_single_file))
	f.write(forward_result)

#!/usr/bin/env python

import os
import re

description = "Lists all primary sol2 header files"

script_path = os.path.normpath(os.path.dirname(os.path.realpath(__file__)))
working_dir = os.getcwd()
os.chdir(script_path)

includes = set([])
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


def process_file(filename):
	global includes
	filename = os.path.normpath(filename)
	relativefilename = filename.replace(script_path + os.sep, "").replace(
	    "\\", "/")

	rel_filename = os.path.relpath(filename, script_path).replace('\\', '/')

	if rel_filename in includes:
		return

	empty_line_state = True

	with open(filename, 'r', encoding='utf-8') as f:
		includes.add(rel_filename)

		for line in f:
			# skip comments
			if line.startswith('//'):
				continue

			# skip include guard non-sense
			if is_include_guard(line):
				continue

			# get relative directory
			base_path = os.path.dirname(filename)

			# see if it's an include file
			name = get_include(line, base_path)

			if name:
				process_file(name)
				continue

processed_files = [os.path.join(script_path, x) for x in ['sol.hpp']]

for processed_file in processed_files:
	process_file(processed_file)

for include in includes:
	print(include)
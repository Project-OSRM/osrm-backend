#!/usr/bin/env python2

import json
import sys
import re

WHITELIST = set(["mph"])

if len(sys.argv) < 3:
    print "Not enough arguments.\nUsage: " + sys.argv[0] + " taginfo.json profile.lua"
    sys.exit(1)

taginfo_path = sys.argv[1]
profile_path = sys.argv[2]

taginfo = None
with open(taginfo_path) as f:
    taginfo = json.load(f)

valid_strings = [t["key"] for t in taginfo["tags"]]
valid_strings += [t["value"] for t in taginfo["tags"] if "value" in t]

string_regxp = re.compile("\"([\d\w\_:]+)\"")

profile = None
with open(profile_path) as f:
    profile = f.readlines()

n_errors = 0
for n, line in enumerate(profile):
    # allow arbitrary suffix lists
    if line.strip().startswith("suffix_list"):
        continue
    # ignore comments
    if line.strip().startswith("--"):
        continue
    tokens = set(string_regxp.findall(line))
    errors = []
    for token in tokens:
        if token not in WHITELIST and token not in valid_strings:
            idx = line.find("\""+token+"\"")
            errors.append((idx, token))
    errors = sorted(errors)
    n_errors += len(errors)
    if len(errors) > 0:
        prefix = "%i: " % n
        offset = len(prefix)
        for idx, token in errors:
            sys.stdout.write(prefix + line)
            marker = " "*(idx+offset) + "~"*(len(token)+2)
            print(marker)

if n_errors > 0:
    sys.exit(1)

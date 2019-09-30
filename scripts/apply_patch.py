#!/usr/bin/env python3

import argparse
import glob
import os
import subprocess
import sys

parser = argparse.ArgumentParser(description="Apply patch set onto git branch")
parser.add_argument('star_src')
parser.add_argument('star_patch')
args = parser.parse_args()

script_dir = os.path.dirname(os.path.realpath(__file__))

patch_files = " ".join(glob.glob(f'{script_dir}/../patch/{args.star_patch}/*.patch'))

subprocess.run('git checkout -- .'.split(), cwd=args.star_src)

if not patch_files:
    sys.exit('No patch files found')

subprocess.run(f'git apply {patch_files}'.split(), cwd=args.star_src)

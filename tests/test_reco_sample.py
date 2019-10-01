#!/usr/bin/env python3

import argparse
import os
import re


def parse_sample_list(sample_list_file):

    with open(sample_list_file, 'r') as slfile:
        sample_list = slfile.read()

    re_block = re.compile(
        r"^\s*##(.*)"
         "\s*"
         "\njob(\d+)\s*{\s*"
         "\ninputfile\s*=(.+)"
         "\nout_dir\s*=(.+)"
         "\nchain\s*=(.+)"
         "\nnevents\s*=(.+)"
         "\n\s*}\s*$", re.MULTILINE)

    for match in re_block.finditer(sample_list):
        matches = [match.strip() for match in match.groups()]
        title, jobid, inputfile, outdir, chain, nevents = matches
        inputfile = os.path.basename(inputfile)
        print(f'\n{jobid} -- {title}\n{inputfile}\n{outdir}\n{chain}\n{nevents}')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Apply patch set onto git branch")
    parser.add_argument('sample_list_file')
    args = parser.parse_args()

    parse_sample_list(args.sample_list_file)

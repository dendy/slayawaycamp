#!/usr/bin/env python3

import argparse
import os.path
import subprocess
import yaml

this_file = os.path.abspath(__file__)
this_dir = os.path.dirname(this_file)
root_dir = f'{this_dir}/../..'
src_dir = f'{root_dir}/slayawaycamp'

with open(os.path.expanduser('~/.config/flat.yaml')) as f:
	flat_scripts_dir = yaml.load(f, Loader=yaml.FullLoader)['scripts']

subprocess.run([f'{flat_scripts_dir}/generate-qtproject.py',
	'--config', f'{this_dir}/config.yaml',
	'--root-dir', src_dir,
	'--project-dir', f'{root_dir}/qtcreator',
	],
	check=True)

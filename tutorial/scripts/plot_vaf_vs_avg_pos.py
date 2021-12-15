#!/usr/bin/env python

import os
import argparse
import logging
import pandas as pd
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt
import seaborn as sns
sns.set_style('whitegrid')

SCRIPT_PATH = os.path.abspath(__file__)
FORMAT = '[%(asctime)s] %(levelname)s %(message)s'
l = logging.getLogger()
lh = logging.StreamHandler()
lh.setFormatter(logging.Formatter(FORMAT))
l.addHandler(lh)
l.setLevel(logging.INFO)
debug = l.debug; info = l.info; warning = l.warning; error = l.error

DESCRIPTION = '''
'''

EPILOG = '''
'''

class CustomFormatter(argparse.ArgumentDefaultsHelpFormatter,
    argparse.RawDescriptionHelpFormatter):
  pass
parser = argparse.ArgumentParser(description=DESCRIPTION, epilog=EPILOG,
  formatter_class=CustomFormatter)

parser.add_argument('variant_table')
parser.add_argument('--min-var', action='store', type=int,
    help='Minimum number of variant reads', default=1)
parser.add_argument('--min-depth', action='store', type=int,
    help='Minimum number of variant reads', default=1)
parser.add_argument('--min-vaf', action='store', type=float,
    help='Minimum VAF', default=0)
parser.add_argument('-v', '--verbose', action='store_true',
    help='Set logging level to DEBUG')

args = parser.parse_args()

if args.verbose:
  l.setLevel(logging.DEBUG)

debug('%s begin', SCRIPT_PATH)

df = pd.read_table(args.variant_table)

select = (df['depth'] >= args.min_depth) & \
    (df['count'] >= args.min_var) & \
    (df['vaf'] >= args.min_vaf)

df = df[select]

plt.plot(df['avg_pos_as_fraction'], df['vaf'], 'o', color='black')
plt.title('vaf vs avg_pos_as_fraction')
plt.xlabel('avg_pos_as_fraction')
plt.ylabel('vaf')
plt.savefig('vaf_vs_avg_pos_as_fraction.png')


debug('%s end', (SCRIPT_PATH))

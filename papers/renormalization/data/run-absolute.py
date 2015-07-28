#/usr/bin/python2

from __future__ import division
from math import pi       # REALLY don't need all of math
import os, numpy, sys


R=1

i= 0
# RG recursion level

ww=1.3
#arg ww = [1.3, 1.5, 2.0, 3.0]

L=5
#arg L = [5.0]

Ns =list(xrange(0,2))  #list(xrange(0,4**(i+1)+1))
#arg Ns = [range(2,10)]

for N in Ns:
    dirname = 'scrunched/i%01d/N%03d/absolute/'
    os.system('mkdir -p'+dirname)
    filename = 'absolute-ww%4.2f-L%04.2f-N%03d'
    filename+= '-g.dat'
    
    ffs = []
    success_ratios = []
    all_total_checks = []
    all_valid_checks = []
    steps = 20 # Need a better value for this
    ff = (4/3.0)*pi*R**3/L**3
    step_size = 0.05 # This too
    steps = 20
    sim_iters = 1000000
    sc_period = int(max(10, 1*N*N/10))
    
    
    for j in xrange(steps):
        if j!= 0 and j % 4 == 0:
            step_size = step_size * 0.5
        cmd = 'srun -J %s' % filename
        cmd += ' ../../../free-energy-monte-carlo'
        cmd += ' --ff %g' % ff
        cmd += ' --sc_period %d' % sc_period
        cmd += ' --iterations %d' % sim_iters
        cmd += ' --filename %s' % filename
        cmd += ' --data_dir %s' % dirname
        cmd += ' --ff_small %g' % (ff+step_size)
        cmd += ' --N %d' % N

        print("Running with command: %s") % cmd
        os.system(cmd)
        #Not sure about the cacheing step... we'll see this
        #tomorrow and talk about it

        

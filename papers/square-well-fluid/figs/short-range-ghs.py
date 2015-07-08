#!/usr/bin/python2
from __future__ import division
# We need the following two lines in order for matplotlib to work
# without access to an X server.
import matplotlib, sys, os.path
if len(sys.argv) < 2 or sys.argv[1] != "show":
  matplotlib.use('Agg')
import sympy
from scipy.optimize import leastsq
from sympy import pi, exp
import pylab, string, numpy

from matplotlib import rc

import styles

rc('text', usetex=True)

sigma = 2

# create variables to store latex / C++ code
latex_code = r"""% Code generated by plot-ghs.py
\documentclass{article}
\usepackage{breqn}
\begin{document}
"""
c_code = r"""// Code generated by plot-ghs.py
#include <math.h>
"""

# define variables / constants
variables = ['r', 'h_sigma', 'g_sigma']
positive_variables = ['kappa_11', 'kappa_12', 'kappa_13', 'kappa_14',
                      'kappa_21', 'kappa_22', 'kappa_23', 'kappa_24',
                      'kappa_31', 'kappa_32', 'kappa_33', 'kappa_34',
                      'kappa_41', 'kappa_42', 'kappa_43', 'kappa_44',
                      'alpha']
# expressions is a list of tuples, where each tuple is the name of a variable followed by the
# expression it is equal two, in terms of lambda expressions of the dict v.

stupidconstant = 0.25

def scalefunction(hsig): # ~1 at 0.494, and ~0 at 0.545
  return 1.0/(1 + (stupidconstant*v['h_sigma'])**4)

expressions = [
  ('g_HS', lambda:
   1 +
   scalefunction(v['h_sigma'])*
   (v['h_sigma']  +
    v['kappa_11'] * v['h_sigma']**1 * v['zeta'] +
    v['kappa_12'] * v['h_sigma']**1 * v['zeta']**2 +
    v['kappa_13'] * v['h_sigma']**1 * v['zeta']**3 +
    v['kappa_14'] * v['h_sigma']**1 * v['zeta']**4 +
    v['kappa_21'] * v['h_sigma']**2 * v['zeta'] +
    v['kappa_22'] * v['h_sigma']**2 * v['zeta']**2 +
    v['kappa_23'] * v['h_sigma']**2 * v['zeta']**3 +
    v['kappa_24'] * v['h_sigma']**2 * v['zeta']**4 +
    v['kappa_31'] * v['h_sigma']**3 * v['zeta'] +
    v['kappa_32'] * v['h_sigma']**3 * v['zeta']**2 +
    v['kappa_33'] * v['h_sigma']**3 * v['zeta']**3 +
    v['kappa_34'] * v['h_sigma']**3 * v['zeta']**4 +
    v['kappa_41'] * v['h_sigma']**4 * v['zeta'] +
    v['kappa_42'] * v['h_sigma']**4 * v['zeta']**2 +
    v['kappa_43'] * v['h_sigma']**4 * v['zeta']**3 +
    v['kappa_44'] * v['h_sigma']**4 * v['zeta']**4) * exp(-0*v['alpha']*v['zeta'])),
  ('g_sigma', lambda: v['h_sigma'] + 1),
  ('zeta', lambda: (v['r'] - v['sigma'])/v['sigma']),
  ('R', lambda: v['sigma']/2),
  ('sigma', lambda: sympy.S(sigma))
]
l = []
expr = []
for x in expressions:
  l.append(x[0])
  expr.append(x[1])

v1 = dict((elem, sympy.symbols(elem)) for elem in l+variables)
v2 = dict((elem, sympy.symbols(elem, positive=True)) for elem in positive_variables)
v = dict(v1, **v2)

##################3



f = open('figs/short-range-ghs-analytics.tex', 'w')
f.write(latex_code)
f.close()


for i in xrange(len(expr)):
  latex_code += '\\begin{dmath}\n' + sympy.latex(sympy.Eq(v[l[i]], expr[i]())) + '\n\\end{dmath}\n'

# this loop unwraps the onion, getting ghs in terms of only the 9 kappas, h_sigma, and r
# we will wait on substituting h_sigma as a function of eta, though
for i in reversed(xrange(len(expr))):
    temp = v[l[i]]
    v[l[i]] = expr[i]()
    # v[l[i]] = sympy.simplify(expr[i]() this makes it take way too long
    latex_code += '\\begin{dmath}\n' + sympy.latex(sympy.Eq(temp, v[l[i]])) + '\n\\end{dmath}\n'

ghs_s = expr[0]()

# now that we have ghs defined, we want to do a best fit to find the kappas
print 'I see g(r) = ', ghs_s.subs(sympy.symbols('h_sigma'), sympy.symbols('g_sigma') - 1)
lam = sympy.utilities.lambdify(('kappa_11', 'kappa_12', 'kappa_13', 'kappa_14',
                                'kappa_21', 'kappa_22', 'kappa_23', 'kappa_24',
                                'kappa_31', 'kappa_32', 'kappa_33', 'kappa_34',
                                'kappa_41', 'kappa_42', 'kappa_43', 'kappa_44',
                                'alpha', 'g_sigma', 'r'),
                               ghs_s.subs(sympy.symbols('h_sigma'), sympy.symbols('g_sigma') - 1), 'numpy')

def evalg(x, g_sigma, r):
  return lam(x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], x[9], x[10], x[11], x[12],
             x[13], x[14], x[15], x[16],
             g_sigma, r)

fit_rcutoff = styles.short_range
def read_ghs(base, ff):
  global able_to_read_file
  mcdatafilename = "%s-%4.2f.dat" % (base, ff)
  if (os.path.isfile(mcdatafilename) == False):
    print "File does not exist: ", mcdatafilename
    able_to_read_file = False
    return 0, 0

  mcdata = numpy.loadtxt(mcdatafilename)
  print 'Using', mcdatafilename, 'for filling fraction', ff
  r_mc = mcdata[:,0]
  n_mc = mcdata[:,1]
  #n_mc = n_mc[r_mc < fit_rcutoff]
  #r_mc = r_mc[r_mc < fit_rcutoff]
  ghs = n_mc/ff
  return r_mc, ghs

colors = ['r', 'g', 'b', 'c', 'm', 'k', 'y', 'g', 'r', 'b']
ff = numpy.array([.45, .4, .35, .3, .25, .2, .15, .1, .05])

x = numpy.array([0.68, 0.16, 0.79, 1.e-3, 2e-4, 3.e-3, 1.e-3, 2e-4, 3.e-3, 0, 0, 0, 0, 0, 0, 0, 3])


# read data
able_to_read_file = True

ghs = [0]*len(ff)
eta = [0]*len(ff)
gsigmas = [0]*len(ff)

fig1 = pylab.figure(1, figsize=(5,4))
ax1 = fig1.gca()
pylab.axvline(x=sigma, color='k', linestyle=':')
pylab.axhline(y=1, color='k', linestyle=':')

for i in range(len(ff)):
    r_mc, ghs[i] = read_ghs("figs/gr", ff[i])
    if able_to_read_file == False:
        break
    pylab.figure(1)
    pylab.plot(r_mc, ghs[i], colors[i]+"-",label='$\eta = %.2f$'%ff[i])
    eta[i] = ff[i]
    gsigmas[i] = (1 - ff[i]/2)/(1 - ff[i])**3
    r = r_mc

# now do the least squares fit
def dist(x):
  # function with x[i] as constants to be determined
  R, GSIGMA = pylab.meshgrid(r, gsigmas)
  g = pylab.zeros_like(GSIGMA)
  g = evalg(x, GSIGMA, R)
  return pylab.reshape(g, len(gsigmas)*len(r))

def dist2(x):
  R, GSIGMAS = pylab.meshgrid(r[r<fit_rcutoff], gsigmas)
  g = pylab.zeros_like(GSIGMAS)
  g = evalg(x, GSIGMAS, R)
  gfit = pylab.reshape(g, len(eta)*len(r[r<fit_rcutoff]))
  return gfit - pylab.reshape([g[r<fit_rcutoff] for g in ghs],
                              len(gsigmas)*len(r[r<fit_rcutoff]))

ghsconcatenated = ghs[0]
for i in range(1,len(ff)):
  ghsconcatenated = pylab.concatenate((ghsconcatenated, ghs[i]))

etaconcatenated = [0]*len(r)*len(eta)
j = 0
while (j < len(eta)):
  i = 0
  while (i < len(r)):
    etaconcatenated[i + j*len(r)] = eta[j]
    i += 1
  j += 1

rconcatenated = [0]*len(r)*len(eta)
j = 0
while (j < len(eta)):
  i = 0
  while (i < len(r)):
    rconcatenated[i + j*len(r)] = r[i]
    i += 1
  j += 1

vals = pylab.zeros_like(x)

chi2 = sum(dist2(x)**2)
print "beginning least squares fit, chi^2 initial: ", chi2
vals, mesg = leastsq(dist2, x)
# round fitted numbers
digits = 3
vals = pylab.around(vals, digits)
chi2 = sum(dist2(vals)**2)
print "original fit complete, chi^2: %g" % chi2

toprint = True
for i in range(len(x)):
  print "vals[%i]: %.*f\t x[%i]: %g" %(i, digits, vals[i], i, x[i])

g = dist(vals)
gdifference = dist2(vals)

chisq = (gdifference**2).sum()
maxerr = abs(gdifference).max()
etamaxerr = 0
rmaxerr = 0
for i in xrange(len(gdifference)):
  if abs(gdifference[i]) == maxerr:
    etamaxerr = etaconcatenated[i]
    rmaxerr = rconcatenated[i]
K11 = vals[0]
K12 = vals[1]
K13 = vals[2]
K14 = vals[3]

K21 = vals[4]
K22 = vals[5]
K23 = vals[6]
K24 = vals[7]

K31 = vals[8]
K32 = vals[9]
K33 = vals[10]
K34 = vals[11]

K41 = vals[12]
K42 = vals[13]
K43 = vals[14]
K44 = vals[15]

alpha = vals[12]

def next_comma(ccode):
  """ returns next comma not counting commas within parentheses """
  deepness = 0
  for i in xrange(len(ccode)):
    if ccode[i] == ')':
      if deepness == 0:
        return -1
      else:
        deepness -= 1
    elif ccode[i] == '(':
      deepness += 1
    elif ccode[i] == ',' and deepness == 0:
      return i

def next_right_paren(ccode):
  """ returns next ")" not counting matching parentheses """
  deepness = 0
  for i in xrange(len(ccode)):
    if ccode[i] == ')':
      if deepness == 0:
        return i
      else:
        deepness -= 1
    elif ccode[i] == '(':
      deepness += 1

def fix_pows(ccode):
  """ A pointless optimization to remove unneeded calls to "pow()".
  It turns out not to make a difference in the speed of walls.mkdat,
  but I'm leaving it in, becuase this way we can easily check if this
  makes a difference (since I thought that it might). """
  n = string.find(ccode, 'pow(')
  if n > 0:
    return ccode[:n] + fix_pows(ccode[n:])
  if n == -1:
    return ccode
  ccode = ccode[4:] # skip 'pow('
  icomma = next_comma(ccode)
  arg1 = fix_pows(ccode[:icomma])
  ccode = ccode[icomma+1:]
  iparen = next_right_paren(ccode)
  arg2 = fix_pows(ccode[:iparen])
  ccode = fix_pows(ccode[iparen+1:])
  if arg2 == ' 2':
    return '((%s)*(%s))%s' % (arg1, arg1, ccode)
  if arg2 == ' 3':
    return '((%s)*(%s)*(%s))%s' % (arg1, arg1, arg1, ccode)
  if arg2 == ' 4':
    return '((%s)*(%s)*(%s)*(%s))%s' % (arg1, arg1, arg1, arg1, ccode)
  if arg2 == ' 5':
    return '((%s)*(%s)*(%s)*(%s)*(%s))%s' % (arg1, arg1, arg1, arg1, arg1, ccode)
  return 'pow(%s, %s)%s' % (arg1, arg2, ccode)

# finish printing to latex and c++ with the constants
c_code += r"""
const double kappa_11 = %.*f;
const double kappa_12 = %.*f;
const double kappa_13 = %.*f;
const double kappa_14 = %.*f;

const double kappa_21 = %.*f;
const double kappa_22 = %.*f;
const double kappa_23 = %.*f;
const double kappa_24 = %.*f;

const double kappa_31 = %.*f;
const double kappa_32 = %.*f;
const double kappa_33 = %.*f;
const double kappa_34 = %.*f;

const double kappa_41 = %.*f;
const double kappa_42 = %.*f;
const double kappa_43 = %.*f;
const double kappa_44 = %.*f;

const double alpha = %.*f;

// inline double short_range_gsigma_to_eta(const double g_sigma) {
//   if (g_sigma <= 1) return 0;
//   return %s;
// }


inline double short_range_radial_distribution(double g_sigma, double r) {
  if (g_sigma <= 1) return 1; // handle roundoff error okay
  if (r < %i) return 0;
  const double h_sigma = g_sigma - 1;
  return %s;
}
""" %(digits, K11, digits, K12, digits, K13, digits, K14,
      digits, K21, digits, K22, digits, K23, digits, K24,
      digits, K31, digits, K32, digits, K33, digits, K34,
      digits, K41, digits, K42, digits, K43, digits, K44,
      digits, alpha,
      fix_pows(sympy.ccode(v['h_sigma'])), v['sigma'], fix_pows(sympy.ccode(ghs_s)))

latex_code += r"""
\begin{dmath}
kappa_21 = %.*f
\end{dmath}
\begin{dmath}
kappa_22 = %.*f
\end{dmath}
\begin{dmath}
kappa_23 = %.*f
\end{dmath}
\begin{dmath}
kappa_31 = %.*f
\end{dmath}
\begin{dmath}
kappa_32 = %.*f
\end{dmath}
\begin{dmath}
kappa_33 = %.*f
\end{dmath}
\end{document}
""" %(digits, K21, digits, K22, digits, K23,
      digits, K31, digits, K32, digits, K33)

f = open('figs/short-range-ghs-analytics.tex', 'w')
f.write(latex_code)
f.close()

f = open('figs/short-range-ghs-analytics.h', 'w')
f.write(c_code)
f.close()

# save fit parameters
outfile = open('figs/short-range-fit-parameters.tex', 'w')
outfile.write(r"""
\newcommand\maxrfit{%(fit_rcutoff).2g}
\newcommand\maxerr{%(maxerr).2g}
\newcommand\etamaxerr{%(etamaxerr)g}
\newcommand\rmaxerr{%(rmaxerr).2g}
\newcommand\chisq{%(chisq).2g}
\newcommand\kappatable{
  \left(
  \begin{array}{c d{3} d{3} d{3} d{3}}
    %(K11)g & %(K12)g & %(K13)g & %(K14)g \\
    %(K21)g & %(K22)g & %(K23)g & %(K24)g \\
    %(K31)g & %(K32)g & %(K33)g & %(K34)g \\
    %(K41)g & %(K42)g & %(K43)g & %(K44)g \\
  \end{array}
  \right)
}
\newcommand\alphaval{%(alpha)g}
""" % locals())
outfile.close()


### gil-villegas ######################################################

gil_matrix = numpy.matrix([
    [2.25855, -1.50349, 0.249434],
    [-0.669270, 1.40049, -0.827739],
    [10.1576, -15.0427, 5.30827]])

def eta_effective(eta, r):
    cs = numpy.dot(gil_matrix, numpy.array([[1], [r], [r**2]]))
    eta_eff = numpy.dot(numpy.array([eta, eta**2, eta**3]), cs)
    return eta_eff[0]

def eta_effective_prime(eta, r):
    cs = numpy.dot(gil_matrix, numpy.array([[0], [1], [2*r]]))
    eta_eff_prime = numpy.dot(numpy.array([eta, eta**2, eta**3]), cs)
    return eta_eff_prime[0]

def g_gil_villegas(eta, r):
    r /= sigma
    eta_eff = eta_effective(eta, r)
    eta_eff_prime = eta_effective_prime(eta, r)
    g_sigma = (1 - eta_eff/2)/(1 - eta_eff)**3
    g_sigma_prime = eta_eff_prime*(-2*eta_eff + 5)/(2*(1 - eta_eff)**4)

    rho = eta / (pi/6*sigma**3)
    return 2*eta/pi/rho/sigma**3*(3*g_sigma + (r**3 - 1)/r**2*g_sigma_prime)

#######################################################################

# takes two arrays, and averages points so that a plot of x vs y
# will have points separated by a distance dpath
# returns (x, y)
def avg_points(x, y, dpath):
  new_y = numpy.array([])
  new_x = numpy.array([])
  old_i = 0
  for i in xrange(1, len(x)):
    dist = numpy.sqrt((x[i] - x[old_i])**2 + (y[i] - y[old_i])**2)
    if dist >= dpath or i == len(x) - 1:
      avg_x = numpy.average(x[old_i:i])
      avg_y = numpy.average(y[old_i:i])

      new_x = numpy.append(new_x, avg_x)
      new_y = numpy.append(new_y, avg_y)
      old_i = i
  return (new_x, new_y)

scaleme = 0.8
fig2 = pylab.figure(2, figsize=(6*scaleme,8*scaleme))
ax2 = fig2.gca()

# ax.axvline(x=sigma, color='gray', linestyle='-')
# ax.axhline(y=1, color='gray', linestyle='-')

alt_styles = {
    'gS' : '-b',
    'gSunfitted' : ':b',
    'mc' : '.k',
    'gil' : '--m',
    'gil_unfitted' : ':m'
}

# now let's plot the fit
ranges = [(0.9,1.2), (0.9,1.4), (0.9,1.7), (0.8, 1.8), (0.8, 2.3),
          (0.75, 2.5), (0.75, 3.2), (0.6, 4.0), (0.6, 5.0)]

skip_every_other = True
indexes = range(len(ff))
if skip_every_other:
  indexes = range(1,len(ff),2)
  for i in range(0,len(ranges),2):
    ranges[i] = (0,0)

zeros = [-ranges[0][0]]*len(ranges)
maxes = [ranges[0][1]]*len(ranges)
mines = [ranges[0][0]]*len(ranges)
for i in range(1,len(zeros)):
  mines[i] = ranges[i][0]
  maxes[i] = ranges[i][1]
  zeros[i] = zeros[i-1] + maxes[i-1] - ranges[i][0]
zeros.reverse()
maxes.reverse()
mines.reverse()
assert(len(ranges) == len(ff))

ticks = []
ticklabels = []
for i in indexes:
  pylab.figure(1)
  pylab.plot(r_mc, g[i*len(r):(i+1)*len(r)], colors[i]+'--')

  # gil-villegas
  g_gil = numpy.zeros_like(r_mc)
  for j in range(len(g_gil)):
    g_gil[j] = g_gil_villegas(ff[i], r_mc[j])

  gil_rcutoff = 3.6
  gil_rmax = 4.0
  pylab.plot(r_mc[r_mc<gil_rcutoff], g_gil[r_mc<gil_rcutoff], colors[i]+':')
  ##############

  hsigma = (1 - 0.5*ff[i])/(1-ff[i])**3 - 1
  density = 4/3*pi*ff[i]
  rhs = (1-ff[i])**4/(1+4*ff[i]+4*ff[i]**2-4*ff[i]**3+ff[i]**4)/3
  #integral = hsigma*(1/a + x[0]*x[1]/())


  # making a second plot to show one eta at a time
  dpath = 0.2
  r_mc_avged, g_mc_avged = avg_points(r_mc, ghs[i], dpath)
  pylab.figure(2)
  offset = zeros[i]
  pylab.axhline(zeros[i] + maxes[i], color='k')
  pylab.axhline(zeros[i] + 1, color='k', ls=':')
  pylab.plot(r_mc[r_mc<fit_rcutoff], g[i*len(r):(i+1)*len(r)][r_mc<fit_rcutoff] + offset,
             alt_styles['gS'], label='this work')
  pylab.plot(r_mc[r_mc<1.1*fit_rcutoff], g[i*len(r):(i+1)*len(r)][r_mc<1.1*fit_rcutoff] + offset,
             alt_styles['gSunfitted'])
  r_gil = r_mc[r_mc < gil_rmax]
  g_gil = g_gil[r_mc < gil_rmax]
  pylab.plot(r_gil[r_gil<gil_rcutoff], g_gil[r_gil<gil_rcutoff] + offset,
             alt_styles['gil'], label="fit by Gil-Villegas")
  pylab.plot(r_gil[r_gil>gil_rcutoff], g_gil[r_gil>gil_rcutoff] + offset,
             alt_styles['gil_unfitted'])
  pylab.plot(r_mc_avged, g_mc_avged + offset,
             alt_styles['mc'], label='Monte Carlo')

  dg = 0.2
  if maxes[i] > 2:
    dg = 0.5
  for gg in pylab.arange(0, maxes[i] - 0.001, dg):
    if gg > mines[i] + 0.001:
      ticks.append(zeros[i] + gg)
      ticklabels.append('%.1f' % gg)

  #mc:
  r_mc, ghs[i]
  integrand_mc = 4*pi*r_mc*r_mc*ghs[i]
  integrand_ours = 4*pi*r_mc*r_mc*g[i*len(r):(i+1)*len(r)]
  integral_mc = sum(integrand_mc)/len(integrand_mc)*(r_mc[2]-r_mc[1]) - 4/3*pi*sigma**3
  integral_ours = sum(integrand_ours)/len(integrand_ours)*(r_mc[2]-r_mc[1]) - 4/3*pi*sigma**3
  print("Int_mc: %6.3f, Int_ours: %6.3f, Diff: %6.3f" %(integral_mc, integral_ours, integral_ours-integral_mc))


pylab.figure(2)
pylab.ylim(ymin=zeros[-1] + mines[-1], ymax=zeros[0] + maxes[0])
xlim = [1.8, 5.5]
pylab.xlim(xlim[0], xlim[1])
pylab.xlabel('$r/R$')
pylab.ylabel('g(r)')
pylab.axvline(2.0, color='k', ls=':')
pylab.axes().set_yticks(ticks)
pylab.axes().set_yticklabels(ticklabels)

for i in indexes:
  frac = 1.0/3 + 0.03*i - 0.1
  pylab.text((1-frac)*xlim[0]+frac*xlim[1], zeros[i]+maxes[i]-0.14,
             r"$\eta = %.1f$" %(ff[i]), ha='center', va='top')


handles, labels = ax2.get_legend_handles_labels()
ax2.legend(handles[0:5:2], labels[0:5:2], loc='upper right')
# # ax4.legend(loc='upper center')
# # ax4.add_artist(legends[3])
# ax4.set_xlabel('$r/R$')
# for ax in axs:
#     ax.set_xlim(xmin=1.8, xmax=5)
#     #ax.set_ylim(ymin=0)

#ax4.set_yticks([.8, 1, 1.2])
#ax3.set_yticks([1, 1.2, 1.4, 1.6, 1.8])
#ax2.set_yticks([.5, 1, 1.5, 2, 2.5])
#ax1.set_yticks([1, 2, 3])
fig2.tight_layout()
fig2.subplots_adjust(hspace=0.01)
fig2.savefig('figs/short-range-ghs-alt.pdf')

pylab.show()

assert not os.system('cd figs && pdflatex short-range-ghs-analytics.tex')

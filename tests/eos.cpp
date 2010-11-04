// Deft is a density functional package developed by the research
// group of Professor David Roundy
//
// Copyright 2010 The Deft Authors
//
// Deft is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// You should have received a copy of the GNU General Public License
// along with deft; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// Please see the file AUTHORS for a list of authors.

#include <stdio.h>
#include <time.h>
#include "Functionals.h"
#include "utilities.h"

int retval = 0;

void test_eos(const char *name, Functional f, double ntrue, double fraccuracy=1e-6) {
  clock_t start = clock();
  printf("\n************");
  for (unsigned i=0;i<strlen(name);i++) printf("*");
  printf("\n* Testing %s *\n", name);
  for (unsigned i=0;i<strlen(name);i++) printf("*");
  printf("************\n\n");

  double nfound = find_density(f, ntrue*3.123e-7, ntrue*12345);
  printf("Found density of %g (versus %g) in %g seconds.\n", nfound, ntrue,
         (clock() - double(start))/CLOCKS_PER_SEC);
  double nerr = nfound/ntrue - 1;
  if (fabs(nerr) > fraccuracy) {
    printf("FAIL: nerr too big: %g\n", nerr);
    retval++;
  }
}

int main(int, char **argv) {
  double kT = 1e-3;
  Functional n = EffectivePotentialToDensity(kT);
  double ngas = 2e-5;
  double mu = -kT*log(ngas);

  test_eos("ideal gas", IdealGasOfVeff(kT) + ChemicalPotential(mu)(n), -kT*log(ngas));

  if (retval == 0) {
    printf("\n%s passes!\n", argv[0]);
  } else {
    printf("\n%s fails %d tests!\n", argv[0], retval);
    return retval;
  }
}

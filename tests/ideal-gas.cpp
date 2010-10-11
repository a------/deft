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
#include "Functionals.h"

int main(int, char **argv) {
  Lattice lat(Cartesian(0,.5,.5), Cartesian(.5,0,.5), Cartesian(.5,.5,0));
  int resolution = 20;
  GridDescription gd(lat, resolution, resolution, resolution);
  Grid density(gd);
  const double kT = 1e-3; // room temperature in Hartree
  density = 10*(-500*r2(gd)).cwise().exp()
    + 1e-7*VectorXd::Ones(gd.NxNyNz);

  int retval = 0;

  Functional ig = IdealGas(kT);
  retval += ig.run_finite_difference_test("ideal gas", density);

  printf("\nNow let's try this with an effective potential...\n");
  Grid potential(gd, 1e-2*((-50*r2(gd)).cwise().exp()) + -2e-3*VectorXd::Ones(gd.NxNyNz));
  //potential.epsNativeSlice("potential.eps", Cartesian(1,0,0),
  //                         Cartesian(0,1,0), Cartesian(0,0,0));
  //density = EffectivePotentialToDensity(kT)(potential);
  //density.epsNativeSlice("dens.eps", Cartesian(1,0,0),
  //                         Cartesian(0,1,0), Cartesian(0,0,0));
  Functional ig2 = IdealGas(kT)(EffectivePotentialToDensity(kT));
  density *= 1e-5;
  retval += ig2.run_finite_difference_test("ideal gas", potential);

  retval += IdealGasOfVeff(kT).run_finite_difference_test("ideal gas of Veff", potential);

  if (retval == 0) {
    printf("\n%s passes!\n", argv[0]);
  } else {
    printf("\n%s fails %d tests!\n", argv[0], retval);
    return retval;
  }
}

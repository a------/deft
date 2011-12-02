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
#include "OptimizedFunctionals.h"
#include "equation-of-state.h"
#include "LineMinimizer.h"
#include "utilities.h"
#include "handymath.h"

const double nm = 18.8972613; // in bohr
// Here we set up the lattice.
const double width = 0.0001;
double diameter = 1*nm;
double distance = 1;
bool using_default_diameter = true;


double notinwall(Cartesian r) {
  const double z = r.z();
  const double y = r.y();
  if (sqrt(sqr(z+(diameter+distance)/2)+sqr(y+(diameter+distance)/2)) < diameter/2) {
      return 0; 
  } 
  if (sqrt(sqr(z+(diameter+distance)/2)+sqr(y-(diameter+distance)/2)) < diameter/2) {
      return 0;
  }
  if (sqrt(sqr(z-(diameter+distance)/2)+sqr(y+(diameter+distance)/2)) < diameter/2) {
      return 0;
  }
  if (sqrt(sqr(z-(diameter+distance)/2)+sqr(y-(diameter+distance)/2)) < diameter/2) {
      return 0;
  } 
  return 1;
}

void plot_grids_y_direction(const char *fname, const Grid &a, const Grid &b, const Grid &c, const Grid &d) {
  FILE *out = fopen(fname, "w");
  if (!out) {
    fprintf(stderr, "Unable to create file %s!\n", fname);
    // don't just abort?
    return;
  }
  const GridDescription gd = a.description();
  const int x = gd.Nx/2;
  const int z = 0;
  for (int y=0; y<gd.Ny; y++) {
    Cartesian here = gd.fineLat.toCartesian(Relative(x,y,z));
    double ahere = a(x,y,z);
    double bhere = b(x,y,z);
    double chere = c(x,y,z);
    double dhere = d(x,y,z);
    fprintf(out, "%g\t%g\t%g\t%g\t%g\t%g\t%g\n", here[0], here[1], here[2], ahere, bhere, chere, dhere);
  }
  fclose(out);
}

void plot_grids_yz_directions(const char *fname, const Grid &a, const Grid &b, 
                              const Grid &c, const Grid &d) {
  FILE *out = fopen(fname, "w");
  if (!out) {
    fprintf(stderr, "Unable to create file %s!\n", fname);
    // don't just abort?
    return;
  }

  const GridDescription gd = a.description();
  const int x = gd.Nx/2;
  const int y_half = gd.Ny/2;
  const int z_half = gd.Nz/2;

  int dy = 10;
  while (gd.Ny % dy != 0) dy--;
  int dz = 10;
  while (gd.Nz % dz != 0) dz--;
  for (int y=0; y<gd.Ny; y+=dy) {
    for (int z=0; z<gd.Nz; z+=dz) {
      Cartesian here;
      //Rearrange coordinates for easier plotting
      if (y < y_half && z < z_half) {
         here = gd.fineLat.toCartesian(Relative(x, y + y_half, z + z_half));
      } else if (y >= y_half && z >= z_half) {
         here = gd.fineLat.toCartesian(Relative(x, y - y_half, z - z_half));
      } else if (y >= y_half && z < z_half) {
         here = gd.fineLat.toCartesian(Relative(x, y - y_half, z + z_half));
      } else {
         here = gd.fineLat.toCartesian(Relative(x, y + y_half, z - z_half));
      }
      double ahere = a(x,y,z);
      double bhere = b(x,y,z);
      double chere = c(x,y,z);
      double dhere = d(x,y,z);
      fprintf(out, "%g\t%g\t%g\t%g\t%g\t%g\t%g\n", here[0], here[1], here[2], 
              ahere, bhere, chere, dhere);
    }
    fprintf(out,"\n");
 }  
  fclose(out);
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    if (sscanf(argv[1], "%lg", &diameter) != 1) {
      printf("Got bad argument: %s\n", argv[1]);
      return 1;
    }
    diameter *= nm;
    using_default_diameter = false;
    printf("Diameter is %g bohr\n", diameter);
  }
  
  const double dmax = 2*nm;
  double zmax = 2*diameter+dmax+2*nm;
  double ymax = 2*diameter+dmax+2*nm;

  char *datname = new char[1024];
  snprintf(datname, 1024, "paper/figs/four-rods-in-water-%04.1fnm.dat", diameter/nm);
  
  FILE *o = fopen(datname, "w");
  delete[] datname;

  Functional f = OfEffectivePotential(SaftFluid(water_prop.lengthscale,
                                                water_prop.epsilonAB, water_prop.kappaAB,
                                                water_prop.epsilon_dispersion,
                                                water_prop.lambda_dispersion,
                                                water_prop.length_scaling, 0));
  double n_1atm = pressure_to_density(f, water_prop.kT, atmospheric_pressure,
                                      0.001, 0.01);

  double mu_satp = find_chemical_potential(f, water_prop.kT, n_1atm);

  f = OfEffectivePotential(SaftFluid(water_prop.lengthscale,
                                     water_prop.epsilonAB, water_prop.kappaAB,
                                     water_prop.epsilon_dispersion,
                                     water_prop.lambda_dispersion,
                                     water_prop.length_scaling, mu_satp));
  
  const double EperVolume = f(water_prop.kT, -water_prop.kT*log(n_1atm));

  Functional X = Xassociation(water_prop.lengthscale, water_prop.epsilonAB, 
                              water_prop.kappaAB, water_prop.epsilon_dispersion,
                              water_prop.lambda_dispersion,
                              water_prop.length_scaling);
  
  Functional S = OfEffectivePotential(SaftEntropy(water_prop.lengthscale, 
                                                  water_prop.epsilonAB,
                                                  water_prop.kappaAB,
                                                  water_prop.epsilon_dispersion,
                                                  water_prop.lambda_dispersion,
                                                  water_prop.length_scaling));
  for (distance=1*nm; distance<=dmax; distance += .1*nm) {
    Lattice lat(Cartesian(width,0,0), Cartesian(0,ymax,0), Cartesian(0,0,zmax));
    GridDescription gd(lat, 0.2);
    
    Grid potential(gd);
    Grid constraint(gd);
    constraint.Set(notinwall);
    
    f = OfEffectivePotential(SaftFluid(water_prop.lengthscale,
                                       water_prop.epsilonAB, water_prop.kappaAB,
                                       water_prop.epsilon_dispersion,
                                       water_prop.lambda_dispersion,
                                       water_prop.length_scaling, mu_satp));
    f = constrain(constraint, f);
    
    // constraint.epsNativeSlice("paper/figs/rods-in-water-constraint.eps",
    // 			      Cartesian(0,ymax,0), Cartesian(0,0,zmax), 
    // 			      Cartesian(0,ymax/2,zmax/2));
    //printf("Constraint has become a graph!\n");
   
    potential = water_prop.liquid_density*constraint
      + 400*water_prop.vapor_density*VectorXd::Ones(gd.NxNyNz);
    //potential = water_prop.liquid_density*VectorXd::Ones(gd.NxNyNz);
    potential = -water_prop.kT*potential.cwise().log();
    
    // {
    //    fflush(stdout);
    //    Grid density(gd, EffectivePotentialToDensity()(water_prop.kT, gd, potential));
    //    //density.epsNative1d("paper/figs/constrained-water-1D.eps",
    //    //			Cartesian(0,0,0), Cartesian(0,0,zmax),
    //    //			water_prop.liquid_density, 1, " ");
    
    //    //sleep(3);
    //  }
    
    Minimizer min = Precision(1e-12, PreconditionedConjugateGradient(f, gd, water_prop.kT, 
                                                                     &potential,
                                                                     QuadraticLineMinimizer));
    printf("Diameter is %g bohr (%g nm)\n", diameter, diameter/nm);
    printf("Distance between rods = %g bohr (%g nm)\n", distance, distance/nm);
    
    const int numiters = 200;
    for (int i=0;i<numiters && min.improve_energy(true);i++) {
      fflush(stdout);
      //Grid density(gd, EffectivePotentialToDensity()(water_prop.kT, gd, potential));
      //density.epsNative1d("paper/figs/constrained-water-1D.eps",
      //			Cartesian(0,0,0), Cartesian(0,0,zmax),
      //			water_prop.liquid_density, 1, " ");
      
      //density.epsNativeSlice("paper/figs/rods-in-water.eps", 
      //			     Cartesian(0,ymax,0), Cartesian(0,0,zmax), 
      //		     Cartesian(0,ymax/2,zmax/2));
      
      //sleep(3);
    }
    //min.print_info();

    // char *plotnamedens = (char *)malloc(1024);
    // sprintf(plotnamedens, "paper/figs/rods-density-1nm-%04.1f.dat", distance/nm);
    // density.epsNativeSlice(plotnamedens, Cartesian(0,ymax,0), Cartesian(0,0,zmax), 
    // 			   Cartesian(0,ymax/2,zmax/2));
    // free(plotnamedens);

    const double EperCell = EperVolume*(zmax*ymax - 4*0.25*M_PI*diameter*diameter)*width;
    printf("The bulk energy per cell should be %g\n", EperCell);
    double energy = (min.energy() - EperCell)/width;
    printf("Energy is %.15g\n", energy);

    fprintf(o, "%g\t%.15g\n", distance/nm, energy);

    char *plotnameslice = new char[1024];
    snprintf(plotnameslice, 1024, "paper/figs/four-rods-%04.1f-%04.1f.dat", diameter/nm, distance/nm);
    Grid density(gd, EffectivePotentialToDensity()(water_prop.kT, gd, potential));
    Grid energy_density(gd, f(water_prop.kT, gd, potential));
    Grid entropy(gd, S(water_prop.kT, potential));
    Grid Xassoc(gd, X(water_prop.kT, density));
    //plot_grids_y_direction(plotnameslice, density, energy_density, entropy, Xassoc);
    plot_grids_yz_directions(plotnameslice, density, energy_density, entropy, Xassoc);
    delete[] plotnameslice;
  }
  fclose(o);
}
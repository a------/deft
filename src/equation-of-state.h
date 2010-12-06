// -*- mode: C++; -*-

#pragma once

#include "Minimizer.h"
#include "GridDescription.h"

struct LiquidProperties {
  double lengthscale, liquid_density, vapor_density, kT;
  double epsilon_dispersion, lambda_dispersion, epsilonAB, kappaAB;
};

extern LiquidProperties water_prop;
static const double atmospheric_pressure = 101325*3.3989316e-14; // in Hartree/bohr^3

// Ultimately, I'd like surface_tension to be smart about finding a
// converged value.  Or maybe just a second function to do that?
double surface_tension(Minimizer min, Functional f, LiquidProperties prop, bool verbose);

double find_density(Functional f, double kT, double nmin, double nmax);
double pressure(Functional f, double kT, double density);
double pressure_to_density(Functional f, double kT, double p,
                           double nmin = 1e-10, double nmax = 1e-2);
double find_chemical_potential(Functional f, double kT, double n);

double chemical_potential_to_density(Functional f, double kT, double mu,
                                     double nmin = 1e-10, double nmax = 1e-2);
double saturated_liquid(Functional f, double kT,
                        double nmin = 1e-10, double nmax = 1e-2);
double coexisting_vapor_density(Functional f, double kT, double liquid_density);

void equation_of_state(FILE *o, Functional f, double kT, double nmin, double nmax);
void other_equation_of_state(FILE *o, Functional f, double kT, double nmin, double nmax);

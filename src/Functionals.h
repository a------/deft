// -*- mode: C++; -*-

#pragma once

#include "Grid.h"
#include "Functional.h"
#include "FieldFunctional.h"

Functional IdealGas(double temperature);
Functional HardSpheres(double radius, double temperature);
Functional ChemicalPotential(double chemical_potential);
Functional ExternalPotential(const Grid &V);

Functional GaussianPolynomial(double amplitude, double width, int power);

FieldFunctional Identity();

FieldFunctional EffectivePotentialToDensity(double temperature);
FieldFunctional Gaussian(double width);

FieldFunctional StepConvolve(double radius);

FieldFunctional ShellConvolve(double radius);

FieldFunctional xShellConvolve(double radius);
FieldFunctional yShellConvolve(double radius);
FieldFunctional zShellConvolve(double radius);

FieldFunctional xxShellConvolve(double radius);
FieldFunctional yyShellConvolve(double radius);
FieldFunctional zzShellConvolve(double radius);
FieldFunctional xyShellConvolve(double radius);
FieldFunctional yzShellConvolve(double radius);
FieldFunctional zxShellConvolve(double radius);

FieldFunctional Pow(int power);

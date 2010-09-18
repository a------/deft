// -*- mode: C++; -*-

#pragma once

#include <string>

class Expression {
public:
  Expression();
  Expression(double); // Constant literal
  Expression(std::string); // Variable
  Expression(const Expression &);
  void operator=(const Expression &);
  ~Expression() {
    delete arg1;
    delete arg2;
  }

  std::string name, type;
  Expression *arg1, *arg2, *arg3;
  double value;

  Expression operator+(const Expression &) const;
  Expression operator-() const;
  Expression operator-(const Expression &) const;
  Expression operator*(const Expression &) const;
  Expression operator/(const Expression &) const;
  Expression operator()(const Expression &) const;
  Expression method(const char *) const;
  Expression method(const char *, const Expression &) const;
  Expression method(const char *, const Expression &, const Expression &) const;

  std::string printme() const;
};

Expression funexpr(const char *name);
Expression funexpr(const char *name, const Expression &);
Expression funexpr(const char *name, const Expression &, const Expression &);
Expression funexpr(const char *name, const Expression &, const Expression &, const Expression &);

inline Expression operator+(double a, const Expression &b) {
  return Expression(a) + b;
}

inline Expression operator-(double a, const Expression &b) {
  return Expression(a) - b;
}

inline Expression operator*(double a, const Expression &b) {
  return Expression(a) * b;
}

inline Expression operator/(double a, const Expression &b) {
  return Expression(a) / b;
}
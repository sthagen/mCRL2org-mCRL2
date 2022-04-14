// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file boolean_expression_test.cpp
/// \brief Test for boolean expressions.

#define BOOST_TEST_MODULE boolean_expression_test
#include "mcrl2/bes/bes2pbes.h"
#include "mcrl2/bes/io.h"
#include "mcrl2/bes/print.h"

#include <boost/test/included/unit_test_framework.hpp>

using namespace mcrl2;

void test_boolean_expressions()
{
  using namespace bes;
  typedef core::term_traits<boolean_expression> tr;

  boolean_variable X1("X1");
  boolean_variable X2("X2");
  boolean_expression t1 = tr::and_(X1, X2);
  boolean_equation e1(fixpoint_symbol::mu(), X1, tr::imp(X1, X2));
  boolean_equation e2(fixpoint_symbol::nu(), X2, tr::or_(X1, X2));
  std::cout << bes::pp(e1) << std::endl;
  std::cout << bes::pp(e2) << std::endl;

  boolean_equation_system p;
  p.equations().push_back(e1);
  p.equations().push_back(e2);
  p.initial_state() = X1;
  std::cout << "----------------" << std::endl;
  std::cout << bes::pp(p) << std::endl;

  std::string filename = "boolean_expression_test.out";
  save_bes(p, filename);
  boolean_equation_system q;
  load_bes(q, filename);
  BOOST_CHECK(p == q);
  remove(filename.c_str());
}

void test_bes2pbes()
{
  using namespace bes;
  typedef core::term_traits<boolean_expression> tr;

  boolean_variable X1("X1");
  boolean_variable X2("X2");
  boolean_variable X3("X3");
  boolean_expression t1 = tr::and_(X1, X2);
  boolean_equation e1(fixpoint_symbol::mu(), X1, tr::imp(X1, X2));
  boolean_equation e2(fixpoint_symbol::nu(), X2, tr::or_(X1, X2));
  boolean_equation e3(fixpoint_symbol::nu(), X3, tr::false_());
  std::cout << bes::pp(e1) << std::endl;
  std::cout << bes::pp(e2) << std::endl;
  std::cout << bes::pp(e3) << std::endl;

  boolean_equation_system p;
  p.equations().push_back(e1);
  p.equations().push_back(e2);
  p.equations().push_back(e3);
  p.initial_state() = X1;
  std::cout << "----------------" << std::endl;
  std::cout << bes::pp(p) << std::endl;

  pbes_system::pbes q = bes2pbes(p);
  std::cout << "----------------" << std::endl;
  std::cout << q << std::endl;
}

void test_precedence()
{
  using namespace bes;
  typedef core::term_traits<boolean_expression> tr;
  boolean_variable X1("X1");
  boolean_variable X2("X2");
  boolean_expression t = tr::and_(X1, X2);
  std::string s = bes::pp(t);
  BOOST_CHECK(s == "X1 && X2");
}

BOOST_AUTO_TEST_CASE(test_main)
{
  test_boolean_expressions();
  test_bes2pbes();
  test_precedence();
}

// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file traverser_test.cpp
/// \brief Test for traversers.

#define BOOST_TEST_MODULE traverser_test
#include "mcrl2/bes/parse.h"
#include "mcrl2/bes/traverser.h"

#include <boost/test/included/unit_test_framework.hpp>

using namespace mcrl2;
using namespace mcrl2::bes;

class custom_traverser: public boolean_expression_traverser<custom_traverser>
{
  public:
    typedef boolean_expression_traverser<custom_traverser> super;

    using super::enter;
    using super::leave;
    using super::apply;
};

void test_custom_traverser()
{
  custom_traverser t;

  boolean_variable v;
  t.apply(v);

  true_ T;
  t.apply(T);

  boolean_expression e;
  t.apply(e);

  boolean_equation eq;
  t.apply(eq);

  boolean_equation_system eqn;
  t.apply(eqn);

}

class traverser1: public boolean_variable_traverser<traverser1>
{
  public:
    typedef boolean_variable_traverser<traverser1> super;

    using super::enter;
    using super::leave;
    using super::apply;

    unsigned int variable_count;
    unsigned int equation_count;
    unsigned int expression_count;

    traverser1()
      : variable_count(0),
        equation_count(0),
        expression_count(0)
    {}

    void enter(const boolean_variable& v)
    {
      variable_count++;
    }

    void enter(const boolean_equation& eq)
    {
      equation_count++;
    }

    void enter(const boolean_expression& x)
    {
      expression_count++;
    }
};

void test_traverser1()
{
  traverser1 t1;
  boolean_expression x = boolean_variable("X");
  t1.apply(x);

  BOOST_CHECK(t1.variable_count == 1);
  BOOST_CHECK(t1.expression_count == 1);
  BOOST_CHECK(t1.equation_count == 0);

  //--------------------------//

  traverser1 t2;

  std::string bes1 =
    "pbes              \n"
    "                  \n"
    "nu X1 = X2 && X1; \n"
    "mu X2 = X1 || X2; \n"
    "                  \n"
    "init X1;          \n"
    ;
  boolean_equation_system b;
  std::stringstream from(bes1);
  from >> b;

  t2.apply(b);

  BOOST_CHECK(t2.variable_count == 7);
  BOOST_CHECK(t2.expression_count == 7);
  BOOST_CHECK(t2.equation_count == 2);

}

BOOST_AUTO_TEST_CASE(test_main)
{
  test_custom_traverser();
  test_traverser1();
}

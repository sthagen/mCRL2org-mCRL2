// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file bes_io_test.cpp
/// \brief Some io tests for boolean equation systems.

#define BOOST_TEST_MODULE bes_io_test
#include <boost/test/included/unit_test_framework.hpp>

#include "mcrl2/bes/io.h"
#include "mcrl2/bes/parse.h"
#include "mcrl2/bes/print.h"
#include "mcrl2/bes/pbesinst_conversion.h"

using namespace mcrl2;
using namespace mcrl2::bes;

std::string bes1 =
  "pbes              \n"
  "                  \n"
  "nu X1 = X2 && X1; \n"
  "mu X2 = X1 || X2; \n"
  "                  \n"
  "init X1;          \n"
  ;

void test_parse_bes()
{
  boolean_equation_system b;
  std::stringstream from(bes1);
  from >> b;
  std::cout << "b = \n" << bes::pp(b) << std::endl;

  // check if the pretty printed BES can be parsed again
  std::string bes2 = bes::pp(b);
  std::stringstream from2(bes1);
  from2 >> b;

}

void test_bes()
{
  boolean_equation_system b;
  std::stringstream bes_stream(bes1);
  bes_stream >> b;

  std::stringstream out;
  bes::save_bes(b, out, bes_format_internal());
}

void test_pbes()
{
  pbes_system::pbes b;
  std::stringstream bes_stream(bes1);
  bes_stream >> b;

  std::stringstream out;
  bes::save_pbes(b, out, bes_format_internal());
}

void test_pgsolver()
{
  boolean_equation_system b;
  std::stringstream bes_stream(bes1);
  bes_stream >> b;

  std::stringstream out;
  bes::save_bes_pgsolver(b, out);

  std::clog << out.str() << std::endl;
}

BOOST_AUTO_TEST_CASE(test_main)
{
  test_parse_bes();
  test_bes();
  test_pbes();
  test_pgsolver();
}

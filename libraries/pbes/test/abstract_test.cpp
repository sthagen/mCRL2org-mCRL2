// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file abstract_test.cpp
/// \brief Test the pbes abstract algorithm.

#define BOOST_TEST_MODULE abstract_test
#include <boost/test/included/unit_test.hpp>
#include "mcrl2/pbes/abstract.h"
#include "mcrl2/pbes/txt2pbes.h"

using namespace mcrl2;
using namespace mcrl2::pbes_system;

void test_pbesabstract(const std::string& pbes_spec, const std::string& variable_spec, bool value_true)
{
  pbes p = txt2pbes(pbes_spec);
  pbes_system::detail::pbes_parameter_map parameter_map = pbes_system::detail::parse_pbes_parameter_map(p, variable_spec);
  pbes_abstract_algorithm algorithm;
  algorithm.run(p, parameter_map, value_true);
  std::cout << "\n-------------------------------\n" << pbes_system::pp(p) << std::endl;
}

BOOST_AUTO_TEST_CASE(pbesabstract)
{
  test_pbesabstract(
    "pbes nu X(a: Bool, b: Nat) =  \n"
    "       val(a) || X(a, b + 1); \n"
    "                              \n"
    "init X(true, 0);              \n"
    ,
    "X(b:Nat)"
    ,
    true
  );

  test_pbesabstract(
    "pbes nu X1(b:Bool) = exists b:Bool.(X2 || val(b)); \n"
    "     mu X2 = X2;                                   \n"
    "                                                   \n"
    "init X1(true);                                     \n"
    ,
    "X1(b:Bool)"
    ,
    true
  );

  test_pbesabstract(
    "pbes nu X1(b:Bool) = X2 || val(b); \n"
    "     mu X2 = X2;                   \n"
    "                                   \n"
    "init X1(true);                     \n"
    ,
    "X1(b:Bool)"
    ,
    true
  );
}

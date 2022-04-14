// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file parelm_test.cpp
/// \brief Add your file description here.

#define BOOST_TEST_MODULE parelm_test
#include <boost/test/included/unit_test_framework.hpp>

#include "mcrl2/lps/detail/test_input.h"
#include "mcrl2/modal_formula/parse.h"
#include "mcrl2/pbes/complement.h"
#include "mcrl2/pbes/lps2pbes.h"
#include "mcrl2/pbes/parelm.h"

using namespace mcrl2;
using namespace mcrl2::pbes_system;

const std::string SPECIFICATION =
  "act a:Nat;                               \n"
  "                                         \n"
  "map smaller: Nat#Nat -> Bool;            \n"
  "                                         \n"
  "var x,y : Nat;                           \n"
  "                                         \n"
  "eqn smaller(x,y) = x < y;                \n"
  "                                         \n"
  "proc P(n:Nat) = sum m: Nat. a(m). P(m);  \n"
  "                                         \n"
  "init P(0);                               \n";

const std::string TRIVIAL_FORMULA  = "[true*]<true*>true";

BOOST_AUTO_TEST_CASE(test_parelm1)
{
  lps::specification spec = lps::remove_stochastic_operators(lps::linearise(lps::detail::ABP_SPECIFICATION()));
  state_formulas::state_formula formula = state_formulas::parse_state_formula(TRIVIAL_FORMULA, spec);
  bool timed = false;
  pbes p = lps2pbes(spec, formula, timed);
  pbes_parelm_algorithm algorithm;
  algorithm.run(p);
  BOOST_CHECK(p.is_well_typed());
}

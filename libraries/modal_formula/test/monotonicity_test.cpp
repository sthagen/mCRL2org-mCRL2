// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file monotonicity_test.cpp
/// \brief Tests for the is_monotonous function for state formulas.

#include <boost/test/included/unit_test_framework.hpp>

#include "mcrl2/lps/detail/test_input.h"
#include "mcrl2/lps/linearise.h"
#include "mcrl2/modal_formula/parse.h"

using namespace mcrl2;
using namespace mcrl2::lps;
using namespace mcrl2::state_formulas;

void run_monotonicity_test_case(const std::string& formula, const std::string& lpstext, const bool expect_success = true)
{
  specification lpsspec = remove_stochastic_operators(linearise(lpstext));
  parse_state_formula_options options;
  options.check_monotonicity = false;
  options.resolve_name_clashes = false;
  state_formula f = parse_state_formula(formula, lpsspec, options);
  if (state_formulas::has_state_variable_name_clashes(f))
  {
    std::cerr << "Error: " << state_formulas::pp(f) << " has name clashes" << std::endl;
    f = state_formulas::resolve_state_variable_name_clashes(f);
    std::cerr << "resolved to " << state_formulas::pp(f) << std::endl;
  }
  BOOST_CHECK(is_monotonous(f) == expect_success);
}

BOOST_AUTO_TEST_CASE(test_abp)
{
  std::string lpstext = lps::detail::ABP_SPECIFICATION();

  run_monotonicity_test_case("true", lpstext, true);
  run_monotonicity_test_case("[true*]<true*>true", lpstext, true);
  run_monotonicity_test_case("mu X. !!X", lpstext, true);
  run_monotonicity_test_case("nu X. ([true]X && <true>true)", lpstext, true);
  run_monotonicity_test_case("nu X. ([true]X && forall d:D. [r1(d)] mu Y. (<true>Y || <s4(d)>true))", lpstext, true);
  run_monotonicity_test_case("forall d:D. nu X. (([!r1(d)]X && [s4(d)]false))", lpstext, true);
  run_monotonicity_test_case("nu X. ([true]X && forall d:D. [r1(d)]nu Y. ([!r1(d) && !s4(d)]Y && [r1(d)]false))", lpstext, true);
  run_monotonicity_test_case("mu X. !X", lpstext, false);
  run_monotonicity_test_case("mu X. nu Y. (X => Y)", lpstext, false);
  run_monotonicity_test_case("mu X. X || mu X. X", lpstext, true);
  run_monotonicity_test_case("mu X. (X || mu X. X)", lpstext, true);
  run_monotonicity_test_case("mu X. (X || mu Y. Y)", lpstext, true);
  run_monotonicity_test_case("!(mu X. X || mu X. X)", lpstext, true);
  run_monotonicity_test_case("!(mu X. (X || mu X. X))", lpstext, true);
  run_monotonicity_test_case("!(mu X. (X || mu Y. Y))", lpstext, true);
}

// Test case provided by Jeroen Keiren, 10-9-2010
BOOST_AUTO_TEST_CASE(test_elevator)
{
  std::string lpstext =

    "% Model of an elevator for n floors.                                                                                           \n"
    "% Originally described in 'Solving Parity Games in Practice' by Oliver                                                         \n"
    "% Friedmann and Martin Lange.                                                                                                  \n"
    "%                                                                                                                              \n"
    "% This is the version with a first in first out policy                                                                         \n"
    "                                                                                                                               \n"
    "sort Floor = Pos;                                                                                                              \n"
    "     DoorStatus = struct open | closed;                                                                                        \n"
    "     Requests = List(Floor);                                                                                                   \n"
    "                                                                                                                               \n"
    "map maxFloor: Floor;                                                                                                           \n"
    "eqn maxFloor = 3;                                                                                                              \n"
    "                                                                                                                               \n"
    "map addRequest : Requests # Floor -> Requests;                                                                                 \n"
    "                                                                                                                               \n"
    "var r: Requests;                                                                                                               \n"
    "    f,g: Floor;                                                                                                                \n"
    "    % FIFO behaviour!                                                                                                          \n"
    "eqn addRequest([], f) = [f];                                                                                                   \n"
    "    (f == g) -> addRequest(g |> r, f) = g |> r;                                                                                \n"
    "    (f != g) -> addRequest(g |> r, f) = g |> addRequest(r, f);                                                                 \n"
    "                                                                                                                               \n"
    "map removeRequest : Requests -> Requests;                                                                                      \n"
    "var r: Requests;                                                                                                               \n"
    "    f: Floor;                                                                                                                  \n"
    "eqn removeRequest(f |> r) = r;                                                                                                 \n"
    "                                                                                                                               \n"
    "map getNext : Requests -> Floor;                                                                                               \n"
    "var r: Requests;                                                                                                               \n"
    "    f: Floor;                                                                                                                  \n"
    "eqn getNext(f |> r) = f;                                                                                                       \n"
    "                                                                                                                               \n"
    "act isAt: Floor;                                                                                                               \n"
    "    request: Floor;                                                                                                            \n"
    "    close, open, up, down;                                                                                                     \n"
    "                                                                                                                               \n"
    "proc Elevator(at: Floor, status: DoorStatus, reqs: Requests, moving: Bool) =                                                   \n"
    "       isAt(at) . Elevator()                                                                                                   \n"
    "     + sum f: Floor. (f <= maxFloor) -> request(f) . Elevator(reqs = addRequest(reqs, f))                                      \n"
    "     + (status == open) -> close . Elevator(status = closed)                                                                   \n"
    "     + (status == closed && reqs != [] && getNext(reqs) > at) -> up . Elevator(at = at + 1, moving = true)                     \n"
    "     + (status == closed && reqs != [] && getNext(reqs) < at) -> down . Elevator(at = Int2Pos(at - 1), moving = true)          \n"
    "     + (status == closed && getNext(reqs) == at) -> open. Elevator(status = open, reqs = removeRequest(reqs), moving = false); \n"
    "                                                                                                                               \n"
    "init Elevator(1, open, [], false);                                                                                             \n"
    ;

  run_monotonicity_test_case("nu U. [true] U && ((mu V . nu W. !([!request(maxFloor)]!W && [request(maxFloor)]!V)) || (nu X . mu Y. [!isAt(maxFloor)] Y &&  [isAt(maxFloor)]X))", lpstext, true);
  run_monotonicity_test_case("nu U. [true] U && ((nu V . mu W. ([!request(maxFloor)]W && [request(maxFloor)]V)) => (nu X . mu Y. [!isAt(maxFloor)] Y &&  [isAt(maxFloor)]X))", lpstext, true);
  run_monotonicity_test_case("nu U. [true] U && (!(nu V . mu W. ([!request(maxFloor)]W && [request(maxFloor)]V)) || (nu X . mu Y. [!isAt(maxFloor)] Y &&  [isAt(maxFloor)]X))", lpstext, true);
  run_monotonicity_test_case("(nu X . mu Y. X) => true", lpstext, true);
  run_monotonicity_test_case("!(nu X . mu Y. X)", lpstext, true);
  run_monotonicity_test_case("mu X . X", lpstext, true);
  run_monotonicity_test_case("nu X . X", lpstext, true);
  run_monotonicity_test_case("mu X . !X", lpstext, false);
  run_monotonicity_test_case("nu X . !X", lpstext, false);
  run_monotonicity_test_case("!(mu X . X)", lpstext, true);
  run_monotonicity_test_case("!(nu X . X)", lpstext, true);
  run_monotonicity_test_case("(mu X . X) => true", lpstext, true);
  run_monotonicity_test_case("(nu X . X) => true", lpstext, true);
  run_monotonicity_test_case("!(mu X. (mu X. X))", lpstext, true);

  // trac ticket #1320
  run_monotonicity_test_case("!mu X. [true]X && mu X. [true]X", lpstext, true);
}

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[])
{
  return nullptr;
}

// Author(s): Gijs Kant
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file bqnf_quantifier_rewriter_test.cpp
/// \brief Test for the bqnf_quantifier rewriter.

#define BOOST_TEST_MODULE bqnf_quantifier_rewriter_test
#include "mcrl2/pbes/normalize.h"
#include "mcrl2/pbes/rewrite.h"
#include "mcrl2/pbes/rewriter.h"
#include "mcrl2/pbes/txt2pbes.h"

#include <boost/test/included/unit_test_framework.hpp>

using namespace mcrl2;
using namespace mcrl2::pbes_system;


void rewrite_bqnf_quantifier(const std::string& source_text, const std::string& target_text)
{
  pbes p = txt2pbes(source_text);
  bqnf_rewriter pbesr;
  pbes_rewrite(p, pbesr);
  normalize(p);
  //std::clog << pp(p);
  pbes target = txt2pbes(target_text);
  normalize(target);
  BOOST_CHECK(p==target);
}


void test_bqnf_quantifier_rewriter()
{
  // buffer.always_send_and_receive
  std::string source_text =
      "pbes nu X(n: Pos) =\n"
      "  forall d: Pos . (val(d < 3) => Y(d)) && (val(d > 5 && d < 7) => Z(d));\n"
      "mu Y(d: Pos) = true;\n"
      "mu Z(d: Pos) = true;\n"
      "init X(1);"
  ;
  std::string target_text =
      "pbes nu X(n: Pos) =\n"
      "  (forall d: Pos. val(!(d < 3)) || Y(d)) && (forall d: Pos. val(!(d > 5 && d < 7)) || Z(d));\n"
      "mu Y(d: Pos) = true;\n"
      "mu Z(d: Pos) = true;\n"
      "init X(1);"
  ;
  rewrite_bqnf_quantifier(source_text, target_text);
}


BOOST_AUTO_TEST_CASE(test_main)
{
  test_bqnf_quantifier_rewriter();
}

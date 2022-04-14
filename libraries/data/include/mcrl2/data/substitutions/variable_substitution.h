// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/substitutions/variable_assignment.h
/// \brief add your file description here.

#ifndef MCRL2_DATA_SUBSTITUTIONS_VARIABLE_SUBSTITUTION_H
#define MCRL2_DATA_SUBSTITUTIONS_VARIABLE_SUBSTITUTION_H

#include "mcrl2/data/is_simple_substitution.h"
#include "mcrl2/data/undefined.h"

namespace mcrl2 {

namespace data {

/// \brief Substitution that maps a single variable to a data expression
struct variable_substitution
{
  using variable_type = variable;
  using expression_type = data_expression;
  using argument_type = variable;

  const variable& lhs;
  const data_expression& rhs;

  variable_substitution(const variable& lhs_, const data_expression& rhs_)
  : lhs(lhs_),
    rhs(rhs_)
  {}

  const data_expression& operator()(const variable& x) const
  {
    if (x == lhs)
    {
      return rhs;
    }
    return x;
  }
};

} // namespace data

} // namespace mcrl2

#endif // MCRL2_DATA_SUBSTITUTIONS_VARIABLE_SUBSTITUTION_H

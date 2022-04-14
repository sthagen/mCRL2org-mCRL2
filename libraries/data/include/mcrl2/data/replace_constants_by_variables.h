// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/replace_constants_by_variables.h
/// \brief add your file description here.

#ifndef MCRL2_DATA_REPLACE_CONSTANTS_BY_VARIABLES_H
#define MCRL2_DATA_REPLACE_CONSTANTS_BY_VARIABLES_H

#include "mcrl2/data/rewriter.h"
#include "mcrl2/data/builder.h"

namespace mcrl2 {

namespace data {

namespace detail {

/// \brief Replace each constant data application c by a fresh variable v, and add extend the substitution sigma
/// with the assignment v := r(c). This can be used in rewriting, to avoid that c is rewritten by the rewriter
/// multiple times.
template <template <class> class Builder>
struct replace_constants_by_variables_builder: public Builder<replace_constants_by_variables_builder<Builder>>
{
  typedef Builder<replace_constants_by_variables_builder<Builder>> super;
  using super::apply;
  using super::update;

  data::set_identifier_generator id_generator;
  std::unordered_map<data::data_expression, data::variable> substitutions;
  const data::rewriter& r;
  data::mutable_indexed_substitution<>& sigma;

  bool is_constant(const data::data_expression& x) const
  {
    return data::find_free_variables(x).empty();
  }

  replace_constants_by_variables_builder(const data::rewriter& r_, data::mutable_indexed_substitution<>& sigma_)
    : r(r_), sigma(sigma_)
  {}

  data::data_expression apply(const data::application& x)
  {
    auto i = substitutions.find(x);
    if (i != substitutions.end())
    {
      return i->second;
    }
    else if (is_constant(x))
    {
      data::variable v(id_generator("@rewr_var"), x.sort());
      substitutions[x] = v;
      sigma[v] = r(x, sigma);
      return std::move(v);
    }
    else
    {
      return super::apply(x);
    }
  }
};

} // namespace detail

} // namespace data

} // namespace mcrl2

#endif // MCRL2_DATA_REPLACE_CONSTANTS_BY_VARIABLES_H

// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/index_traits.h
/// \brief add your file description here.

#ifndef MCRL2_PBES_INDEX_TRAITS_H
#define MCRL2_PBES_INDEX_TRAITS_H

#include "mcrl2/pbes/pbes_expression.h"

namespace mcrl2 {

namespace pbes_system {

inline
void on_delete_propositional_variable_instantiation(const atermpp::aterm& t)
{
  const pbes_system::propositional_variable_instantiation& v = atermpp::down_cast<const pbes_system::propositional_variable_instantiation>(t);
  atermpp::detail::index_traits<pbes_system::propositional_variable_instantiation, propositional_variable_key_type, 2>::erase(std::make_pair(v.name(), v.parameters()));
}

inline
void register_propositional_variable_instantiation_hooks()
{
  add_deletion_hook(core::detail::function_symbol_PropVarInst(), on_delete_propositional_variable_instantiation);
}

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_INDEX_TRAITS_H

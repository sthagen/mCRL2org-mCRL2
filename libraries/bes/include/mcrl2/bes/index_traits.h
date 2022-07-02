// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/bes/index_traits.h
/// \brief add your file description here.

#ifndef MCRL2_BES_INDEX_TRAITS_H
#define MCRL2_BES_INDEX_TRAITS_H

#include "mcrl2/bes/boolean_expression.h"

namespace mcrl2 {

namespace bes {

/* This function is not used. 
inline
void on_create_boolean_variable(const atermpp::aterm& t)
{
//  const boolean_variable& v = atermpp::down_cast<const boolean_variable>(t);
//  atermpp::detail::index_traits<boolean_variable, boolean_variable_key_type, 1>::insert(v.name());
} */

inline
void on_delete_boolean_variable(const atermpp::aterm& t)
{
  const boolean_variable& v = atermpp::down_cast<const boolean_variable>(t);
  atermpp::detail::index_traits<boolean_variable, boolean_variable_key_type, 1>::erase(v.name());
}

inline
void register_boolean_variable_hooks()
{
  add_deletion_hook(core::detail::function_symbol_BooleanVariable(), on_delete_boolean_variable);
}

} // namespace bes

} // namespace mcrl2

#endif // MCRL2_BES_INDEX_TRAITS_H

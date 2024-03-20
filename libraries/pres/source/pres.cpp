// Author(s): Jan Friso Groote. Based on source/pbes.cpp by Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file pres.cpp
/// \brief

#include "mcrl2/pres/detail/has_propositional_variables.h"
#include "mcrl2/pres/detail/instantiate_global_variables.h"
#include "mcrl2/pres/detail/is_well_typed.h"
#include "mcrl2/pres/detail/occurring_variable_visitor.h"
#include "mcrl2/pres/is_res.h"
#include "mcrl2/pres/parse_impl.h"
#include "mcrl2/pres/print.h"
#include "mcrl2/pres/translate_user_notation.h"

namespace mcrl2
{

namespace pres_system
{

//--- start generated pres_system overloads ---//
std::string pp(const pres_system::pres_equation_vector& x) { return pres_system::pp< pres_system::pres_equation_vector >(x); }
std::string pp(const pres_system::pres_expression_list& x) { return pres_system::pp< pres_system::pres_expression_list >(x); }
std::string pp(const pres_system::pres_expression_vector& x) { return pres_system::pp< pres_system::pres_expression_vector >(x); }
std::string pp(const pres_system::propositional_variable_list& x) { return pres_system::pp< pres_system::propositional_variable_list >(x); }
std::string pp(const pres_system::propositional_variable_vector& x) { return pres_system::pp< pres_system::propositional_variable_vector >(x); }
std::string pp(const pres_system::propositional_variable_instantiation_list& x) { return pres_system::pp< pres_system::propositional_variable_instantiation_list >(x); }
std::string pp(const pres_system::propositional_variable_instantiation_vector& x) { return pres_system::pp< pres_system::propositional_variable_instantiation_vector >(x); }
std::string pp(const pres_system::and_& x) { return pres_system::pp< pres_system::and_ >(x); }
std::string pp(const pres_system::or_& x) { return pres_system::pp< pres_system::or_ >(x); }
std::string pp(const pres_system::supremum& x) { return pres_system::pp< pres_system::supremum >(x); }
std::string pp(const pres_system::fixpoint_symbol& x) { return pres_system::pp< pres_system::fixpoint_symbol >(x); }
std::string pp(const pres_system::infimum& x) { return pres_system::pp< pres_system::infimum >(x); }
std::string pp(const pres_system::sum& x) { return pres_system::pp< pres_system::sum >(x); }
std::string pp(const pres_system::imp& x) { return pres_system::pp< pres_system::imp >(x); }
std::string pp(const pres_system::minus& x) { return pres_system::pp< pres_system::minus >(x); }
std::string pp(const pres_system::const_multiply& x) { return pres_system::pp< pres_system::const_multiply >(x); }
std::string pp(const pres_system::const_multiply_alt& x) { return pres_system::pp< pres_system::const_multiply_alt >(x); }
std::string pp(const pres_system::pres& x) { return pres_system::pp< pres_system::pres >(x); }
std::string pp(const pres_system::pres_equation& x) { return pres_system::pp< pres_system::pres_equation >(x); }
std::string pp(const pres_system::pres_expression& x) { return pres_system::pp< pres_system::pres_expression >(x); }
std::string pp(const pres_system::propositional_variable& x) { return pres_system::pp< pres_system::propositional_variable >(x); }
std::string pp(const pres_system::propositional_variable_instantiation& x) { return pres_system::pp< pres_system::propositional_variable_instantiation >(x); }
void normalize_sorts(pres_system::pres_equation_vector& x, const data::sort_specification& sortspec) { pres_system::normalize_sorts< pres_system::pres_equation_vector >(x, sortspec); }
void normalize_sorts(pres_system::pres& x, const data::sort_specification& /* sortspec */) { pres_system::normalize_sorts< pres_system::pres >(x, x.data()); }
pres_system::pres_expression normalize_sorts(const pres_system::pres_expression& x, const data::sort_specification& sortspec) { return pres_system::normalize_sorts< pres_system::pres_expression >(x, sortspec); }
void translate_user_notation(pres_system::pres& x) { pres_system::translate_user_notation< pres_system::pres >(x); }
pres_system::pres_expression translate_user_notation(const pres_system::pres_expression& x) { return pres_system::translate_user_notation< pres_system::pres_expression >(x); }
std::set<data::sort_expression> find_sort_expressions(const pres_system::pres& x) { return pres_system::find_sort_expressions< pres_system::pres >(x); }
std::set<data::variable> find_all_variables(const pres_system::pres& x) { return pres_system::find_all_variables< pres_system::pres >(x); }
std::set<data::variable> find_free_variables(const pres_system::pres& x) { return pres_system::find_free_variables< pres_system::pres >(x); }
std::set<data::variable> find_free_variables(const pres_system::pres_expression& x) { return pres_system::find_free_variables< pres_system::pres_expression >(x); }
std::set<data::variable> find_free_variables(const pres_system::pres_equation& x) { return pres_system::find_free_variables< pres_system::pres_equation >(x); }
std::set<data::function_symbol> find_function_symbols(const pres_system::pres& x) { return pres_system::find_function_symbols< pres_system::pres >(x); }
std::set<pres_system::propositional_variable_instantiation> find_propositional_variable_instantiations(const pres_system::pres_expression& x) { return pres_system::find_propositional_variable_instantiations< pres_system::pres_expression >(x); }
std::set<core::identifier_string> find_identifiers(const pres_system::pres_expression& x) { return pres_system::find_identifiers< pres_system::pres_expression >(x); }
bool search_variable(const pres_system::pres_expression& x, const data::variable& v) { return pres_system::search_variable< pres_system::pres_expression >(x, v); }
//--- end generated pres_system overloads ---//

namespace algorithms {

void instantiate_global_variables(pres& p)
{
  pres_system::detail::instantiate_global_variables(p);
}

bool is_res(const pres& x)
{
  return pres_system::is_res(x);
}

} // namespace algorithms

bool is_well_typed(const pres_equation& eqn)
{
  return pres_system::detail::is_well_typed(eqn);
}

bool is_well_typed_equation(const pres_equation& eqn,
                            const std::set<data::sort_expression>& declared_sorts,
                            const std::set<data::variable>& declared_global_variables,
                            const data::data_specification& data_spec
                           )
{
  return pres_system::detail::is_well_typed_equation(eqn, declared_sorts, declared_global_variables, data_spec);
}

bool is_well_typed_pres(const std::set<data::sort_expression>& declared_sorts,
                        const std::set<data::variable>& declared_global_variables,
                        const std::set<data::variable>& occurring_global_variables,
                        const std::set<propositional_variable>& declared_variables,
                        const std::set<propositional_variable_instantiation>& occ,
                        const propositional_variable_instantiation& init,
                        const data::data_specification& data_spec
                       )
{
  return pres_system::detail::is_well_typed_pres(declared_sorts, declared_global_variables, occurring_global_variables, declared_variables, occ, init, data_spec);
}

bool pres_equation::is_solved() const
{
  return !detail::has_propositional_variables(formula());
}

std::set<propositional_variable_instantiation> pres::occurring_variable_instantiations() const
{
  std::set<propositional_variable_instantiation> result;
  for (const pres_equation& eqn: equations())
  {
    detail::occurring_variable_visitor f;
    f.apply(eqn.formula());
    result.insert(f.variables.begin(), f.variables.end());
  }
  return result;
}

namespace detail {

pres_expression parse_pres_expression_new(const std::string& text)
{
  core::parser p(parser_tables_mcrl2, core::detail::ambiguity_fn, core::detail::syntax_error_fn);
  unsigned int start_symbol_index = p.start_symbol_index("PresExpr");
  bool partial_parses = false;
  core::parse_node node = p.parse(text, start_symbol_index, partial_parses);
  core::warn_and_or(node);
  pres_expression result = pres_actions(p).parse_PresExpr(node);
  return result;
}

untyped_pres parse_pres_new(const std::string& text)
{
  core::parser p(parser_tables_mcrl2, core::detail::ambiguity_fn, core::detail::syntax_error_fn);
  unsigned int start_symbol_index = p.start_symbol_index("PresSpec");
  bool partial_parses = false;
  core::parse_node node = p.parse(text, start_symbol_index, partial_parses);
  core::warn_and_or(node);
  untyped_pres result = pres_actions(p).parse_PresSpec(node);
  return result;
}

void complete_pres(pres& x)
{
  typecheck_pres(x);
  pres_system::translate_user_notation(x);
  complete_data_specification(x);
}

propositional_variable parse_propositional_variable(const std::string& text)
{
  core::parser p(parser_tables_mcrl2, core::detail::ambiguity_fn, core::detail::syntax_error_fn);
  unsigned int start_symbol_index = p.start_symbol_index("PropVarDecl");
  bool partial_parses = false;
  core::parse_node node = p.parse(text, start_symbol_index, partial_parses);
  return detail::pres_actions(p).parse_PropVarDecl(node);
}

pres_expression parse_pres_expression(const std::string& text)
{
  core::parser p(parser_tables_mcrl2, core::detail::ambiguity_fn, core::detail::syntax_error_fn);
  unsigned int start_symbol_index = p.start_symbol_index("PresExpr");
  bool partial_parses = false;
  core::parse_node node = p.parse(text, start_symbol_index, partial_parses);
  core::warn_and_or(node);
  return detail::pres_actions(p).parse_PresExpr(node);
}

} // namespace detail

} // namespace pres_system

} // namespace mcrl2


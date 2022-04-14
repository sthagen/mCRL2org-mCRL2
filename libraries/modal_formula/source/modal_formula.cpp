// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file modal_formula.cpp
/// \brief

#include "mcrl2/modal_formula/algorithms.h"
#include "mcrl2/modal_formula/is_timed.h"
#include "mcrl2/modal_formula/normalize.h"
#include "mcrl2/modal_formula/parse.h"
#include "mcrl2/modal_formula/parse_impl.h"
#include "mcrl2/modal_formula/print.h"
#include "mcrl2/modal_formula/replace.h"

namespace mcrl2
{

namespace action_formulas
{

//--- start generated action_formulas overloads ---//
std::string pp(const action_formulas::action_formula& x) { return action_formulas::pp< action_formulas::action_formula >(x); }
std::string pp(const action_formulas::and_& x) { return action_formulas::pp< action_formulas::and_ >(x); }
std::string pp(const action_formulas::at& x) { return action_formulas::pp< action_formulas::at >(x); }
std::string pp(const action_formulas::exists& x) { return action_formulas::pp< action_formulas::exists >(x); }
std::string pp(const action_formulas::false_& x) { return action_formulas::pp< action_formulas::false_ >(x); }
std::string pp(const action_formulas::forall& x) { return action_formulas::pp< action_formulas::forall >(x); }
std::string pp(const action_formulas::imp& x) { return action_formulas::pp< action_formulas::imp >(x); }
std::string pp(const action_formulas::multi_action& x) { return action_formulas::pp< action_formulas::multi_action >(x); }
std::string pp(const action_formulas::not_& x) { return action_formulas::pp< action_formulas::not_ >(x); }
std::string pp(const action_formulas::or_& x) { return action_formulas::pp< action_formulas::or_ >(x); }
std::string pp(const action_formulas::true_& x) { return action_formulas::pp< action_formulas::true_ >(x); }
std::set<data::variable> find_all_variables(const action_formulas::action_formula& x) { return action_formulas::find_all_variables< action_formulas::action_formula >(x); }
//--- end generated action_formulas overloads ---//

namespace detail {

action_formula parse_action_formula(const std::string& text)
{
  core::parser p(parser_tables_mcrl2, core::detail::ambiguity_fn, core::detail::syntax_error_fn);
  unsigned int start_symbol_index = p.start_symbol_index("ActFrm");
  bool partial_parses = false;
  core::parse_node node = p.parse(text, start_symbol_index, partial_parses);
  core::warn_and_or(node);
  action_formula result = action_formula_actions(p).parse_ActFrm(node);
  return result;
}

} // namespace detail

} // namespace action_formulas

namespace regular_formulas
{

//--- start generated regular_formulas overloads ---//
std::string pp(const regular_formulas::alt& x) { return regular_formulas::pp< regular_formulas::alt >(x); }
std::string pp(const regular_formulas::regular_formula& x) { return regular_formulas::pp< regular_formulas::regular_formula >(x); }
std::string pp(const regular_formulas::seq& x) { return regular_formulas::pp< regular_formulas::seq >(x); }
std::string pp(const regular_formulas::trans& x) { return regular_formulas::pp< regular_formulas::trans >(x); }
std::string pp(const regular_formulas::trans_or_nil& x) { return regular_formulas::pp< regular_formulas::trans_or_nil >(x); }
std::string pp(const regular_formulas::untyped_regular_formula& x) { return regular_formulas::pp< regular_formulas::untyped_regular_formula >(x); }
//--- end generated regular_formulas overloads ---//

namespace detail
{

regular_formula parse_regular_formula(const std::string& text)
{
  core::parser p(parser_tables_mcrl2, core::detail::ambiguity_fn, core::detail::syntax_error_fn);
  unsigned int start_symbol_index = p.start_symbol_index("RegFrm");
  bool partial_parses = false;
  core::parse_node node = p.parse(text, start_symbol_index, partial_parses);
  regular_formula result = regular_formula_actions(p).parse_RegFrm(node);
  return result;
}

} // namespace detail

} // namespace regular_formulas

namespace state_formulas
{

//--- start generated state_formulas overloads ---//
std::string pp(const state_formulas::and_& x) { return state_formulas::pp< state_formulas::and_ >(x); }
std::string pp(const state_formulas::delay& x) { return state_formulas::pp< state_formulas::delay >(x); }
std::string pp(const state_formulas::delay_timed& x) { return state_formulas::pp< state_formulas::delay_timed >(x); }
std::string pp(const state_formulas::exists& x) { return state_formulas::pp< state_formulas::exists >(x); }
std::string pp(const state_formulas::false_& x) { return state_formulas::pp< state_formulas::false_ >(x); }
std::string pp(const state_formulas::forall& x) { return state_formulas::pp< state_formulas::forall >(x); }
std::string pp(const state_formulas::imp& x) { return state_formulas::pp< state_formulas::imp >(x); }
std::string pp(const state_formulas::may& x) { return state_formulas::pp< state_formulas::may >(x); }
std::string pp(const state_formulas::mu& x) { return state_formulas::pp< state_formulas::mu >(x); }
std::string pp(const state_formulas::must& x) { return state_formulas::pp< state_formulas::must >(x); }
std::string pp(const state_formulas::not_& x) { return state_formulas::pp< state_formulas::not_ >(x); }
std::string pp(const state_formulas::nu& x) { return state_formulas::pp< state_formulas::nu >(x); }
std::string pp(const state_formulas::or_& x) { return state_formulas::pp< state_formulas::or_ >(x); }
std::string pp(const state_formulas::state_formula& x) { return state_formulas::pp< state_formulas::state_formula >(x); }
std::string pp(const state_formulas::state_formula_specification& x) { return state_formulas::pp< state_formulas::state_formula_specification >(x); }
std::string pp(const state_formulas::true_& x) { return state_formulas::pp< state_formulas::true_ >(x); }
std::string pp(const state_formulas::variable& x) { return state_formulas::pp< state_formulas::variable >(x); }
std::string pp(const state_formulas::yaled& x) { return state_formulas::pp< state_formulas::yaled >(x); }
std::string pp(const state_formulas::yaled_timed& x) { return state_formulas::pp< state_formulas::yaled_timed >(x); }
state_formulas::state_formula normalize_sorts(const state_formulas::state_formula& x, const data::sort_specification& sortspec) { return state_formulas::normalize_sorts< state_formulas::state_formula >(x, sortspec); }
state_formulas::state_formula translate_user_notation(const state_formulas::state_formula& x) { return state_formulas::translate_user_notation< state_formulas::state_formula >(x); }
std::set<data::sort_expression> find_sort_expressions(const state_formulas::state_formula& x) { return state_formulas::find_sort_expressions< state_formulas::state_formula >(x); }
std::set<data::variable> find_all_variables(const state_formulas::state_formula& x) { return state_formulas::find_all_variables< state_formulas::state_formula >(x); }
std::set<data::variable> find_free_variables(const state_formulas::state_formula& x) { return state_formulas::find_free_variables< state_formulas::state_formula >(x); }
std::set<core::identifier_string> find_identifiers(const state_formulas::state_formula& x) { return state_formulas::find_identifiers< state_formulas::state_formula >(x); }
std::set<process::action_label> find_action_labels(const state_formulas::state_formula& x) { return state_formulas::find_action_labels< state_formulas::state_formula >(x); }
//--- end generated state_formulas overloads ---//

namespace detail {

state_formula parse_state_formula(const std::string& text)
{
  core::parser p(parser_tables_mcrl2, core::detail::ambiguity_fn, core::detail::syntax_error_fn);
  unsigned int start_symbol_index = p.start_symbol_index("StateFrm");
  bool partial_parses = false;
  core::parse_node node = p.parse(text, start_symbol_index, partial_parses);
  core::warn_and_or(node);
  state_formula result = state_formula_actions(p).parse_StateFrm(node);
  return result;
}

state_formula_specification parse_state_formula_specification(const std::string& text)
{
  core::parser p(parser_tables_mcrl2, core::detail::ambiguity_fn, core::detail::syntax_error_fn);
  unsigned int start_symbol_index = p.start_symbol_index("StateFrmSpec");
  bool partial_parses = false;
  core::parse_node node = p.parse(text, start_symbol_index, partial_parses);
  core::warn_and_or(node);
  core::warn_left_merge_merge(node);

  untyped_state_formula_specification untyped_statespec = state_formula_actions(p).parse_StateFrmSpec(node);
  state_formula_specification result = untyped_statespec.construct_state_formula_specification();
  return result;
}

} // namespace detail

namespace algorithms {

state_formula parse_state_formula(std::istream& in, lps::specification& lpsspec)
{
  return state_formulas::parse_state_formula(in, lpsspec);
}

state_formula parse_state_formula(const std::string& text, lps::specification& lpsspec)
{
  return state_formulas::parse_state_formula(text, lpsspec);
}

state_formula_specification parse_state_formula_specification(std::istream& in)
{
  return state_formulas::parse_state_formula_specification(in);
}

state_formula_specification parse_state_formula_specification(const std::string& text)
{
  return state_formulas::parse_state_formula_specification(text);
}

state_formula_specification parse_state_formula_specification(std::istream& in, lps::specification& lpsspec)
{
  return state_formulas::parse_state_formula_specification(in, lpsspec);
}

state_formula_specification parse_state_formula_specification(const std::string& text, lps::specification& lpsspec)
{
  return state_formulas::parse_state_formula_specification(text, lpsspec);
}

bool is_monotonous(const state_formula& f)
{
  return state_formulas::is_monotonous(f);
}

state_formula normalize(const state_formula& x)
{
  return state_formulas::normalize(x);
}

bool is_normalized(const state_formula& x)
{
  return state_formulas::is_normalized(x);
}

bool is_timed(const state_formula& x)
{
  return state_formulas::is_timed(x);
}

std::set<core::identifier_string> find_state_variable_names(const state_formula& x)
{
  return state_formulas::find_state_variable_names(x);
}

} // namespace algorithms

} // namespace state_formulas

} // namespace mcrl2

// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/tools/lps2pbes.h
/// \brief add your file description here.

#ifndef MCRL2_PBES_TOOLS_LPS2PBES_H
#define MCRL2_PBES_TOOLS_LPS2PBES_H

#include "mcrl2/lps/io.h"
#include "mcrl2/modal_formula/parse.h"
#include "mcrl2/pbes/io.h"
#include "mcrl2/pbes/lps2pbes.h"

namespace mcrl2 {

namespace pbes_system {

namespace detail
{
/// \brief Prints a warning if formula contains an action that is not used in lpsspec.
inline void check_lps2pbes_actions(const state_formulas::state_formula& formula, const lps::specification& lpsspec)
{
  std::set<process::action_label> used_lps_actions = lps::find_action_labels(lpsspec.process());
  std::set<process::action_label> used_state_formula_actions = state_formulas::find_action_labels(formula);
  std::set<process::action_label> diff = utilities::detail::set_difference(used_state_formula_actions, used_lps_actions);
  if (!diff.empty())
  {
    mCRL2log(log::warning) << "Warning: the modal formula contains an action "
                           << *diff.begin()
                           << " that does not appear in the LPS!" << std::endl;
  }
}

} // namespace detail

void lps2pbes(const std::string& input_filename,
              const std::string& output_filename,
              const utilities::file_format& output_format,
              const std::string& formula_filename,
              bool timed,
              bool structured,
              bool unoptimized,
              bool preprocess_modal_operators,
              bool generate_counter_example,
              bool check_only
             )
{
  if (formula_filename.empty())
  {
    throw mcrl2::runtime_error("option -f is not specified");
  }
  if (input_filename.empty())
  {
    mCRL2log(log::verbose) << "reading LPS from stdin..." << std::endl;
  }
  else
  {
    mCRL2log(log::verbose) << "reading LPS from file '" <<  input_filename << "'..." << std::endl;
  }
  lps::specification lpsspec;
  load_lps(lpsspec, input_filename);
  mCRL2log(log::verbose) << "reading input from file '" <<  formula_filename << "'..." << std::endl;
  std::ifstream from(formula_filename.c_str(), std::ifstream::in | std::ifstream::binary);
  if (!from)
  {
    throw mcrl2::runtime_error("cannot open state formula file: " + formula_filename);
  }
  std::string text = utilities::read_text(from);
  state_formulas::state_formula_specification formspec = state_formulas::algorithms::parse_state_formula_specification(text, lpsspec);
  detail::check_lps2pbes_actions(formspec.formula(), lpsspec);
  mCRL2log(log::verbose) << "converting state formula and LPS to a PBES..." << std::endl;
  pbes_system::pbes result = pbes_system::lps2pbes(lpsspec, formspec, timed, structured, unoptimized, preprocess_modal_operators, generate_counter_example, check_only);

  if (check_only)
  {
    mCRL2log(mcrl2::log::info)
      << "the file '" << formula_filename
      << "' contains a well-formed state formula" << std::endl;
    return;
  }

  if (output_filename.empty())
  {
    mCRL2log(log::verbose) << "writing PBES to stdout..." << std::endl;
  }
  else
  {
    mCRL2log(log::verbose) << "writing PBES to file '" <<  output_filename << "'..." << std::endl;
  }
  save_pbes(result, output_filename, output_format);
}

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_TOOLS_LPS2PBES_H

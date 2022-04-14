// Author(s): Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/lts/detail/lts_generation_options.h
/// \brief Options used during state space generation.

#ifndef MCRL2_LTS_DETAIL_LTS_GENERATION_OPTIONS_H
#define MCRL2_LTS_DETAIL_LTS_GENERATION_OPTIONS_H

#include <climits>
#include "mcrl2/lts/lts_io.h"
#include "mcrl2/lps/exploration_strategy.h"

namespace mcrl2
{
namespace lts
{


class lts_generation_options
{
  private:
    static const std::size_t default_max_states=ULONG_MAX;
    static const std::size_t default_bithashsize=209715200ULL; // ~25 MB
    static const std::size_t default_init_tsize=10000UL;

  public:
    static const std::size_t default_max_traces=ULONG_MAX;

    mcrl2::lps::stochastic_specification specification;
    bool usedummies;
    bool removeunused;

    mcrl2::data::rewriter::strategy strat;
    lps::exploration_strategy expl_strat;
    std::string priority_action;
    std::size_t todo_max;
    std::size_t max_states;
    std::size_t initial_table_size;
    bool suppress_progress_messages;

    bool bithashing;
    std::size_t bithashsize;

    mcrl2::lts::lts_type outformat;
    bool outinfo;
    std::string lts;

    bool trace;
    std::size_t max_traces;
    std::string trace_prefix;
    bool save_error_trace;
    bool detect_deadlock;
    bool detect_nondeterminism;
    bool detect_divergence;
    bool detect_action;
    std::set < mcrl2::core::identifier_string > trace_actions;
    std::set < std::string > trace_multiaction_strings;
    std::set < mcrl2::lps::multi_action > trace_multiactions;

    bool use_enumeration_caching;
    bool use_summand_pruning;
    std::set< mcrl2::core::identifier_string > actions_internal_for_divergencies;

    /// \brief Constructor
    lts_generation_options() :
      usedummies(true),
      removeunused(true),
      strat(mcrl2::data::jitty),
      expl_strat(lps::es_breadth),
      todo_max((std::numeric_limits< std::size_t >::max)()),
      max_states(default_max_states),
      initial_table_size(default_init_tsize),
      suppress_progress_messages(false),
      bithashing(false),
      bithashsize(default_bithashsize),
      outformat(mcrl2::lts::lts_none),
      outinfo(true),
      trace(false),
      max_traces(default_max_traces),
      save_error_trace(false),
      detect_deadlock(false),
      detect_nondeterminism(false),
      detect_divergence(false),
      detect_action(false),
      use_enumeration_caching(false),
      use_summand_pruning(false)
    {}

    /// \brief Copy assignment operator.
    lts_generation_options& operator=(const lts_generation_options& )=default;

    void validate_actions()
    {
      for (const std::string& s: trace_multiaction_strings)
      {
        try
        {
          trace_multiactions.insert(mcrl2::lps::parse_multi_action(s, specification.action_labels(), specification.data()));
        }
        catch (mcrl2::runtime_error& e)
        {
          throw mcrl2::runtime_error(std::string("Multi-action ") + s + " does not exist: " + e.what());
        }
        mCRL2log(log::verbose) << "Checking for action \"" << s << "\"\n";
      }
      if (detect_action)
      {
        for (const mcrl2::core::identifier_string& ta: trace_actions)
        {
          bool found = (std::string(ta) == "tau");
          for(const process::action_label& al: specification.action_labels())
          {
            if (al.name() == ta)
            {
              found=true;
              break;
            }
          }
          if (!found)
          {
            throw mcrl2::runtime_error(std::string("Action label ") + core::pp(ta) + " is not declared.");
          }
          else
          {
            mCRL2log(log::verbose) << "Checking for action " << ta << "\n";
          }
        }
      }
      for (const mcrl2::core::identifier_string& ta: actions_internal_for_divergencies)
      {
        mcrl2::process::action_label_list::iterator it = specification.action_labels().begin();
        bool found = (std::string(ta) == "tau");
        while (!found && it != specification.action_labels().end())
        {
          found = (it++->name() == ta);
        }
        if (!found)
        {
          throw mcrl2::runtime_error(std::string("Action label ") + core::pp(ta) + " is not declared.");
        }
      }
    }

};

} // namespace lts
} // namespace mcrl2

#endif // MCRL2_LTS_DETAIL_LTS_GENERATION_OPTIONS_H

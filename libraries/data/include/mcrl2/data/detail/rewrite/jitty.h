// Author(s): Muck van Weerdenburg
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/detail/rewrite/jitty.h

#ifndef __REWR_JITTY_H
#define __REWR_JITTY_H

#include "mcrl2/data/detail/rewrite.h"
#include "mcrl2/data/data_specification.h"

namespace mcrl2
{
namespace data
{
namespace detail
{

class RewriterJitty: public Rewriter
{
  public:
    typedef Rewriter::substitution_type substitution_type;
    typedef Rewriter::internal_substitution_type internal_substitution_type;

    RewriterJitty(const data_specification& DataSpec, const used_data_equation_selector &);
    virtual ~RewriterJitty();

    rewrite_strategy getStrategy();

    data_expression rewrite(const data_expression &term, substitution_type &sigma);

    bool addRewriteRule(const data_equation &Rule);
    bool removeRewriteRule(const data_equation &Rule);

  private:
    size_t max_vars;
    bool need_rebuild;

    std::map< function_symbol, data_equation_list > jitty_eqns;
    std::vector < atermpp::aterm_list >  jitty_strat;
    size_t MAX_LEN; 
    data_expression rewrite_aux(const data_expression &term, internal_substitution_type &sigma);
    void build_strategies();

    data_expression rewrite_aux_function_symbol(
                      const function_symbol &op,
                      const data_expression &term,
                      internal_substitution_type &sigma);

    /* Auxiliary function to take care that the array jitty_strat is sufficiently large
       to access element i */
    void make_jitty_strat_sufficiently_larger(const size_t i);
    atermpp::aterm_list create_strategy(const data_equation_list& rules1);
    void rebuild_strategy();

};
}
}
}

#endif

// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/lps/builder.h
/// \brief add your file description here.

#ifndef MCRL2_LPS_BUILDER_H
#define MCRL2_LPS_BUILDER_H

#include "mcrl2/lps/stochastic_specification.h"
#include "mcrl2/process/builder.h"

namespace mcrl2
{

namespace lps
{

// Adds sort expression traversal to a builder
//--- start generated add_sort_expressions code ---//
template <template <class> class Builder, class Derived>
struct add_sort_expressions: public Builder<Derived>
{
  typedef Builder<Derived> super;
  using super::enter;
  using super::leave;
  using super::update;
  using super::apply;

  void update(lps::deadlock& x)
  {
    static_cast<Derived&>(*this).enter(x);
    if (x.has_time())
    {
      x.time() = static_cast<Derived&>(*this).apply(x.time());    
    }
    static_cast<Derived&>(*this).leave(x);
  }

  lps::multi_action apply(const lps::multi_action& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::multi_action result = x.has_time() ?
      lps::multi_action(static_cast<Derived&>(*this).apply(x.actions()), static_cast<Derived&>(*this).apply(x.time())) :
      lps::multi_action(static_cast<Derived&>(*this).apply(x.actions()), x.time());
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

  void update(lps::deadlock_summand& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.summation_variables() = static_cast<Derived&>(*this).apply(x.summation_variables());
    x.condition() = static_cast<Derived&>(*this).apply(x.condition());
    static_cast<Derived&>(*this).update(x.deadlock());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::action_summand& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.summation_variables() = static_cast<Derived&>(*this).apply(x.summation_variables());
    x.condition() = static_cast<Derived&>(*this).apply(x.condition());
    x.multi_action() = static_cast<Derived&>(*this).apply(x.multi_action());
    x.assignments() = static_cast<Derived&>(*this).apply(x.assignments());
    static_cast<Derived&>(*this).leave(x);
  }

  lps::process_initializer apply(const lps::process_initializer& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::process_initializer result = lps::process_initializer(static_cast<Derived&>(*this).apply(x.expressions()));
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

  void update(lps::linear_process& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.process_parameters() = static_cast<Derived&>(*this).apply(x.process_parameters());
    static_cast<Derived&>(*this).update(x.deadlock_summands());
    static_cast<Derived&>(*this).update(x.action_summands());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::specification& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.action_labels() = static_cast<Derived&>(*this).apply(x.action_labels());
    static_cast<Derived&>(*this).update(x.global_variables());
    static_cast<Derived&>(*this).update(x.process());
    x.initial_process() = static_cast<Derived&>(*this).apply(x.initial_process());
    static_cast<Derived&>(*this).leave(x);
  }

  lps::stochastic_distribution apply(const lps::stochastic_distribution& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::stochastic_distribution result = x; if (x.is_defined()) { result = lps::stochastic_distribution(static_cast<Derived&>(*this).apply(x.variables()), static_cast<Derived&>(*this).apply(x.distribution())); }
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

  void update(lps::stochastic_action_summand& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.summation_variables() = static_cast<Derived&>(*this).apply(x.summation_variables());
    x.condition() = static_cast<Derived&>(*this).apply(x.condition());
    x.multi_action() = static_cast<Derived&>(*this).apply(x.multi_action());
    x.assignments() = static_cast<Derived&>(*this).apply(x.assignments());
    x.distribution() = static_cast<Derived&>(*this).apply(x.distribution());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::stochastic_linear_process& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.process_parameters() = static_cast<Derived&>(*this).apply(x.process_parameters());
    static_cast<Derived&>(*this).update(x.deadlock_summands());
    static_cast<Derived&>(*this).update(x.action_summands());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::stochastic_specification& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.action_labels() = static_cast<Derived&>(*this).apply(x.action_labels());
    static_cast<Derived&>(*this).update(x.global_variables());
    static_cast<Derived&>(*this).update(x.process());
    x.initial_process() = static_cast<Derived&>(*this).apply(x.initial_process());
    static_cast<Derived&>(*this).leave(x);
  }

  lps::stochastic_process_initializer apply(const lps::stochastic_process_initializer& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::stochastic_process_initializer result = lps::stochastic_process_initializer(static_cast<Derived&>(*this).apply(x.expressions()), static_cast<Derived&>(*this).apply(x.distribution()));
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

};

/// \brief Builder class
template <typename Derived>
struct sort_expression_builder: public add_sort_expressions<process::sort_expression_builder, Derived>
{
};
//--- end generated add_sort_expressions code ---//

// Adds data expression traversal to a builder
//--- start generated add_data_expressions code ---//
template <template <class> class Builder, class Derived>
struct add_data_expressions: public Builder<Derived>
{
  typedef Builder<Derived> super;
  using super::enter;
  using super::leave;
  using super::update;
  using super::apply;

  void update(lps::deadlock& x)
  {
    static_cast<Derived&>(*this).enter(x);
    if (x.has_time())
    {
      x.time() = static_cast<Derived&>(*this).apply(x.time());    
    }
    static_cast<Derived&>(*this).leave(x);
  }

  lps::multi_action apply(const lps::multi_action& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::multi_action result = x.has_time() ?
      lps::multi_action(static_cast<Derived&>(*this).apply(x.actions()), static_cast<Derived&>(*this).apply(x.time())) :
      lps::multi_action(static_cast<Derived&>(*this).apply(x.actions()), x.time());
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

  void update(lps::deadlock_summand& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.condition() = static_cast<Derived&>(*this).apply(x.condition());
    static_cast<Derived&>(*this).update(x.deadlock());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::action_summand& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.condition() = static_cast<Derived&>(*this).apply(x.condition());
    x.multi_action() = static_cast<Derived&>(*this).apply(x.multi_action());
    x.assignments() = static_cast<Derived&>(*this).apply(x.assignments());
    static_cast<Derived&>(*this).leave(x);
  }

  lps::process_initializer apply(const lps::process_initializer& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::process_initializer result = lps::process_initializer(static_cast<Derived&>(*this).apply(x.expressions()));
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

  void update(lps::linear_process& x)
  {
    static_cast<Derived&>(*this).enter(x);
    static_cast<Derived&>(*this).update(x.deadlock_summands());
    static_cast<Derived&>(*this).update(x.action_summands());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::specification& x)
  {
    static_cast<Derived&>(*this).enter(x);
    static_cast<Derived&>(*this).update(x.process());
    x.initial_process() = static_cast<Derived&>(*this).apply(x.initial_process());
    static_cast<Derived&>(*this).leave(x);
  }

  lps::stochastic_distribution apply(const lps::stochastic_distribution& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::stochastic_distribution result = x; if (x.is_defined()) { result = lps::stochastic_distribution(x.variables(), static_cast<Derived&>(*this).apply(x.distribution())); }
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

  void update(lps::stochastic_action_summand& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.condition() = static_cast<Derived&>(*this).apply(x.condition());
    x.multi_action() = static_cast<Derived&>(*this).apply(x.multi_action());
    x.assignments() = static_cast<Derived&>(*this).apply(x.assignments());
    x.distribution() = static_cast<Derived&>(*this).apply(x.distribution());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::stochastic_linear_process& x)
  {
    static_cast<Derived&>(*this).enter(x);
    static_cast<Derived&>(*this).update(x.deadlock_summands());
    static_cast<Derived&>(*this).update(x.action_summands());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::stochastic_specification& x)
  {
    static_cast<Derived&>(*this).enter(x);
    static_cast<Derived&>(*this).update(x.process());
    x.initial_process() = static_cast<Derived&>(*this).apply(x.initial_process());
    static_cast<Derived&>(*this).leave(x);
  }

  lps::stochastic_process_initializer apply(const lps::stochastic_process_initializer& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::stochastic_process_initializer result = lps::stochastic_process_initializer(static_cast<Derived&>(*this).apply(x.expressions()), static_cast<Derived&>(*this).apply(x.distribution()));
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

};

/// \brief Builder class
template <typename Derived>
struct data_expression_builder: public add_data_expressions<process::data_expression_builder, Derived>
{
};
//--- end generated add_data_expressions code ---//

//--- start generated add_variables code ---//
template <template <class> class Builder, class Derived>
struct add_variables: public Builder<Derived>
{
  typedef Builder<Derived> super;
  using super::enter;
  using super::leave;
  using super::update;
  using super::apply;

  void update(lps::deadlock& x)
  {
    static_cast<Derived&>(*this).enter(x);
    if (x.has_time())
    {
      x.time() = static_cast<Derived&>(*this).apply(x.time());    
    }
    static_cast<Derived&>(*this).leave(x);
  }

  lps::multi_action apply(const lps::multi_action& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::multi_action result = x.has_time() ?
      lps::multi_action(static_cast<Derived&>(*this).apply(x.actions()), static_cast<Derived&>(*this).apply(x.time())) :
      lps::multi_action(static_cast<Derived&>(*this).apply(x.actions()), x.time());
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

  void update(lps::deadlock_summand& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.summation_variables() = static_cast<Derived&>(*this).apply(x.summation_variables());
    x.condition() = static_cast<Derived&>(*this).apply(x.condition());
    static_cast<Derived&>(*this).update(x.deadlock());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::action_summand& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.summation_variables() = static_cast<Derived&>(*this).apply(x.summation_variables());
    x.condition() = static_cast<Derived&>(*this).apply(x.condition());
    x.multi_action() = static_cast<Derived&>(*this).apply(x.multi_action());
    x.assignments() = static_cast<Derived&>(*this).apply(x.assignments());
    static_cast<Derived&>(*this).leave(x);
  }

  lps::process_initializer apply(const lps::process_initializer& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::process_initializer result = lps::process_initializer(static_cast<Derived&>(*this).apply(x.expressions()));
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

  void update(lps::linear_process& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.process_parameters() = static_cast<Derived&>(*this).apply(x.process_parameters());
    static_cast<Derived&>(*this).update(x.deadlock_summands());
    static_cast<Derived&>(*this).update(x.action_summands());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::specification& x)
  {
    static_cast<Derived&>(*this).enter(x);
    static_cast<Derived&>(*this).update(x.global_variables());
    static_cast<Derived&>(*this).update(x.process());
    x.initial_process() = static_cast<Derived&>(*this).apply(x.initial_process());
    static_cast<Derived&>(*this).leave(x);
  }

  lps::stochastic_distribution apply(const lps::stochastic_distribution& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::stochastic_distribution result = x; if (x.is_defined()) { result = lps::stochastic_distribution(static_cast<Derived&>(*this).apply(x.variables()), static_cast<Derived&>(*this).apply(x.distribution())); }
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

  void update(lps::stochastic_action_summand& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.summation_variables() = static_cast<Derived&>(*this).apply(x.summation_variables());
    x.condition() = static_cast<Derived&>(*this).apply(x.condition());
    x.multi_action() = static_cast<Derived&>(*this).apply(x.multi_action());
    x.assignments() = static_cast<Derived&>(*this).apply(x.assignments());
    x.distribution() = static_cast<Derived&>(*this).apply(x.distribution());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::stochastic_linear_process& x)
  {
    static_cast<Derived&>(*this).enter(x);
    x.process_parameters() = static_cast<Derived&>(*this).apply(x.process_parameters());
    static_cast<Derived&>(*this).update(x.deadlock_summands());
    static_cast<Derived&>(*this).update(x.action_summands());
    static_cast<Derived&>(*this).leave(x);
  }

  void update(lps::stochastic_specification& x)
  {
    static_cast<Derived&>(*this).enter(x);
    static_cast<Derived&>(*this).update(x.global_variables());
    static_cast<Derived&>(*this).update(x.process());
    x.initial_process() = static_cast<Derived&>(*this).apply(x.initial_process());
    static_cast<Derived&>(*this).leave(x);
  }

  lps::stochastic_process_initializer apply(const lps::stochastic_process_initializer& x)
  {
    static_cast<Derived&>(*this).enter(x);
    lps::stochastic_process_initializer result = lps::stochastic_process_initializer(static_cast<Derived&>(*this).apply(x.expressions()), static_cast<Derived&>(*this).apply(x.distribution()));
    static_cast<Derived&>(*this).leave(x);
    return result;
  }

};

/// \brief Builder class
template <typename Derived>
struct variable_builder: public add_variables<process::data_expression_builder, Derived>
{
};
//--- end generated add_variables code ---//

} // namespace lps

} // namespace mcrl2

#endif // MCRL2_LPS_BUILDER_H

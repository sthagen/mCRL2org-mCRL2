// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/remove_parameters.h
/// \brief Functions for removing insignificant parameters from pbes types.

#ifndef MCRL2_PBES_REMOVE_PARAMETERS_H
#define MCRL2_PBES_REMOVE_PARAMETERS_H

#include "mcrl2/pbes/builder.h"

namespace mcrl2
{

namespace pbes_system
{

/// \cond INTERNAL_DOCS
namespace detail
{

/// \brief Removes elements with indices in a given sequence from the sequence l
/// \param l A sequence of terms
/// \param to_be_removed A sorted sequence of integers
/// \return The removal result
template <typename Term>
atermpp::term_list<Term> remove_elements(const atermpp::term_list<Term>& l, const std::vector<std::size_t>& to_be_removed)
{
  assert(std::is_sorted(to_be_removed.begin(), to_be_removed.end()));
  std::size_t index = 0;
  std::vector<Term> result;
  auto j = to_be_removed.begin();
  for (auto i = l.begin(); i != l.end(); ++i, ++index)
  {
    if (j != to_be_removed.end() && index == *j)
    {
      ++j;
    }
    else
    {
      result.push_back(*i);
    }
  }
  return atermpp::term_list<Term>(result.begin(),result.end());
}

template <typename Derived>
struct remove_parameters_builder: public pbes_system::pbes_expression_builder<Derived>
{
  typedef pbes_system::pbes_expression_builder<Derived> super;
  using super::enter;
  using super::leave;
  using super::update;
  using super::apply;

  const std::vector<std::size_t>& to_be_removed;

  remove_parameters_builder(const std::vector<std::size_t>& to_be_removed_)
    : to_be_removed(to_be_removed_)
  {}

  propositional_variable apply(const propositional_variable& x)
  {
    return propositional_variable(x.name(), detail::remove_elements(x.parameters(), to_be_removed));
  }

  propositional_variable_instantiation apply(const propositional_variable_instantiation& x)
  {
    return propositional_variable_instantiation(x.name(), detail::remove_elements(x.parameters(), to_be_removed));
  }

  void update(pbes_equation& x)
  {
    x.variable() = static_cast<Derived&>(*this).apply(x.variable());
    x.formula() = static_cast<Derived&>(*this).apply(x.formula());
  }

  void update(pbes& x)
  {
    static_cast<Derived&>(*this).update(x.equations());
    x.initial_state() = static_cast<Derived&>(*this).apply(x.initial_state());
    static_cast<Derived&>(*this).update(x.global_variables());
  }
};


} // namespace detail
/// \endcond

/// \brief Removes parameters from propositional variable instantiations in a pbes expression
/// \param x A PBES library object that derives from atermpp::aterm_appl
/// \param to_be_removed The indices of parameters that are to be removed
/// \return The expression \p x with parameters removed according to the mapping \p to_be_removed
template <typename T>
T remove_parameters(const T& x,
                    const std::vector<std::size_t>& to_be_removed,
                    typename std::enable_if< std::is_base_of< atermpp::aterm, T >::value>::type* = nullptr
                   )
{
  return core::make_apply_builder_arg1<detail::remove_parameters_builder>(to_be_removed).apply(x);
}

/// \brief Removes parameters from propositional variable instantiations in a pbes expression
/// \param x A PBES library object that does not derive from atermpp::aterm_appl
/// \param to_be_removed The indices of parameters that are to be removed
/// \return The expression \p x with parameters removed according to the mapping \p to_be_removed
template <typename T>
void remove_parameters(T& x,
                       const std::vector<std::size_t>& to_be_removed,
                       typename std::enable_if< !std::is_base_of< atermpp::aterm, T >::value>::type* = 0
                      )
{
  core::make_apply_builder_arg1<detail::remove_parameters_builder>(to_be_removed).update(x);
}

/// \cond INTERNAL_DOCS
namespace detail
{

template <typename Derived>
struct map_based_remove_parameters_builder: public pbes_expression_builder<Derived>
{
  typedef pbes_expression_builder<Derived> super;
  using super::enter;
  using super::leave;
  using super::update;
  using super::apply;

  const std::map<core::identifier_string, std::vector<std::size_t> >& to_be_removed;

  map_based_remove_parameters_builder(const std::map<core::identifier_string, std::vector<std::size_t> >& to_be_removed_)
    : to_be_removed(to_be_removed_)
  {}

  // to prevent default operator() being called
  data::data_expression apply(const data::data_expression& x)
  {
  	return x;
  }

  propositional_variable apply(const propositional_variable& x)
  {
    auto i = to_be_removed.find(x.name());
    if (i == to_be_removed.end())
    {
      return x;
    }
    return remove_parameters(x, i->second);
  }

  propositional_variable_instantiation apply(const propositional_variable_instantiation& x)
  {
    auto i = to_be_removed.find(x.name());
    if (i == to_be_removed.end())
    {
      return x;
    }
    else
    {
      return remove_parameters(x, i->second);
    }
  }

  void update(pbes_equation& x)
  {
    x.variable() = static_cast<Derived&>(*this).apply(x.variable());
    x.formula() = static_cast<Derived&>(*this).apply(x.formula());
  }

  void update(pbes& x)
  {
    static_cast<Derived&>(*this).update(x.equations());
    x.initial_state() = static_cast<Derived&>(*this).apply(x.initial_state());
  }
};
} // namespace detail
/// \endcond

/// \brief Removes parameters from propositional variable instantiations in a pbes expression
/// \param x A PBES library object that derives from atermpp::aterm_appl
/// \param to_be_removed A mapping that maps propositional variable names to indices of parameters that are removed
/// \return The expression \p x with parameters removed according to the mapping \p to_be_removed
template <typename T>
T remove_parameters(const T& x,
                    const std::map<core::identifier_string, std::vector<std::size_t> >& to_be_removed,
                    typename std::enable_if< std::is_base_of< atermpp::aterm, T >::value>::type* = nullptr
                   )
{
  return core::make_apply_builder_arg1<detail::map_based_remove_parameters_builder>(to_be_removed).apply(x);
}

/// \brief Removes parameters from propositional variable instantiations in a pbes expression
/// \param x A PBES library object that does not derive from atermpp::aterm_appl
/// \param to_be_removed A mapping that maps propositional variable names to a sorted vector of parameter indices that
/// need to be removed
/// \return The expression \p x with parameters removed according to the mapping \p to_be_removed
template <typename T>
void remove_parameters(T& x,
                       const std::map<core::identifier_string, std::vector<std::size_t> >& to_be_removed,
                       typename std::enable_if< !std::is_base_of< atermpp::aterm, T >::value>::type* = nullptr
                      )
{
  core::make_apply_builder_arg1<detail::map_based_remove_parameters_builder>(to_be_removed).update(x);
}

/// \cond INTERNAL_DOCS
namespace detail
{

template <typename Derived>
struct set_based_remove_parameters_builder: public pbes_expression_builder<Derived>
{
  typedef pbes_expression_builder<Derived> super;
  using super::enter;
  using super::leave;
  using super::update;
  using super::apply;

  const std::set<data::variable>& to_be_removed;

  set_based_remove_parameters_builder(const std::set<data::variable>& to_be_removed_)
    : to_be_removed(to_be_removed_)
  {}

  void remove_parameters(std::set<data::variable>& x) const
  {
    for (auto i = to_be_removed.begin(); i != to_be_removed.end(); ++i)
    {
      x.erase(*i);
    }
  }

  data::variable_list apply(const data::variable_list& l) const
  {
  	using utilities::detail::contains;

    std::vector<data::variable> result;
    for (const data::variable& v: l)
    {
      if (!contains(to_be_removed, v))
      {
        result.push_back(v);
      }
    }
    return data::variable_list(result.begin(), result.end());
  }

  data::assignment_list apply(const data::assignment_list& l) const
  {
    using utilities::detail::contains;
    std::vector<data::assignment> a(l.begin(), l.end());
    a.erase(std::remove_if(a.begin(), a.end(), [&](const data::assignment& y) {	return contains(to_be_removed, y.lhs()); }), a.end());
    return data::assignment_list(a.begin(), a.end());
  }

  propositional_variable apply(const propositional_variable& x)
  {
    return propositional_variable(x.name(), static_cast<Derived&>(*this).apply(x.parameters()));
  }

  void update(pbes_equation& x)
  {
    x.variable() = static_cast<Derived&>(*this).apply(x.variable());
    x.formula() = static_cast<Derived&>(*this).apply(x.formula());
  }

  void update(pbes& x)
  {
    static_cast<Derived&>(*this).update(x.equations());
    x.initial_state() = static_cast<Derived&>(*this).apply(x.initial_state());
    remove_parameters(x.global_variables());
  }
};
} // namespace detail
/// \endcond

/// \brief Removes parameters from propositional variable instantiations in a pbes expression
/// \param x A PBES library object that derives from atermpp::aterm_appl
/// \param to_be_removed A set of parameters that are to be removed
/// \return The expression \p x with parameters removed according to the mapping \p to_be_removed
template <typename T>
T remove_parameters(const T& x,
                    const std::set<data::variable>& to_be_removed,
                    typename std::enable_if< std::is_base_of< atermpp::aterm, T >::value>::type* = 0
                   )
{
  return core::make_apply_builder_arg1<detail::set_based_remove_parameters_builder>(to_be_removed).apply(x);
}

/// \brief Removes parameters from propositional variable instantiations in a pbes expression
/// \param x A PBES library object that does not derive from atermpp::aterm_appl
/// \param to_be_removed A set of parameters that are to be removed
/// \return The expression \p x with parameters removed according to the mapping \p to_be_removed
template <typename T>
void remove_parameters(T& x,
                       const std::set<data::variable>& to_be_removed,
                       typename std::enable_if< !std::is_base_of< atermpp::aterm, T >::value>::type* = nullptr
                      )
{
  core::make_apply_builder_arg1<detail::set_based_remove_parameters_builder>(to_be_removed).update(x);
}


/// \cond INTERNAL_DOCS
// used in pbes.h
inline
void remove_pbes_parameters(pbes& x,
                            const std::set<data::variable>& to_be_removed
                           )
{
  remove_parameters(x, to_be_removed);
}
/// \endcond

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_REMOVE_PARAMETERS_H

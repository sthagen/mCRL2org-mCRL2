// Author(s): Wieger Wesselink, Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/bes/detail/bes_property_map.h
/// \brief A property map containing properties of an LPS specification.

#ifndef MCRL2_BES_DETAIL_BES_PROPERTY_MAP_H
#define MCRL2_BES_DETAIL_BES_PROPERTY_MAP_H

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "mcrl2/data/detail/data_property_map.h"
#include "mcrl2/bes/find.h"
#include "mcrl2/bes/print.h"

namespace mcrl2
{

namespace bes
{

namespace detail
{

/// \brief Stores the following properties of a linear process specification:
/// <table>
/// <tr><th>property                    </th><th>description                             </th><th>format                               </th></tr>
/// <tr><td>equation_count              </td><td>The number of equation                  </td><td>NUMBER                               </td></tr>
/// <tr><td>mu_equation_count           </td><td>The number of mu equations              </td><td>NUMBER                               </td></tr>
/// <tr><td>nu_equation_count           </td><td>The number of nu equations              </td><td>NUMBER                               </td></tr>
/// <tr><td>block_nesting_depth         </td><td>The number of mu/nu alternations        </td><td>NUMBER                               </td></tr>
/// <tr><td>declared_free_variables     </td><td>The declared free variables             </td><td>PARAMLIST                            </td></tr>
/// <tr><td>declared_free_variable_names</td><td>The names of the declared free variables</td><td>NAME, ..., NAME                      </td></tr>
/// <tr><td>declared_variable_count     </td><td>The number of declared free variables   </td><td>NUMBER                               </td></tr>
/// <tr><td>used_free_variables         </td><td>The used free variables                 </td><td>PARAMLIST                            </td></tr>
/// <tr><td>used_free_variables_names   </td><td>The names of the used free variables    </td><td>NAME, ..., NAME                      </td></tr>
/// <tr><td>used_free_variable_count    </td><td>The number of used free variables       </td><td>NUMBER                               </td></tr>
/// <tr><td>binding_variables           </td><td>The binding variables                   </td><td>NAME(PARAMLIST), ..., NAME(PARAMLIST)</td></tr>
/// <tr><td>binding_variable_names      </td><td>The names of the binding variables      </td><td>NAME, ..., NAME                      </td></tr>
/// <tr><td>occurring_variables         </td><td>The occurring variables                 </td><td>NUMBER                               </td></tr>
/// <tr><td>occurring_variable_names    </td><td>The names of the occurring variables    </td><td>NAME(PARAMLIST), ..., NAME(PARAMLIST)</td></tr>
/// </table>
/// where <tt>PARAMLIST</tt> is defined as <tt>NAME:SORT, ... ,NAME:SORT</tt>.
class bes_property_map : public mcrl2::data::detail::data_property_map< bes_property_map >
{
  protected:

    // Allow base class access to protected functions
    friend class data::detail::data_property_map< bes_property_map >;

    typedef data::detail::data_property_map< bes_property_map > super;

    using super::print;

    std::string print(const boolean_variable& v) const
    {
      return bes::pp(v);
    }

    // super class compare functions
    using super::compare;

    std::string compare_property(const std::string& property, const std::string& x, const std::string& y) const
    {
      if (property == "equation_count")
      {
        return compare(property, parse_unsigned_int(x), parse_unsigned_int(y));
      }
      else if (property == "mu_equation_count")
      {
        return compare(property, parse_unsigned_int(x), parse_unsigned_int(y));
      }
      else if (property == "nu_equation_count")
      {
        return compare(property, parse_unsigned_int(x), parse_unsigned_int(y));
      }
      else if (property == "block_nesting_depth")
      {
        return compare(property, parse_unsigned_int(x), parse_unsigned_int(y));
      }
      else if (property == "declared_free_variables")
      {
        return compare(property, parse_set_string(x), parse_set_string(y));
      }
      else if (property == "declared_free_variable_names")
      {
        return compare(property, parse_set_string(x), parse_set_string(y));
      }
      else if (property == "declared_variable_count")
      {
        return compare(property, parse_unsigned_int(x), parse_unsigned_int(y));
      }
      else if (property == "used_free_variables")
      {
        return compare(property, parse_set_string(x), parse_set_string(y));
      }
      else if (property == "used_free_variables_names")
      {
        return compare(property, parse_set_string(x), parse_set_string(y));
      }
      else if (property == "used_free_variable_count")
      {
        return compare(property, parse_unsigned_int(x), parse_unsigned_int(y));
      }
      else if (property == "binding_variables")
      {
        return compare(property, parse_set_string(x), parse_set_string(y));
      }
      else if (property == "binding_variable_names")
      {
        return compare(property, parse_set_string(x), parse_set_string(y));
      }
      else if (property == "occurring_variables")
      {
        return compare(property, parse_set_string(x), parse_set_string(y));
      }
      else if (property == "occurring_variable_names")
      {
        return compare(property, parse_set_string(x), parse_set_string(y));
      }
      return "ERROR: unknown property " + property + " encountered!";
    }

    //--------------------------------------------//
    // compute functions
    //--------------------------------------------//
    /// \brief Computes the number of mu and nu equations and returns them as a pair
    std::pair<unsigned int, unsigned int> compute_equation_counts(const boolean_equation_system& b) const
    {
      unsigned int mu_count = 0;
      unsigned int nu_count = 0;
      for (auto i = b.equations().begin(); i != b.equations().end(); ++i)
      {
        if (i->symbol().is_mu())
        {
          mu_count++;
        }
        else
        {
          nu_count++;
        }
      }
      return std::make_pair(mu_count, nu_count);
    }

    // number of changes from mu to nu or vice versa
    unsigned int compute_block_nesting_depth(const boolean_equation_system& b) const
    {
      unsigned int block_nesting_depth = 0;
      bool last_symbol_is_mu = false;

      for (auto i = b.equations().begin(); i != b.equations().end(); ++i)
      {
        if (i != b.equations().begin())
        {
          if (i->symbol().is_mu() != last_symbol_is_mu)
          {
            block_nesting_depth++;
          }
        }
        last_symbol_is_mu = i->symbol().is_mu();
      }
      return block_nesting_depth;
    }

  public:
    /// \brief Constructor.
    /// The strings may appear in a random order, and not all of them need to be present
    bes_property_map(const std::string& text)
      : super(text)
    {}

    /// \brief Constructor
    /// Initializes the bes_property_map with a boolean equation system
    bes_property_map(const boolean_equation_system& b)
    {
      std::pair<unsigned int, unsigned int>  equation_counts              = compute_equation_counts(b);
      unsigned int block_nesting_depth                                    = compute_block_nesting_depth(b);
      std::set<boolean_variable>             binding_variables            = b.binding_variables();
      std::set<boolean_variable>             occurring_variables          = b.occurring_variables();

      m_data["equation_count"              ] = print(equation_counts.first + equation_counts.second);
      m_data["mu_equation_count"           ] = print(equation_counts.first);
      m_data["nu_equation_count"           ] = print(equation_counts.second);
      m_data["block_nesting_depth"         ] = print(block_nesting_depth);
      m_data["binding_variables"           ] = print(binding_variables, false);
      m_data["binding_variable_names"      ] = print(names(binding_variables), false);
      m_data["occurring_variables"         ] = print(occurring_variables, false);
      m_data["occurring_variable_names"    ] = print(names(occurring_variables), false);
    }

    using super::to_string;
    using super::data;

    std::string compare(const bes_property_map& other) const
    {
      return super::compare(other);
    }
};

} // namespace detail

} // namespace boolean_equation_system

} // namespace mcrl2

#endif // MCRL2_BES_DETAIL_BES_PROPERTY_MAP_H

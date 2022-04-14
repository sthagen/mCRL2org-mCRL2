// Author(s): Rimco Boudewijns
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "mcrl2/data/rewriter.h"
#include "mcrl2/data/substitutions/mutable_map_substitution.h"
#include "rewriter.h"
#include "parsing.h"

Rewriter::Rewriter(QThread *atermThread, mcrl2::data::rewrite_strategy strategy):
  m_strategy(strategy)
{
  moveToThread(atermThread);
  m_parsed = false;
}

void Rewriter::rewrite(QString specification, QString dataExpression)
{
  if (m_specification != specification || !m_parsed)
  {
    m_parsed = false;
    m_specification = specification;
    try
    {
      mcrl2xi_qt::parseMcrl2Specification(m_specification.toStdString(), m_data_spec, m_vars);
      m_parsed = true;
    }
    catch (const mcrl2::runtime_error& e)
    {
      m_parseError = QString::fromStdString(e.what());
    }
  }

  if (m_parsed)
  {
    try
    {

      std::string stdDataExpression = dataExpression.toStdString();

      mCRL2log(info) << "Evaluate: \"" << stdDataExpression << "\"" << std::endl;
      mCRL2log(info) << "Parsing data expression: \"" << stdDataExpression << "\"" << std::endl;

      mcrl2::data::data_expression term = mcrl2::data::parse_data_expression(stdDataExpression, m_vars, m_data_spec);

      mCRL2log(info) << "Rewriting data expression: \"" << stdDataExpression << "\"" << std::endl;

      std::set<mcrl2::data::sort_expression> all_sorts=find_sort_expressions(term);
      m_data_spec.add_context_sorts(all_sorts);
      mcrl2::data::rewriter rewr(m_data_spec, m_strategy);
      mcrl2::data::mutable_map_substitution < std::map < mcrl2::data::variable, mcrl2::data::data_expression > > assignments;

      std::string result = mcrl2::data::pp(rewr(term,assignments));

      mCRL2log(info) << "Result: \"" << result << "\"" << std::endl;

      emit rewritten(QString::fromStdString(result));

    }
    catch (const mcrl2::runtime_error& e)
    {
      QString err = QString::fromStdString(e.what());
      emit(exprError(err));
    }
  }
  else
  {
    emit(parseError(m_parseError));
  }

  emit(finished());
}


// Author(s): Rimco Boudewijns
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

/**

  @file solver.h
  @author R. Boudewijns

  A solver object that moves itself to the aTerm Thread upon construction

*/

#ifndef MCRL2XI_SOLVER_H
#define MCRL2XI_SOLVER_H

#include <QObject>

#ifndef Q_MOC_RUN // Workaround for QTBUG-22829
#include "mcrl2/data/data_specification.h"
#include "mcrl2/data/rewrite_strategy.h"
#endif // Q_MOC_RUN

class Solver : public QObject
{
    Q_OBJECT
  public:
    /**
     * @brief Constructor
     */
    Solver(QThread *atermThread, mcrl2::data::rewrite_strategy strategy);

  signals:
    /**
     * @brief Signal to indicate the solving process produced new output
     * @param output The output of the solving process
     */
    void solvedPart(QString output);
    /**
     * @brief Signal to indicate that the algorithm finished
     */
    void finished();

    /**
     * @brief Signal to indicate that the parsing failed
     */
    void parseError(QString error);
    /**
     * @brief Signal to indicate an expression error
     */
    void exprError(QString error);
    
  public slots:
    /**
     * @brief Starts the solving process
     * @param specification The specification used during the solving process
     * @param dataExpression The expression that should be solved
     */
    void solve(QString specification, QString dataExpression);
    /**
     * @brief Aborts the currently running solving process
     */
    void abort();
    
  private:
    mcrl2::data::rewrite_strategy m_strategy;   ///< The currently used rewriter
    mcrl2::data::data_specification m_data_spec;        ///< The specification that was used last time
    std::set <mcrl2::data::variable > m_global_vars;    ///< The global variables of the last parsed specification
    bool m_parsed;                                      ///< Boolean indicating if the last specification was successfully parsed (used to cache the parsing step)
    QString m_parseError;                               ///< The last parse error message
    QString m_specification;                            ///< String containing the last specification that was parsed
    bool m_abort;                                       ///< Boolean indicating that the solving should be aborted as quickly as possible
};

#endif // MCRL2XI_SOLVER_H

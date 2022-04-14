// Author(s): Rimco Boudewijns
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

/**

  @file rewriter.h
  @author R. Boudewijns

  A rewriter object that moves itself to the aTerm Thread upon construction

*/

#ifndef MCRL2XI_REWRITER_H
#define MCRL2XI_REWRITER_H

#include <QObject>
#include <QThread>

#ifndef Q_MOC_RUN // Workaround for QTBUG-22829
#include "mcrl2/data/rewrite_strategy.h"
#include "mcrl2/data/data_specification.h"
#endif // Q_MOC_RUN

class Rewriter : public QObject
{
    Q_OBJECT
  public:
    /**
     * @brief Constructor
     */
    Rewriter(QThread *atermThread, mcrl2::data::rewrite_strategy strategy);

  signals:
    /**
     * @brief Signal to indicate the rewrite process is finished
     * @param output The output of the rewrite process
     */
    void rewritten(QString output);
    /**
     * @brief Signal to indicate that the algorithm finished
     */
    void finished();

    /**
     * @brief Signal to indicate that the parsing failed
     */
    void parseError(QString error);
    /**
     * @brief Signal to indicate an epression error
     */
    void exprError(QString error);
    
  public slots:
    /**
     * @brief Starts the rewriting process
     * @param specification The specification used during the rewrite process
     * @param dataExpression The expression that should be rewritten
     */
    void rewrite(QString specification, QString dataExpression);
    
  private:
    mcrl2::data::rewrite_strategy m_strategy;   ///< The currently used rewriter
    mcrl2::data::data_specification m_data_spec;        ///< The specification that was used last time
    std::set <mcrl2::data::variable > m_vars;           ///< The variables used in the last rewrite process
    bool m_parsed;                                      ///< Boolean indicating if the last specification was successfully parsed (used to cache the parsing step)
    QString m_parseError;                               ///< The last parse error message
    QString m_specification;                            ///< String containing the last specification that was parsed
};

#endif // MCRL2XI_REWRITER_H

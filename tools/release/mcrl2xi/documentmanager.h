// Author(s): Rimco Boudewijns
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

/**

  @file documentmanager.h
  @author R. Boudewijns

  Manager for multiple DocumentWidget elements

*/

#ifndef MCRL2XI_DOCUMENTMANAGER_H
#define MCRL2XI_DOCUMENTMANAGER_H

#include "mcrl2/gui/extendedtabwidget.h"

#include "documentwidget.h"

class DocumentManager : public mcrl2::gui::qt::ExtendedTabWidget
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor
     * @param parent The parent QWidget for the manager
     */
    DocumentManager(QWidget *parent = 0);

    void setAtermThread(QThread *atermThread) { m_atermThread = atermThread; }
    void setRewriteStrategy(mcrl2::data::rewrite_strategy strategy) { m_strategy = strategy; }

    /**
     * @brief Creates a new empty tab
     */
    void newFile();

    /**
     * @brief Opens the given file in a tab or
     *        focussed the tab containing the given file
     * @param fileName The file that should be opened
     */
    void openFile(QString fileName);

    /**
     * @brief Saves the document with @e index to the given @e fileName
     * @param index The index of the document to be saved
     * @param fileName The location where to save the document
     */
    void saveFile(int index, QString fileName);

    /**
     * @brief Returns the DocumentWidget for the given @e index
     * @param index The index of the DocumentWidget
     */
    DocumentWidget *getDocument(int index);

    /**
     * @brief Returns the DocumentWidget for the given @e fileName
     * @param fileName The filename of the DocumentWidget
     */
    DocumentWidget *findDocument(QString fileName);

    /**
     * @brief Closes the DocumentWidget for the given @e index
     * @param index The index of the DocumentWidget
     */
    void closeDocument(int index);

    /**
     * @brief Changes the title of the currect tab to reflect
     * whether the text of the document has changed
     */
    void updateTitle();

    /**
     * @brief Returns the DocumentWidget currently visible
     */
    DocumentWidget *currentDocument();
    /**
     * @brief Returns the filename of the currently visible DocumentWidget
     */
    QString currentFileName();

  signals:
    /**
     * @brief Activated if a new document/tab was created
     * @param document The newly created DocumentWidget
     */
    void documentCreated(DocumentWidget *document);
    /**
     * @brief Activated if a new document/tab was selected
     * @param document The newly selected DocumentWidget
     */
    void documentChanged(DocumentWidget *document);
    /**
     * @brief Activated if a document/tab is closing,
     *        the document is deleted immediately after this signal
     * @param document The closing DocumentWidget
     */
    void documentClosed(DocumentWidget *document);

  private:
    /**
     * @brief Creates a new DocumentWidget for this manager
     * @param title The title for the created tab
     */
    DocumentWidget *createDocument(QString title);

  private slots:
    /**
     * @brief Redirects the tab change, should be called if a tab was changed
     * @param index The index of the new visible tab
     */
    void onCurrentChanged(int index) { emit documentChanged(getDocument(index)); }

  protected:
    /**
     * @brief Overridden function used to guarantee there is at least 1 document at all times
     */
    void showEvent(QShowEvent *event);

  private:
    QThread *m_atermThread;
    mcrl2::data::rewrite_strategy m_strategy;
};

#endif // MCRL2XI_DOCUMENTMANAGER_H

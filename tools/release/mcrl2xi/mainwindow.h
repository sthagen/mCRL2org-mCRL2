// Author(s): Rimco Boudewijns
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

/**

  @file mainwindow.h
  @author R. Boudewijns

  Main Window of mCRL2xi used as GUI

*/

#ifndef MCRL2XI_MAINWINDOW_H
#define MCRL2XI_MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

#ifndef Q_MOC_RUN // Workaround for QTBUG-22829
#include "mcrl2/data/rewriter.h"
#endif // Q_MOC_RUN
#include "mcrl2/gui/persistentfiledialog.h"

#include "documentmanager.h"
#include "parser.h"
#include "findreplacedialog.h"

class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT
  public:
    /**
     * @brief Constructor
     * @param parent The parent QWidget for the window
     */
    MainWindow(QThread *atermThread, mcrl2::data::rewrite_strategy strategy);


    /**
     * @brief Saves the current document
     */
    bool saveDocument();
    /**
     * @brief Saves the document with the given index
     */
    bool saveDocument(int index);

    /**
     * @brief Opens the given file in the documentmanager
     * @param fileName The file that should be opened
     */
    void openDocument(QString fileName);


  public slots:
    /**
     * @brief Formats the given DocumentWidget
     * @param document The document to be formatted
     */
    void formatDocument(DocumentWidget *document);
    /**
     * @brief Updates the FindReplaceDialog to use the newly focussed document
     * @param document The newly focussed document
     */
    void changeDocument(DocumentWidget *document);
    /**
     * @brief Asks the user to save the document if needed
     * @param index The index for which the close was requested
     * @returns True if the document may be closed
     */
    bool onCloseRequest(int index);

    /**
     * @brief Updates the statusbar with the latest log output
     */
    void onLogOutput(QString level, QDateTime timestamp, QString message, QString formattedMessage);

    /**
     * @brief Removes all highlights if the text of a document changes
     */
    void textChanged();

  protected:
    /**
     * @brief Asks the user to save all changed files and saves window information
     */
    void closeEvent(QCloseEvent *event);

  private slots:
    // Slots for all menu items
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onExit();

    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onDelete();
    void onSelectAll();

    void onFind();
    void onZoomIn();
    void onZoomOut();
    void onResetZoom();
    void onWrapMode();
    void onResetPerspective();

    // Slots for the parser
    void onParse();

    void parseError(QString err);
    void parserFinished();

    // Slots for the rewriter
    void onRewrite();
    void onRewriteAbort();

    void rewritten(QString output);
    void rewriteError(QString err);
    void rewriterFinished();

    // Slots for the solver
    void onSolve();
    void onSolveAbort();

    void solvedPart(QString output);
    void solveError(QString err);
    void solverFinished();

  private:
    /**
     * @brief Tries to find a row and column number in an error message
     */
    void findErrorPosition(QString err);

    Ui::MainWindow m_ui;                      ///< The user interface generated by Qt
    Parser m_parser;                         ///< The parser that is used
    QByteArray m_state;                       ///< The window state that is used to restore the user interface
    FindReplaceDialog *m_findReplaceDialog;   ///< The Find and Replace dialog used
    QPalette m_palette;
    QPoint m_lastErrorPosition;               ///< The last error position that was found in the log
    int m_zoom_level = 0;

    mcrl2::gui::qt::PersistentFileDialog m_fileDialog;

};

#endif // MCRL2XI_MAINWINDOW_H

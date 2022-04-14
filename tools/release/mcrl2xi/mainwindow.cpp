// Author(s): Rimco Boudewijns
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <QMessageBox>
#include <QSettings>

#include "mainwindow.h"

MainWindow::MainWindow(QThread *atermThread, mcrl2::data::rewrite_strategy strategy) :
  m_parser(atermThread),
  m_palette(QApplication::palette()),
  m_fileDialog("", this)
{
  m_findReplaceDialog = new FindReplaceDialog(this);
  m_findReplaceDialog->setModal(false);

  m_ui.setupUi(this);
  m_ui.documentManager->setAtermThread(atermThread);
  m_ui.documentManager->setRewriteStrategy(strategy);

  QColor disabledColor = m_palette.brush(QPalette::Disabled, QPalette::Base).color();
  m_palette.setColor(QPalette::Base, disabledColor);

  m_ui.editRewriteOutput->setPalette(m_palette);
  m_ui.editSolveOutput->setPalette(m_palette);

  //All menu items
  connect(m_ui.actionNew, SIGNAL(triggered()), this, SLOT(onNew()));
  connect(m_ui.actionOpen, SIGNAL(triggered()), this, SLOT(onOpen()));
  connect(m_ui.actionSave, SIGNAL(triggered()), this, SLOT(onSave()));
  connect(m_ui.actionSave_As, SIGNAL(triggered()), this, SLOT(onSaveAs()));
  connect(m_ui.actionExit, SIGNAL(triggered()), this, SLOT(onExit()));

  connect(m_ui.actionUndo, SIGNAL(triggered()), this, SLOT(onUndo()));
  connect(m_ui.actionRedo, SIGNAL(triggered()), this, SLOT(onRedo()));
  connect(m_ui.actionCut, SIGNAL(triggered()), this, SLOT(onCut()));
  connect(m_ui.actionCopy, SIGNAL(triggered()), this, SLOT(onCopy()));
  connect(m_ui.actionPaste, SIGNAL(triggered()), this, SLOT(onPaste()));
  connect(m_ui.actionDelete, SIGNAL(triggered()), this, SLOT(onDelete()));
  connect(m_ui.actionSelect_All, SIGNAL(triggered()), this, SLOT(onSelectAll()));
  connect(m_ui.actionFind, SIGNAL(triggered()), this, SLOT(onFind()));
  connect(m_ui.actionZoom_in, SIGNAL(triggered()), this, SLOT(onZoomIn()));
  connect(m_ui.actionZoom_out, SIGNAL(triggered()), this, SLOT(onZoomOut()));
  connect(m_ui.actionReset_zoom, SIGNAL(triggered()), this, SLOT(onResetZoom()));

  connect(m_ui.actionWrap_mode, SIGNAL(triggered()), this, SLOT(onWrapMode()));
  connect(m_ui.actionReset_perspective, SIGNAL(triggered()), this, SLOT(onResetPerspective()));
  connect(m_ui.actionRewriter, SIGNAL(toggled(bool)), m_ui.dockRewriter, SLOT(setVisible(bool)));
  connect(m_ui.actionSolver, SIGNAL(toggled(bool)), m_ui.dockSolver, SLOT(setVisible(bool)));
  connect(m_ui.actionOutput, SIGNAL(toggled(bool)), m_ui.dockOutput, SLOT(setVisible(bool)));

  //All button functionality
  connect(m_ui.actionParse, SIGNAL(triggered()), this, SLOT(onParse()));
  connect(&m_parser, SIGNAL(parseError(QString)), this, SLOT(parseError(QString)));
  connect(&m_parser, SIGNAL(finished()), this, SLOT(parserFinished()));

  connect(m_ui.buttonRewrite, SIGNAL(clicked()), this, SLOT(onRewrite()));
  connect(m_ui.actionRewrite, SIGNAL(triggered()), this, SLOT(onRewrite()));
  connect(m_ui.buttonRewriteAbort, SIGNAL(clicked()), this, SLOT(onRewriteAbort()));

  connect(m_ui.buttonSolve, SIGNAL(clicked()), this, SLOT(onSolve()));
  connect(m_ui.actionSolve, SIGNAL(triggered()), this, SLOT(onSolve()));
  connect(m_ui.buttonSolveAbort, SIGNAL(clicked()), this, SLOT(onSolveAbort()));

  //Documentmanager events
  connect(m_ui.documentManager, SIGNAL(tabCloseRequested(int)), this, SLOT(onCloseRequest(int)));
  connect(m_ui.documentManager, SIGNAL(documentCreated(DocumentWidget*)), this, SLOT(formatDocument(DocumentWidget*)));
  connect(m_ui.documentManager, SIGNAL(documentChanged(DocumentWidget*)), this, SLOT(changeDocument(DocumentWidget*)));

  connect(m_ui.dockWidgetOutput, SIGNAL(logMessage(QString, QString, QDateTime, QString, QString)), this, SLOT(onLogOutput(QString, QString, QDateTime, QString, QString)));

  m_state = saveState();
  QSettings settings("mCRL2", "mCRL2xi");
  restoreGeometry(settings.value("geometry").toByteArray());
  restoreState(settings.value("windowState").toByteArray());

  m_ui.actionRewriter->setChecked(!m_ui.dockRewriter->isHidden());
  m_ui.actionSolver->setChecked(!m_ui.dockSolver->isHidden());
  m_ui.actionOutput->setChecked(!m_ui.dockOutput->isHidden());

}

bool MainWindow::saveDocument()
{
  return saveDocument(m_ui.documentManager->currentIndex());
}

bool MainWindow::saveDocument(int index)
{
  DocumentWidget* document = m_ui.documentManager->getDocument(index);
  QString fileName = document->getFileName();
  if (fileName.isNull()) {
    fileName = m_fileDialog.getSaveFileName(tr("Save file"),
                                            tr("mCRL2 specification (*.mcrl2 *.txt )"));
  }
  if (!fileName.isNull()) {
    m_ui.documentManager->saveFile(index, fileName);
    m_ui.statusBar->showMessage(QString("Saved %1.").arg(fileName), 5000);
    return true;
  }
  return false;
}

void MainWindow::openDocument(QString fileName)
{
  if (!fileName.isNull()) {
    m_ui.documentManager->openFile(fileName);
  }
}


void MainWindow::formatDocument(DocumentWidget *document)
{
  if (m_ui.actionWrap_mode->isChecked())
  {
    document->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
  }
  else
  {
    document->setWordWrapMode(QTextOption::NoWrap);
  }

  document->setFocus();

  QFont font;
  font.setFamily("Monospace");
  font.setFixedPitch(true);
  font.setWeight(QFont::Light);

  document->setFont(font);

  connect(document->rewriter(), SIGNAL(rewritten(QString)), this, SLOT(rewritten(QString)));
  connect(document->rewriter(), SIGNAL(parseError(QString)), this, SLOT(parseError(QString)));
  connect(document->rewriter(), SIGNAL(exprError(QString)), this, SLOT(rewriteError(QString)));
  connect(document->rewriter(), SIGNAL(finished()), this, SLOT(rewriterFinished()));

  connect(document->solver(), SIGNAL(solvedPart(QString)), this, SLOT(solvedPart(QString)));
  connect(document->solver(), SIGNAL(parseError(QString)), this, SLOT(parseError(QString)));
  connect(document->solver(), SIGNAL(exprError(QString)), this, SLOT(solveError(QString)));
  connect(document->solver(), SIGNAL(finished()), this, SLOT(solverFinished()));

  connect(document, SIGNAL(textChanged()), this, SLOT(textChanged()));
}

void MainWindow::changeDocument(DocumentWidget *document)
{
  if (document)
  {
    m_findReplaceDialog->setTextEdit(document);
  }
  else
  {  
    m_findReplaceDialog->setTextEdit(nullptr);
  }
}

bool MainWindow::onCloseRequest(int index)
{
  DocumentWidget *document = m_ui.documentManager->getDocument(index);

  if (!document->isModified()) 
  {
    m_findReplaceDialog->setTextEdit(nullptr);
    m_ui.documentManager->closeDocument(index);
    return true;
  }

  QMessageBox::StandardButton ret = QMessageBox::question ( this, tr("Specification modified"), tr("Do you want to save your modifications?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
  switch(ret)
  {
    case QMessageBox::Yes:
      if (saveDocument(index))
      {
        m_findReplaceDialog->setTextEdit(nullptr);
        m_ui.documentManager->closeDocument(index);
      }
      break;
    case QMessageBox::No:
      m_findReplaceDialog->setTextEdit(nullptr);
      m_ui.documentManager->closeDocument(index);
      break;
    case QMessageBox::Cancel:
      return false;
    default:
      // Should not occur, but leaving default out triggers
      // -Wswitch warnings.
      throw mcrl2::runtime_error("Unhandled answer in message box");
  }

  return true;
}

void MainWindow::onLogOutput(QString /*level*/, QString /*hint*/, QDateTime /*timestamp*/, QString message, QString /*formattedMessage*/)
{
  findErrorPosition(message);
  m_ui.statusBar->showMessage(message, 5000);
}

void MainWindow::textChanged()
{
  m_ui.documentManager->updateTitle();
}

void MainWindow::onNew()
{
  m_ui.documentManager->newFile();
}

void MainWindow::onOpen()
{
  QString fileName(m_fileDialog.getOpenFileName(tr("Open file"),
                                                tr("mCRL2 specification (*.mcrl2 *.txt )")));
  openDocument(fileName);
}

void MainWindow::onSave()
{
  saveDocument();
  m_ui.documentManager->updateTitle();
}

void MainWindow::onSaveAs()
{
  QString fileName(m_fileDialog.getSaveFileName(tr("Save file"),
                                                tr("mCRL2 specification (*.mcrl2 *.txt )")));
  if (!fileName.isNull()) {
    m_ui.documentManager->saveFile(m_ui.documentManager->currentIndex(), fileName);
    m_ui.statusBar->showMessage(QString("Saved %1.").arg(fileName), 5000);
  }
}

void MainWindow::onExit()
{
  close();
}

void MainWindow::onUndo()
{
  m_ui.documentManager->currentDocument()->undo();
}

void MainWindow::onRedo()
{
  m_ui.documentManager->currentDocument()->redo();
}

void MainWindow::onCut()
{
  m_ui.documentManager->currentDocument()->cut();
}

void MainWindow::onCopy()
{
  m_ui.documentManager->currentDocument()->copy();
}

void MainWindow::onPaste()
{
  m_ui.documentManager->currentDocument()->paste();
}

void MainWindow::onDelete()
{
  m_ui.documentManager->currentDocument()->textCursor().deleteChar();
}

void MainWindow::onSelectAll()
{
  m_ui.documentManager->currentDocument()->selectAll();
}

void MainWindow::onFind()
{
  m_findReplaceDialog->setTextEdit(m_ui.documentManager->currentDocument());
  // The two lines below guarantee that if the window was already open,
  // it will become the active window. 
  m_findReplaceDialog->raise();
  m_findReplaceDialog->activateWindow();
  m_findReplaceDialog->show();
}

void MainWindow::onZoomIn()
{
  m_zoom_level++;
  for(int i = 0; i < m_ui.documentManager->count(); ++i)
  {
    DocumentWidget *editor = m_ui.documentManager->getDocument(i);
    editor->zoomIn();
  }
}

void MainWindow::onZoomOut()
{
  m_zoom_level--;
  for(int i = 0; i < m_ui.documentManager->count(); ++i)
  {
    DocumentWidget *editor = m_ui.documentManager->getDocument(i);
    editor->zoomOut();
  }
}

void MainWindow::onResetZoom()
{
  for(int i = 0; i < m_ui.documentManager->count(); ++i)
  {
    DocumentWidget *editor = m_ui.documentManager->getDocument(i);
    if(m_zoom_level < 0)
    {
      editor->zoomIn(-m_zoom_level);
    }
    else if(m_zoom_level > 0)
    {
      editor->zoomOut(m_zoom_level);
    }
  }
  m_zoom_level = 0;
}

void MainWindow::onWrapMode()
{
  for (int i = 0; i < m_ui.documentManager->count(); i++)
  {
    if (m_ui.actionWrap_mode->isChecked())
    {
      m_ui.documentManager->getDocument(i)->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    }
    else
    {
      m_ui.documentManager->getDocument(i)->setWordWrapMode(QTextOption::NoWrap);
    }
  }
}

void MainWindow::onResetPerspective()
{
  restoreState(m_state);

  m_ui.actionRewriter->setChecked(!m_ui.dockRewriter->isHidden());
  m_ui.actionSolver->setChecked(!m_ui.dockSolver->isHidden());
  m_ui.actionOutput->setChecked(!m_ui.dockOutput->isHidden());
}


void MainWindow::onParse()
{
  m_ui.actionParse->setEnabled(false);
  DocumentWidget *document = m_ui.documentManager->currentDocument();
  QMetaObject::invokeMethod(&m_parser, "parse", Qt::QueuedConnection, Q_ARG(QString, document->toPlainText()));
}

void MainWindow::parseError(QString err)
{
  mCRL2log(error) << err.toStdString() << std::endl;

  int line = m_lastErrorPosition.x();
  int col = m_lastErrorPosition.y();
  QTextEdit::ExtraSelection highlight;

  DocumentWidget *editor = m_ui.documentManager->currentDocument();

  QTextBlock block = editor->document()->findBlockByNumber(line-1);
  if (block.isValid() && block.position()+col <= editor->toPlainText().length())
  {
    QTextCursor cursor = editor->textCursor();
    cursor.setPosition(block.position()+col);

    editor->setTextCursor(cursor);

    highlight.cursor = cursor;
    highlight.format.setProperty(QTextFormat::FullWidthSelection, true);
    highlight.format.setBackground(QColor("orange"));

    QList<QTextEdit::ExtraSelection> extras;
    extras << highlight;
    editor->setExtraSelections(extras);
  }
}

void MainWindow::parserFinished()
{
  m_ui.actionParse->setEnabled(true);
}

void MainWindow::onRewrite()
{
  // Save the document first as the rewriter may run out of its stack. 
  if (m_ui.documentManager->currentDocument()->isModified())
  { 
    for(int i = 0; i < m_ui.documentManager->count(); i++)
    {
      saveDocument(i);
    }
  }
  m_ui.buttonRewrite->setEnabled(false);
  m_ui.buttonRewriteAbort->setEnabled(true);
  m_ui.editRewriteOutput->clear();
  DocumentWidget *document = m_ui.documentManager->currentDocument();
  QMetaObject::invokeMethod(document->rewriter(), "rewrite", Qt::QueuedConnection, Q_ARG(QString, document->toPlainText()), Q_ARG(QString, m_ui.editRewriteExpr->text()));
}

void MainWindow::onRewriteAbort()
{
  DocumentWidget *document = m_ui.documentManager->currentDocument();
  QMetaObject::invokeMethod(document->rewriter(), "abort", Qt::QueuedConnection);

  m_ui.buttonRewriteAbort->setEnabled(false);
}

void MainWindow::rewriteError(QString err)
{
  mCRL2log(error) << err.toStdString() << std::endl;
  m_ui.editRewriteExpr->selectAll();
}

void MainWindow::rewritten(QString output)
{
  m_ui.editRewriteOutput->setPlainText(output);
}

void MainWindow::rewriterFinished()
{
  m_ui.buttonRewriteAbort->setEnabled(false);
  m_ui.buttonRewrite->setEnabled(true);
}

void MainWindow::onSolve()
{
  m_ui.buttonSolve->setEnabled(false);
  m_ui.buttonSolveAbort->setEnabled(true);
  m_ui.editSolveOutput->clear();
  DocumentWidget *document = m_ui.documentManager->currentDocument();
  QMetaObject::invokeMethod(document->solver(), "solve", Qt::QueuedConnection, Q_ARG(QString, document->toPlainText()), Q_ARG(QString, m_ui.editSolveExpr->text()));
}

void MainWindow::onSolveAbort()
{
  DocumentWidget *document = m_ui.documentManager->currentDocument();
  QMetaObject::invokeMethod(document->solver(), "abort", Qt::QueuedConnection);

  m_ui.buttonSolveAbort->setEnabled(false);
}

void MainWindow::solvedPart(QString output)
{
  m_ui.editSolveOutput->appendPlainText(output);
}

void MainWindow::solveError(QString err)
{
  mCRL2log(error) << err.toStdString() << std::endl;
  m_ui.editSolveExpr->selectAll();
}

void MainWindow::solverFinished()
{
  m_ui.buttonSolveAbort->setEnabled(false);
  m_ui.buttonSolve->setEnabled(true);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
  m_findReplaceDialog->close();
  for (int i = 0; i < m_ui.documentManager->count(); i++)
  {
    if (!onCloseRequest(i))
    {
      event->ignore();
      return;
    }
  }
  QSettings settings("mCRL2", "mCRL2xi");
  settings.setValue("geometry", saveGeometry());
  settings.setValue("windowState", saveState());
  event->accept();
}

void MainWindow::findErrorPosition(QString err)
{
  QStringList lines = err.split("\n");
  for (int i = 0; i < lines.size(); i++)
  {
    QRegExp rxlen("Line (\\d+), column (\\d+): syntax error");
    int pos = rxlen.indexIn(lines[i]);
    if (pos > -1) {
      m_lastErrorPosition.setX(rxlen.cap(1).toInt());
      m_lastErrorPosition.setY(rxlen.cap(2).toInt());
    }
  }
}

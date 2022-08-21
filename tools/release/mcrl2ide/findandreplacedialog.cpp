// Author(s): Olav Bunte
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "findandreplacedialog.h"
#include "ui_findandreplacedialog.h"

FindAndReplaceDialog::FindAndReplaceDialog(
    mcrl2::gui::qt::CodeEditor* codeEditor, QWidget* parent)
    : QDialog(parent), ui(new Ui::FindAndReplaceDialog), codeEditor(codeEditor)
{
  ui->setupUi(this);

  setWindowFlags(Qt::Dialog);

  connect(ui->textToFind, SIGNAL(textChanged(QString)), this,
          SLOT(setFindEnabled()));
  connect(codeEditor, SIGNAL(selectionChanged()), this,
          SLOT(setReplaceEnabled()));

  connect(ui->findButton, SIGNAL(clicked()), this, SLOT(actionFind()));
  connect(ui->replaceButton, SIGNAL(clicked()), this, SLOT(actionReplace()));
  connect(ui->replaceAllButton, SIGNAL(clicked()), this,
          SLOT(actionReplaceAll()));
  connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void FindAndReplaceDialog::showMessage(const QString& message, bool error)
{
  if (error)
  {
    ui->infoLabel->setStyleSheet("color:red");
  }
  else
  {
    ui->infoLabel->setStyleSheet("color:green");
  }
  ui->infoLabel->setText(message);
}

void FindAndReplaceDialog::resetFocus()
{
  ui->textToFind->setFocus();
  QString selection = codeEditor->textCursor().selectedText();
  if (!selection.isEmpty())
  {
    ui->textToFind->setText(selection);
  }

  if (isVisible())
  {
    setFocus();
    ui->textToFind->setFocus();
    ui->textToFind->selectAll();
    activateWindow();
    raise(); // for MacOS
  }
  else
  {
    show();
  }
}

void FindAndReplaceDialog::setFindEnabled()
{
  ui->findButton->setEnabled(ui->textToFind->text().count() > 0);
}

void FindAndReplaceDialog::setReplaceEnabled()
{
  Qt::CaseSensitivity flag =
      ui->caseCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
  ui->replaceButton->setEnabled(
      QString::compare(codeEditor->textCursor().selectedText(),
                       ui->textToFind->text(), flag) == 0);
}

void FindAndReplaceDialog::actionFind(bool forReplaceAll)
{
  bool back = !forReplaceAll && ui->upRadioButton->isChecked();
  QString toSearch = ui->textToFind->text();
  bool result = false;
  QTextCursor originalPosition = codeEditor->textCursor();

  QTextDocument::FindFlags flags;

  if (back)
  {
    flags |= QTextDocument::FindBackward;
  }
  if (ui->caseCheckBox->isChecked())
  {
    flags |= QTextDocument::FindCaseSensitively;
  }
  if (ui->wholeCheckBox->isChecked())
  {
    flags |= QTextDocument::FindWholeWords;
  }

  result = codeEditor->find(toSearch, flags);

  /* if found, we are done */
  if (result)
  {
    showMessage("");
    return;
  }

  if (!forReplaceAll)
  {
    /* if the string was not found, try to wrap around begin/end of file */
    codeEditor->moveCursor(back ? QTextCursor::End : QTextCursor::Start);
    result = codeEditor->find(toSearch, flags);

    /* if found, we are done and tell the user that we have wrapped around */
    if (result)
    {
      if (back)
      {
        showMessage("Found the last occurrence");
      }
      else
      {
        showMessage("Found the first occurrence");
      }
      return;
    }
  }

  /* if the string was still not found, mention it and reset the cursor */
  showMessage("No match found", true);
  codeEditor->setTextCursor(originalPosition);
}

void FindAndReplaceDialog::findNext(bool down)
{
  if (!ui->textToFind->text().isEmpty())
  {
    if (!this->isVisible())
    {
      this->show();
    }
    if (down)
    {
      ui->downRadioButton->setChecked(true);
    }
    else
    {
      ui->upRadioButton->setChecked(true);
    }
    this->actionFind();
  }
}

void FindAndReplaceDialog::actionReplace()
{
  codeEditor->textCursor().insertText(ui->textToReplace->text());
  actionFind();
}

void FindAndReplaceDialog::actionReplaceAll()
{
  QTextCursor originalPosition = codeEditor->textCursor();
  originalPosition.beginEditBlock();

  codeEditor->moveCursor(QTextCursor::Start);
  actionFind(true);

  int i = 0;
  while (codeEditor->textCursor().hasSelection())
  {
    codeEditor->textCursor().insertText(ui->textToReplace->text());
    actionFind(true);
    i++;
  }

  originalPosition.endEditBlock();
  showMessage(tr("Replaced %1 occurrence(s)").arg(i));

  codeEditor->setTextCursor(originalPosition);
}

void FindAndReplaceDialog::keyPressEvent(QKeyEvent* event)
{
  /* Do find next or find previous if the corresponding keys are pressed */
  if (event->matches(QKeySequence::FindNext))
  {
    this->findNext(true);
  }
  else if (event->matches(QKeySequence::FindPrevious))
  {
    this->findNext(false);
  }
  else
  {
    QDialog::keyPressEvent(event);
  }
}

FindAndReplaceDialog::~FindAndReplaceDialog()
{
  delete ui;
}

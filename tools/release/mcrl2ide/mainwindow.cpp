// Author(s): Olav Bunte
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "mainwindow.h"
#include "mcrl2/utilities/platform.h"

#include <QApplication>
#include <QtGlobal>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QStandardItemModel>

MainWindow::MainWindow(const QString& inputFilePath, QWidget* parent)
    : QMainWindow(parent)
{
  specificationEditor = new mcrl2::gui::qt::CodeEditor(this);
  specificationEditor->setPlaceholderText("Type your mCRL2 specification here");
  specificationEditor->setPurpose(true);
  setCentralWidget(specificationEditor);

  settings = new QSettings("mCRL2", "mcrl2ide");

  fileSystem = new FileSystem(specificationEditor, settings, this);
  processSystem = new ProcessSystem(fileSystem);
  findAndReplaceDialog = new FindAndReplaceDialog(specificationEditor, this);
  addPropertyDialog = new AddEditPropertyDialog(true, processSystem, fileSystem,
                                                findAndReplaceDialog, this);
  toolOptionsDialog = new ToolOptionsDialog(this, fileSystem);

  setupMenuBar();
  setupToolbar();
  setupDocks();

  processSystem->setConsoleDock(consoleDock);

  /* change the UI whenever a new project is opened */
  connect(fileSystem, SIGNAL(newProjectOpened()), this,
          SLOT(onNewProjectOpened()));
  /* change the UI whenever the IDE enters specification only mode */
  connect(fileSystem, SIGNAL(enterSpecificationOnlyMode()), this,
          SLOT(onEnterSpecificationOnlyMode()));
  /* reset the propertiesdock when the specification changes */
  connect(specificationEditor->document(), SIGNAL(modificationChanged(bool)),
          propertiesDock, SLOT(resetAllPropertyWidgets()));
  /* make saving a project only enabled whenever there are changes */
  saveAction->setEnabled(false);
  connect(specificationEditor, SIGNAL(modificationChanged(bool)), saveAction,
          SLOT(setEnabled(bool)));
  /* update the edit menu whenever the focus changes */
  connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), this,
          SLOT(updateEditMenu(QWidget*, QWidget*)));

  /* change the tool buttons to start or abort a tool depending on whether
   *   processes are running */
  for (ProcessType processType : PROCESSTYPES)
  {
    connect(processSystem->getProcessThread(processType),
            SIGNAL(statusChanged(bool, ProcessType)), this,
            SLOT(changeToolButtons(bool, ProcessType)));
  }

  /* set the title of the main window */
  setWindowTitle("mCRL2 IDE - Unnamed project");
  if (settings->contains("geometry"))
  {
    restoreGeometry(settings->value("geometry").toByteArray());
  }
  if (settings->contains("windowstate"))
  {
    restoreState(settings->value("windowstate").toByteArray());
  }

  processSystem->testExecutableExistence();

  /* open a project if a project file is given */
  if (!inputFilePath.isEmpty())
  {
    actionOpenProject(inputFilePath);
  }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupMenuBar()
{
  /* Create the File menu */
  QMenu* fileMenu = menuBar()->addMenu("File");

  newProjectAction =
      fileMenu->addAction(QIcon(":/icons/new_project.png"), "New Project", QKeySequence::New, this,
                          SLOT(actionNewProject()));

  fileMenu->addSeparator();

  openProjectAction =
      fileMenu->addAction(QIcon(":/icons/open_project.png"), "Open Project", QKeySequence::Open,
                          this, SLOT(actionOpenProject()));

  fileMenu->addSeparator();

  saveAction = fileMenu->addAction(saveProjectIcon, saveProjectText, QKeySequence::Save, this,
                                   SLOT(actionSave()));

  saveAsAction =
      fileMenu->addAction(saveProjectAsText, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), this, SLOT(actionSaveAs()));

  fileMenu->addSeparator();

  openProjectFolderInExplorerAction =
      fileMenu->addAction("Open Project Folder in File Manager", this,
                          SLOT(actionOpenProjectFolderInExplorer()));
  openProjectFolderInExplorerAction->setEnabled(false);

  fileMenu->addSeparator();

  importPropertiesAction = fileMenu->addAction(
      "Import Properties", QKeySequence(Qt::ALT | Qt::Key_I), this, SLOT(actionImportProperties()));
  importPropertiesAction->setEnabled(false);

  fileMenu->addSeparator();

  openGuiAction =
      fileMenu->addAction("Open mcrl2-gui", this, SLOT(actionOpenMcrl2gui()));

  fileMenu->addSeparator();

  exitAction = fileMenu->addAction("Exit", QKeySequence(Qt::CTRL | Qt::Key_Q), this, SLOT(close()));

  /* Create the Edit menu (actions are added in updateEditMenu()) */
  editMenu = menuBar()->addMenu("Edit");
  this->updateEditMenu(this, QApplication::focusWidget());

  /* Create the View Menu (actions are added in setupDocks()) */
  viewMenu = menuBar()->addMenu("View");

  /* Create the Tools menu */
  QMenu* toolsMenu = menuBar()->addMenu("Tools");

  parseAction = toolsMenu->addAction(parseStartIcon, parseStartText, QKeySequence(Qt::ALT | Qt::Key_P), this,
                                     SLOT(actionParse()));

  simulateAction = toolsMenu->addAction(simulateStartIcon, simulateStartText, QKeySequence(Qt::ALT | Qt::Key_S),
                                        this, SLOT(actionSimulate()));

  toolsMenu->addSeparator();

  showLtsAction = toolsMenu->addAction(showLtsStartIcon, showLtsStartText, QKeySequence(Qt::ALT | Qt::Key_T), this,
                                       SLOT(actionShowLts()));

  showReducedLtsAction = toolsMenu->addAction(
      showReducedLtsStartIcon, showReducedLtsStartText, QKeySequence(Qt::ALT | Qt::Key_R), this,
      SLOT(actionShowReducedLts()));

  toolsMenu->addSeparator();

  addPropertyAction = toolsMenu->addAction(
      QIcon(":/icons/add_property.png"), "Add Property", QKeySequence(Qt::ALT | Qt::Key_A), this,
      SLOT(actionAddProperty()));

  verifyAllPropertiesAction = toolsMenu->addAction(
      verifyAllPropertiesStartIcon, verifyAllPropertiesStartText, QKeySequence(Qt::ALT | Qt::Key_V), this,
      SLOT(actionVerifyAllProperties()));

  /* create the options menu */
  QMenu* optionsMenu = menuBar()->addMenu("Options");

  saveIntermediateFilesMenu =
      optionsMenu->addMenu("Save intermediate files to project");
  saveIntermediateFilesMenu->setEnabled(false);
  saveIntermediateFilesMenu->setToolTipsVisible(true);

  for (std::pair<IntermediateFileType, QString> item :
       INTERMEDIATEFILETYPENAMES)
  {
    QAction* saveFileAction = saveIntermediateFilesMenu->addAction(item.second, this, SLOT(nothingSlot()));
    saveFileAction->setCheckable(true);
    saveFileAction->setProperty("filetype", item.first);
    saveFileAction->setToolTip("Changing this will only have effect on "
                               "processes that have not started yet");
    connect(saveFileAction, SIGNAL(toggled(bool)), fileSystem,
            SLOT(setSaveIntermediateFilesOptions(bool)));
  }

  optionsMenu->addAction("Tool Options", this, SLOT(actionShowToolOptions()));
}

void MainWindow::setupToolbar()
{
  toolbar = addToolBar("Tools");
  toolbar->setIconSize(QSize(48, 48));

  /* create each toolbar item by adding the actions */
  toolbar->addAction(newProjectAction);
  toolbar->addAction(openProjectAction);
  toolbar->addAction(saveAction);
  toolbar->addSeparator();
  toolbar->addAction(parseAction);
  toolbar->addAction(simulateAction);
  toolbar->addSeparator();
  toolbar->addAction(showLtsAction);
  toolbar->addAction(showReducedLtsAction);
  toolbar->addSeparator();
  toolbar->addAction(addPropertyAction);
  toolbar->addAction(verifyAllPropertiesAction);
}

void MainWindow::setDocksToDefault()
{
  addDockWidget(propertiesDock->defaultArea, propertiesDock);
  addDockWidget(consoleDock->defaultArea, consoleDock);

  propertiesDock->setFloating(false);
  consoleDock->setFloating(false);
  rewriteExpressionDock->setFloating(true);

  propertiesDock->show();
  consoleDock->show();
  rewriteExpressionDock->hide();

  // Workaround for QTBUG-65592.
  propertiesDock->setObjectName("PropertiesDockObject");
  consoleDock->setObjectName("ConsoleDockObject");
  toolbar->setObjectName("ToolbarObject");
  rewriteExpressionDock->setObjectName("RewriteExpressionDockObject");
  QByteArray array = saveState();
  restoreState(array);

  /* Also put the toolbar in the default position */
  addToolBar(Qt::TopToolBarArea, toolbar);
}

void MainWindow::setupDocks()
{
  /* instantiate the docks */
  propertiesDock =
      new PropertiesDock(processSystem, fileSystem, findAndReplaceDialog, this);
  consoleDock = new ConsoleDock(this);
  rewriteExpressionDock = new RewriteExpressionDock(specificationEditor, processSystem, this);

  /* add toggleable option in the view menu for each dock */
  viewMenu->addAction(propertiesDock->toggleViewAction());
  viewMenu->addAction(consoleDock->toggleViewAction());
  viewMenu->addAction(rewriteExpressionDock->toggleViewAction());
  rewriteExpressionDock->setEnabled(false);

  /* place the docks in the default dock layout */
  setDocksToDefault();

  /* add option to view menu to put all docks back to their default layout */
  viewMenu->addSeparator();
  viewMenu->addAction("Revert to default layout", this,
                      SLOT(setDocksToDefault()));
}

void MainWindow::onNewProjectOpened()
{
  /* change the title */
  setWindowTitle(QString("mCRL2 IDE - ").append(fileSystem->getProjectName()));

  /* add the properties to the dock */
  propertiesDock->setToNoProperties();
  for (Property property : fileSystem->getProperties())
  {
    propertiesDock->addProperty(property);
  }

  /* change the file buttons */
  changeFileButtons(false);

  toolOptionsDialog->updateToolOptions();

  rewriteExpressionDock->setEnabled(true);
}

void MainWindow::onEnterSpecificationOnlyMode()
{
  /* change the title */
  setWindowTitle(QString("mCRL2 IDE - Specification only mode - ")
                     .append(fileSystem->getSpecificationFileName()));

  /* change the file buttons */
  changeFileButtons(true);
}

void MainWindow::actionNewProject()
{
  if (askToSaveChanges("New Project"))
  {
    fileSystem->newProject();
  }
}

void MainWindow::actionOpenProject(const QString& inputFilePath)
{
  if (inputFilePath.isEmpty())
  {
    if (askToSaveChanges("Open Project"))
    {
      fileSystem->openProject();
    }
  }
  else
  {
    fileSystem->openFromArgument(inputFilePath);
  }
}

void MainWindow::actionSave()
{
  fileSystem->save();
}

void MainWindow::actionSaveAs()
{
  fileSystem->saveAs();
}

void MainWindow::actionOpenProjectFolderInExplorer()
{
  fileSystem->openProjectFolderInExplorer();
}

void MainWindow::actionImportProperties()
{
  std::list<Property> importedProperties = fileSystem->importProperties();
  for (Property property : importedProperties)
  {
    propertiesDock->addProperty(property);
  }
}

void MainWindow::actionOpenMcrl2gui()
{
  QProcess* p = new QProcess();
  p->setProgram(fileSystem->toolPath("mcrl2-gui"));
  if (!p->startDetached())
  {
    QMessageBox::warning(this, "mCRL2 IDE",
                         "Failed to start mcrl2-gui: " + p->errorString());
  }
}

void MainWindow::actionFindAndReplace()
{
  findAndReplaceDialog->resetFocus();
}

bool MainWindow::assertProjectOpened()
{
  if (!fileSystem->projectOpened())
  {
    executeInformationBox(
        this, "mCRL2 IDE",
        "It is required to create or open a project first");
    return false;
  }
  else
  {
    return true;
  }
}

bool MainWindow::assertSpecificationOpened()
{
  if (!fileSystem->getSpecificationFileName().isEmpty())
  {
    return true;
  }
  else
  {
    return assertProjectOpened();
  }
}

void MainWindow::actionParse()
{
  if (assertSpecificationOpened())
  {
    if (processSystem->isThreadRunning(ProcessType::Parsing))
    {
      processSystem->abortAllProcesses(ProcessType::Parsing);
    }
    else
    {
      processSystem->parseSpecification();
    }
  }
}

void MainWindow::actionShowToolOptions()
{
  if (assertSpecificationOpened())
  {
    toolOptionsDialog->show();
  }
}

void MainWindow::actionSimulate()
{
  if (assertSpecificationOpened())
  {
    if (processSystem->isThreadRunning(ProcessType::Simulation))
    {
      processSystem->abortAllProcesses(ProcessType::Simulation);
    }
    else
    {
      processSystem->simulate();
    }
  }
}

void MainWindow::actionShowLts()
{
  if (assertSpecificationOpened())
  {
    if (processSystem->isThreadRunning(ProcessType::LtsCreation))
    {
      processSystem->abortAllProcesses(ProcessType::LtsCreation);
    }
    else
    {
      lastLtsHasReduction = false;
      processSystem->showLts(mcrl2::lts::lts_eq_none);
    }
  }
}

void MainWindow::actionShowReducedLts()
{
  if (assertSpecificationOpened())
  {
    if (processSystem->isThreadRunning(ProcessType::LtsCreation))
    {
      processSystem->abortAllProcesses(ProcessType::LtsCreation);
    }
    else
    {
      /* create a dialog for asking the user what reduction to use */
      QDialog reductionDialog(this, Qt::WindowCloseButtonHint);
      QVBoxLayout vbox;
      QLabel textLabel("Reduction:");
      EquivalenceComboBox reductionBox(&reductionDialog);
      QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

      vbox.addWidget(&textLabel);
      vbox.addWidget(&reductionBox);
      vbox.addWidget(&buttonBox);

      reductionDialog.setLayout(&vbox);

      connect(&reductionBox, SIGNAL(activated(int)), &reductionDialog,
              SLOT(accept()));
      connect(&buttonBox, SIGNAL(rejected()), &reductionDialog, SLOT(reject()));

      if (last_equivalence)
      {
        reductionBox.setSelectedEquivalence(last_equivalence.value());
        connect(&buttonBox, SIGNAL(accepted()), &reductionDialog, SLOT(accept()));
      }

      /* execute the dialog */
      if (reductionDialog.exec())
      {
        mcrl2::lts::lts_equivalence reduction =
            reductionBox.getSelectedEquivalence();

        last_equivalence = reduction;

        lastLtsHasReduction = true;
        processSystem->showLts(reduction);
      }
    }
  }
}

void MainWindow::actionAddProperty()
{
  if (assertProjectOpened())
  {
    addPropertyDialog->activateDialog();
  }
}

void MainWindow::actionVerifyAllProperties()
{
  if (assertProjectOpened())
  {
    if (processSystem->isThreadRunning(ProcessType::Verification))
    {
      processSystem->abortAllProcesses(ProcessType::Verification);
    }
    else
    {
      propertiesDock->verifyAllProperties();
    }
  }
}

bool MainWindow::askToSaveChanges(QString context)
{
  if (fileSystem->isSpecificationModified())
  {
    QMessageBox::StandardButton result =
        executeQuestionBox(this, context,
                           "There are unsaved changes in the current project, "
                           "do you want to save?");
    switch (result)
    {
    case QMessageBox::Yes:
      return fileSystem->save();
    case QMessageBox::No:
      return true;
    default:
      return false;
    }
  }
  else
  {
    return true;
  }
}

void MainWindow::changeFileButtons(bool specificationOnlyMode)
{
  saveIntermediateFilesMenu->setEnabled(true);
  if (specificationOnlyMode)
  {
    saveAction->setText(saveSpecificationText);
    saveAction->setIcon(saveSpecificationIcon);
    saveAsAction->setText(saveSpecificationAsText);
    openProjectFolderInExplorerAction->setEnabled(false);
  }
  else
  {
    saveAction->setText(saveProjectText);
    saveAction->setIcon(saveProjectIcon);
    saveAsAction->setText(saveProjectAsText);
    openProjectFolderInExplorerAction->setEnabled(true);
  }
  if (fileSystem->projectOpened())
  {
    importPropertiesAction->setEnabled(true);
    rewriteExpressionDock->setEnabled(true);
  }
  else
  {
    importPropertiesAction->setEnabled(false);
    rewriteExpressionDock->setEnabled(false);
  }
}

void MainWindow::updateEditMenu(QWidget*, QWidget* widget)
{
  /* clear the edit menu to rebuild it from scratch */
  editMenu->clear();

  /* if the widget in focus is a text editor, map all the actions to this text
   *   editor, otherwise use the main specification editor as placeholder and
   *   make all actions disabled */
  QPlainTextEdit* textwidget = qobject_cast<QPlainTextEdit*>(widget);
  bool textWidgetHasFocus = true;
  if (textwidget == nullptr)
  {
    textWidgetHasFocus = false;
    textwidget = specificationEditor;
  }

  editMenu->addAction("Undo", QKeySequence::Undo, textwidget, SLOT(undo()));
  editMenu->addAction("Redo", QKeySequence::Redo, textwidget, SLOT(redo()));
  editMenu->addSeparator();
  editMenu->addAction("Find and Replace", QKeySequence::Find, this, SLOT(actionFindAndReplace()));
  editMenu->addSeparator();
  editMenu->addAction("Cut", QKeySequence::Cut, textwidget, SLOT(cut()));
  editMenu->addAction("Copy", QKeySequence::Copy, textwidget, SLOT(copy()));
  editMenu->addAction("Paste", QKeySequence::Paste, textwidget, SLOT(paste()));
  editMenu->addAction("Delete", QKeySequence::Delete, textwidget, SLOT(deleteChar()));
  editMenu->addAction("Select All", QKeySequence::SelectAll, textwidget, SLOT(selectAll()));

  if (!textWidgetHasFocus)
  {
    for (QAction* action : editMenu->actions())
    {
      action->setEnabled(false);
    }
  }
}

void MainWindow::changeToolButtons(bool toAbort, ProcessType processType)
{
  switch (processType)
  {
  case ProcessType::Parsing:
    if (toAbort)
    {
      parseAction->setText(parseAbortText);
      parseAction->setIcon(parseAbortIcon);
    }
    else
    {
      parseAction->setText(parseStartText);
      parseAction->setIcon(parseStartIcon);
    }
    break;
  case ProcessType::Simulation:
    if (toAbort)
    {
      simulateAction->setText(simulateAbortText);
      simulateAction->setIcon(simulateAbortIcon);
    }
    else
    {
      simulateAction->setText(simulateStartText);
      simulateAction->setIcon(simulateStartIcon);
    }
    break;
  case ProcessType::LtsCreation:
    if (toAbort)
    {
      if (lastLtsHasReduction)
      {
        showLtsAction->setEnabled(false);
        showReducedLtsAction->setText(showReducedLtsAbortText);
        showReducedLtsAction->setIcon(showReducedLtsAbortIcon);
      }
      else
      {
        showReducedLtsAction->setEnabled(false);
        showLtsAction->setText(showLtsAbortText);
        showLtsAction->setIcon(showLtsAbortIcon);
      }
    }
    else
    {
      showLtsAction->setEnabled(true);
      showLtsAction->setText(showLtsStartText);
      showLtsAction->setIcon(showLtsStartIcon);
      showReducedLtsAction->setEnabled(true);
      showReducedLtsAction->setText(showReducedLtsStartText);
      showReducedLtsAction->setIcon(showReducedLtsStartIcon);
    }
    break;
  case ProcessType::Verification:
    if (toAbort)
    {
      verifyAllPropertiesAction->setText(verifyAllPropertiesAbortText);
      verifyAllPropertiesAction->setIcon(verifyAllPropertiesAbortIcon);
    }
    else
    {
      verifyAllPropertiesAction->setText(verifyAllPropertiesStartText);
      verifyAllPropertiesAction->setIcon(verifyAllPropertiesStartIcon);
    }
    break;
  default:
    break;
  }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
  /* Do find next or find previous if the corresponding keys are pressed */
  if (event->matches(QKeySequence::FindNext))
  {
    findAndReplaceDialog->findNext(true);
  }
  else if (event->matches(QKeySequence::FindPrevious))
  {
    findAndReplaceDialog->findNext(false);
  }
  else
  {
    QMainWindow::keyPressEvent(event);
  }
}

bool MainWindow::event(QEvent* event)
{
  switch (event->type())
  {
  case QEvent::WindowActivate:
    /* if the project has been modified outside of the IDE, ask to reload the
     *   project */
    if (!reloadIsBeingHandled &&
        (fileSystem->projectOpened() || fileSystem->inSpecificationOnlyMode()))
    {
      reloadIsBeingHandled = true;
      if (fileSystem->isProjectNewlyModifiedFromOutside())
      {
        if (executeBinaryQuestionBox(
                this, "mCRL2 IDE",
                "The project has been modified from outside of the IDE, do you "
                "want to reload it?"))
        {
          fileSystem->reloadProject();
        }
        else
        {
          fileSystem->setProjectModified();
        }
      }

      reloadIsBeingHandled = false;
    }

    break;

  case QEvent::Close:
    /* if there are changes, ask the user to save the project first */
    if (!askToSaveChanges("mCRL2 IDE"))
    {
      event->ignore();
      return false;
    }

    /* save the settings for the main window */
    settings->setValue("geometry", saveGeometry());
    settings->setValue("windowstate", saveState());

    /* remove the temporary folder */
    fileSystem->removeTemporaryFolder();

    /* abort all processes */
    for (ProcessType processType : PROCESSTYPES)
    {
      processSystem->abortAllProcesses(processType);
    }

    break;

  default:
    break;
  }

  return QMainWindow::event(event);
}

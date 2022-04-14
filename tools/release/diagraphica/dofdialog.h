// Author(s): Rimco Boudewijns
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef DOFDIALOG_H
#define DOFDIALOG_H

#include "ui_dofdialog.h"

#include <QDialog>
#include <QComboBox>
#include "colorchooser.h"

class DofDialog : public QDialog
{
    Q_OBJECT
    
  public:
    explicit DofDialog(Graph* graph, Shape* shape, QWidget *parent = 0);
    Graph* graph() { return m_graph; }
    Shape* shape() { return m_shape; }
    
  public slots:
    void attributeSelected(int index);

  protected slots:
    void colorActivated() { emit(dofActivated(5)); }
    void opacityActivated() { emit(dofActivated(6)); }

  signals:
    void dofActivated(int dofIndex);

  protected:
    bool eventFilter(QObject *object, QEvent *event);

  private:
    Ui::DofDialog m_ui;
    ColorChooser *m_colorChooser;
    ColorChooser *m_opacityChooser;

    Graph* m_graph;
    Shape* m_shape;

    QList<QComboBox*> m_comboBoxes;

};

#endif // DOFDIALOG_H

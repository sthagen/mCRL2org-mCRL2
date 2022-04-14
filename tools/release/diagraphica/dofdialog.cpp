// Author(s): Rimco Boudewijns
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "dofdialog.h"

DofDialog::DofDialog(Graph* graph, Shape* shape, QWidget *parent) :
  QDialog(parent),
  m_graph(graph),
  m_shape(shape)
{
  m_ui.setupUi(this);

  setAttribute(Qt::WA_DeleteOnClose);
  connect(shape, SIGNAL(destroyed()), this, SLOT(close()));
  connect(this, SIGNAL(dofActivated(int)), shape, SLOT(setModeEditDof(int)));

  m_comboBoxes.clear();

  for (int i = 0; i < m_shape->dofCount(); ++i)
  {
    QComboBox* comboBox = new QComboBox(this);
    comboBox->addItem("None");

    DOF* dof = m_shape->dof(i);
    Attribute* currentAttribute = (dof == 0 ? 0 : dof->attribute());

    for (std::size_t j = 0; j < m_graph->getSizeAttributes(); ++j)
    {
      Attribute* attribute = m_graph->getAttribute(j);
      comboBox->addItem(attribute->name());
      if (currentAttribute == attribute)
      {
        comboBox->setCurrentIndex(comboBox->count()-1);
      }
    }

    m_ui.formLayout->addRow(m_shape->dofLabel(i), comboBox);

    m_comboBoxes.insert(i, comboBox);
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(attributeSelected(int)));
    comboBox->setFocusPolicy(Qt::StrongFocus);
    comboBox->installEventFilter(this);
  }
  m_ui.colorLabel->setText(m_shape->colorDOF()->label());
  m_ui.opacityLabel->setText(m_shape->opacityDOF()->label());

  m_colorChooser = new ColorChooser(m_ui.colorChooser, m_shape->colorDOF(), &m_shape->colorYValues(), ColorChooser::HueColor);
  m_ui.colorChooser->layout()->addWidget(m_colorChooser);
  connect(m_colorChooser, SIGNAL(activated()), this, SLOT(colorActivated()));

  m_opacityChooser = new ColorChooser(m_ui.opacityChooser, m_shape->opacityDOF(), &m_shape->opacityYValues(), ColorChooser::OpacityColor);
  m_ui.opacityChooser->layout()->addWidget(m_opacityChooser);
  connect(m_opacityChooser , SIGNAL(activated()), this, SLOT(opacityActivated()));
}

void DofDialog::attributeSelected(int index)
{
  if (m_graph != 0)
  {
    QObject* sender = QObject::sender();
    QComboBox* comboBox = dynamic_cast<QComboBox*>(sender);
    if (comboBox != 0)
    {
      int dofIndex = m_comboBoxes.indexOf(comboBox);
      if (dofIndex != -1)
      {
        DOF* dof = m_shape->dof(dofIndex);
        if (dof != 0)
        {
          if (index > 0)
          {
            dof->setAttribute(m_graph->getAttribute(index-1));
          }
          else
          {
            dof->setAttribute(0);
          }
        }
      }
    }
  }
}

bool DofDialog::eventFilter(QObject *object, QEvent *event)
{
  if (event->type() == QEvent::FocusIn)
  {
    QComboBox* comboBox = dynamic_cast<QComboBox*>(object);
    if (comboBox != 0)
    {
      int dofIndex = m_comboBoxes.indexOf(comboBox);
      if (dofIndex != -1)
      {
        emit(dofActivated(dofIndex));
      }
      return true;
    }
  }
  return false;
}

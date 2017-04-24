// Author(s): Bas Ploeger, Carst Tankink, Ruud Koolen
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "markstateruledialog.h"
#include <QColorDialog>
#include <utility>


MarkStateRuleDialog::MarkStateRuleDialog(QWidget *parent, LTS* lts, QColor color, int parameter, bool negated, const QSet<int>& values):
  QDialog(parent),
  m_lts(lts)
{
  m_ui.setupUi(this);

  connect(m_ui.color, SIGNAL(clicked()), this, SLOT(colorClicked()));
  connect(m_ui.parameterList, SIGNAL(itemSelectionChanged()), this, SLOT(parameterSelected()));

  setColor(std::move(color));

  for (size_t i = 0; i < m_lts->getNumParameters(); i++)
  {
    m_ui.parameterList->addItem(QString::fromStdString(m_lts->getParameterName(i)));
  }

  assert(parameter < static_cast<int>(m_lts->getNumParameters()));
  m_ui.parameterList->item(parameter)->setSelected(true);

  if (negated)
  {
    m_ui.relationList->item(0)->setSelected(false);
    m_ui.relationList->item(1)->setSelected(true);
  }
  else
  {
    m_ui.relationList->item(0)->setSelected(true);
    m_ui.relationList->item(1)->setSelected(false);
  }

  for (int i = 0; i < m_ui.valueList->count(); i++)
  {
    m_ui.valueList->item(i)->setCheckState(values.contains(i) ? Qt::Checked : Qt::Unchecked);
  }
}

QSet<int> MarkStateRuleDialog::values()
{
  QSet<int> output;
  for (int i = 0; i < m_ui.valueList->count(); i++)
  {
    if (m_ui.valueList->item(i)->checkState() == Qt::Checked)
    {
      output += i;
    }
  }
  return output;
}

void MarkStateRuleDialog::colorClicked()
{
  QColor color = QColorDialog::getColor(m_color, this);
  if (color.isValid())
  {
    setColor(color);
  }
}

void MarkStateRuleDialog::setColor(const QColor& color)
{
  m_color = color;
  m_ui.color->setAutoFillBackground(true);
  QString colorString = QString("rgb(") + QString::number(color.red()) + ", " + QString::number(color.green()) + ", " + QString::number(color.blue()) + ")";
  m_ui.color->setStyleSheet(QString("background-color: ") + colorString + "; color: " + colorString + ";");
}

void MarkStateRuleDialog::parameterSelected()
{
  QList<QListWidgetItem *> selection = m_ui.parameterList->selectedItems();
  if (selection.empty())
  {
    return;
  }
  int parameter = m_ui.parameterList->row(selection[0]);

  if (parameter != m_currentParameter)
  {
    m_currentParameter = parameter;

    m_ui.valueList->clear();
    std::vector<std::string> domain = m_lts->getParameterDomain(parameter);
    for (size_t i = 0; i < domain.size(); i++)
    {
      m_ui.valueList->addItem(QString::fromStdString(domain[i]));
      m_ui.valueList->item(i)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
      m_ui.valueList->item(i)->setCheckState(Qt::Unchecked);
    }
  }
}

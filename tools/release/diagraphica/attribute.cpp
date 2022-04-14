// Author(s): A.J. (Hannes) pretorius
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file ./attribute.cpp

#include "attribute.h"

// -- constructors and destructor -----------------------------------

Attribute::Attribute(
  QString name,
  QString type,
  const std::size_t& idx)
    : QObject()
{
  m_name  = name;
  m_type  = type;
  index = idx;
}


Attribute::Attribute(const Attribute& attr)
    : QObject()
{
  index = attr.index;
  m_name  = attr.m_name;
  m_type  = attr.m_type;
}


Attribute::~Attribute()
{}


// -- set functions -------------------------------------------------


void Attribute::setIndex(const std::size_t& idx)
{
  index = idx;
}


void Attribute::setName(QString name)
{
  m_name = name;
  emit renamed();
}


void Attribute::setType(QString type)
{
  m_type = type;
}


void Attribute::clusterValues(
  const std::vector< int > & /*indices*/,
  const std::string& /*newValue*/)
{}


void Attribute::moveValue(
  const std::size_t& /*idxFr*/,
  const std::size_t& /*idxTo*/)
{}


void Attribute::configValues(
  const std::vector< std::string > &/*curDomain*/,
  std::map< std::size_t, std::size_t  > &/*origToCurDomain*/)
{}


// -- get functions -------------------------------------------------


std::size_t Attribute::getIndex()
{
  return index;
}


QString Attribute::name()
{
  return m_name;
}


QString Attribute::type()
{
  return m_type;
}


std::size_t Attribute::getSizeOrigValues()
{
  return 0;
}


Value* Attribute::getOrigValue(std::size_t /*idx*/)
{
  return 0;
}


Value* Attribute::getCurValue(std::size_t /*idx*/)
{
  return 0;
}


// -- end -----------------------------------------------------------

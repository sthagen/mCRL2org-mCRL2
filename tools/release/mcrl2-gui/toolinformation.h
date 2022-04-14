// Author(s): Rimco Boudewijns
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef TOOLINFORMATION_H
#define TOOLINFORMATION_H

#include <set>

#include <QString>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QWidget>
#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QDomDocument>

enum ArgumentType
{
  StringArgument,
  LevelArgument,
  EnumArgument,
  FileArgument,
  IntegerArgument,
  RealArgument,
  BooleanArgument,
  InvalidArgument
};

struct ToolValue
{
    ToolValue() :
      standard(false)
    {}
    ToolValue(bool standard, QString nameShort, QString nameLong, QString description) :
      standard(standard),
      nameShort(nameShort),
      nameLong(nameLong),
      description(description)
    {}
    bool standard;
    QString nameShort, nameLong, description;
};

struct ToolArgument
{
    ToolArgument() :
      optional(true),
      type(InvalidArgument)
    {}
    ToolArgument(bool optional, ArgumentType type, QString name) :
      optional(optional),
      type(type),
      name(name)
    {}

    bool optional;
    ArgumentType type;
    QString name;

    QList<ToolValue> values;
};

struct ToolOption
{
    ToolOption() :
      standard(false)
    {}
    ToolOption(bool standard, QString nameShort, QString nameLong, QString description) :
      standard(standard),
      nameShort(nameShort),
      nameLong(nameLong),
      description(description)
    {}

    bool hasArgument() { return (argument.type != InvalidArgument); }

    bool standard;
    QString nameShort, nameLong, description;

    ToolArgument argument;

};

class ToolInformation
{
  public:
    ToolInformation(QString name, QString input, QString input2, QString output, bool guiTool);

    void load();
    bool hasOutput() { return !output.isEmpty(); }
    bool hasSecondInput() { return !input2.isEmpty(); }
    bool inputMatchesAny(const QStringList& filetypes);

    QString path, name, input2, output, desc, author;
    std::set<QString> input;
    bool guiTool, valid;

    QList<ToolOption> options;

  private:
    void parseOptions(QDomElement optionsElement);
    ArgumentType guessType(QString type, QString name);
};

#endif // TOOLINFORMATION_H

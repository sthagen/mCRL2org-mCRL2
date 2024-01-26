// Author(s): Jan Friso Groote
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file lps2pres.cpp
/// \brief This tool translates an .lps and a real modal formula to
///        a parameterised real equation system. 

#include "mcrl2/utilities/input_output_tool.h"
#include "mcrl2/pres/tools.h"
#include "mcrl2/pres/pres_output_tool.h"

using namespace mcrl2;
using namespace mcrl2::pres_system;
using namespace mcrl2::utilities;
using namespace mcrl2::utilities::tools;
using namespace mcrl2::log;
using pres_system::tools::pres_output_tool;

class lps2pres_tool : public pres_output_tool<input_output_tool>
{
    typedef pres_output_tool<input_output_tool> super;

  protected:
    std::string formula_filename;
    bool timed = false;
    bool unoptimized = false;
    bool preprocess_modal_operators = false;
    bool check_only = false;

    std::string synopsis() const override
    {
      return "[OPTION]... --formula=FILE [INFILE [OUTFILE]]\n";
    }

    void add_options(interface_description& desc) override
    {
      super::add_options(desc);
      desc.add_option("formula", make_file_argument("FILE"),
                      "use the state formula from FILE", 'f');
      desc.add_option("preprocess-modal-operators",
                      "insert dummy fixpoints in modal operators, which may lead to smaller PRESs", 'm');
      desc.add_option("timed",
                      "use the timed version of the algorithm, even for untimed LPS's", 't');
      desc.add_option("unoptimized",
                      "do not simplify boolean expressions", 'u');
      desc.add_hidden_option("check-only",
                             "check syntax and semantics of state formula; do not generate PRES", 'e');
    }

    void parse_options(const command_line_parser& parser) override
    {
      super::parse_options(parser);
      if (parser.options.count("formula"))
      {
        formula_filename = parser.option_argument("formula");
      }
      preprocess_modal_operators = parser.options.count("preprocess-modal-operators") > 0;
      timed       = parser.options.count("timed") > 0;
      unoptimized = parser.options.count("unoptimized") > 0;
      check_only = parser.options.count("check-only") > 0;
    }

  public:
    lps2pres_tool() : super(
        "lps2pres",
        "Jan Friso Groote. Based on the tool lps2pbes",
        "generate a PRES from a stochastic LPS and a modal formula over reals",
        "Convert the state formula in FILE and the LPS in INFILE to a parameterised "
        "real equation system (PRES) and save it to OUTFILE. If OUTFILE is not "
        "present, stdout is used. If INFILE is not present, stdin is used."
      )
    {}

    bool run() override
    {
      lps2pres(input_filename(),
               output_filename(),
               pres_output_format(),
               formula_filename,
               timed,
               unoptimized,
               preprocess_modal_operators,
               check_only
             );
      return true;
    }

};

int main(int argc, char** argv)
{
  return lps2pres_tool().execute(argc, argv);
}

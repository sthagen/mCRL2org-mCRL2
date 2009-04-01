// Author(s): J.van der Wulp, Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file ./lpsparelm.cpp

#include "boost.hpp" // precompiled headers

#include "mcrl2/lps/specification.h"
#include "mcrl2/lps/parelm.h"

#include "mcrl2/utilities/input_output_tool.h"
#include "mcrl2/utilities/squadt_tool.h"

using namespace mcrl2::utilities::tools;

class lps_parelm_tool : public squadt_tool< input_output_tool >
{
  typedef squadt_tool< mcrl2::utilities::tools::input_output_tool > super;

  public:

    lps_parelm_tool() : super(
             "lpsparelm",
             "Jeroen van der Wulp and Wieger Wesselink",
             "remove unused parameters from an LPS",
             "Remove unused parameters from the linear process specification (LPS) in INFILE "
             "and write the result to OUTFILE. If INFILE is not present, stdin is used. If "
             "OUTFILE is not present, stdout is used.") {
    }

    bool run() {
      mcrl2::lps::specification lps_specification;
      lps_specification.load(m_input_filename);
      mcrl2::lps::specification r = mcrl2::lps::parelm(lps_specification);
      r.save(m_output_filename);
      return true;
    }

// Squadt protocol interface and utility pseudo-library
#ifdef ENABLE_SQUADT_CONNECTIVITY
    /** \brief configures tool capabilities */
    void set_capabilities(tipi::tool::capabilities& c) const {
      c.add_input_configuration("main-input",
                 tipi::mime_type("lps", tipi::mime_type::application),
                                         tipi::tool::category::transformation);
    }

    /** \brief queries the user via SQuADT if needed to obtain configuration information */
    void user_interactive_configuration(tipi::configuration& c) {
      if (!c.output_exists("main-output")) {
        c.add_output("main-output",
                 tipi::mime_type("lps", tipi::mime_type::application),
                                                 c.get_output_name(".lps"));
      }
    }

    /** \brief check an existing configuration object to see if it is usable */
    bool check_configuration(tipi::configuration const& c) const {
      return c.input_exists("main-input") && c.output_exists("main-output");
    }

    /** \brief performs the task specified by a configuration */
    bool perform_task(tipi::configuration& c) {
      m_input_filename  = c.get_input("main-input").location();
      m_output_filename = c.get_output("main-output").location();

      return run();
    }
#endif
};

int main(int argc, char** argv)
{
  MCRL2_ATERMPP_INIT(argc, argv)

  lps_parelm_tool tool;

  return tool.execute(argc, argv);
}

// Author(s): Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file bessolve.cpp

#define NAME "bessolve"
#define AUTHOR "Jeroen Keiren"

#include "mcrl2/utilities/input_tool.h"
#include "mcrl2/bes/pbes_input_tool.h"
#include "mcrl2/bes/gauss_elimination.h"
#include "mcrl2/bes/small_progress_measures.h"
#include "mcrl2/bes/local_fixpoints.h"
#include "mcrl2/bes/solution_strategy.h"

using namespace mcrl2::log;
using namespace mcrl2::utilities::tools;
using namespace mcrl2::utilities;
using namespace mcrl2::core;
using namespace mcrl2;
using bes::tools::bes_input_tool;

using namespace mcrl2::bes;

//local declarations

class bessolve_tool: public bes_input_tool<input_tool>
{
  private:
    typedef bes_input_tool<input_tool> super;

  public:
    bessolve_tool()
      : super(NAME, AUTHOR,
              "solve a BES",
              "Solve the BES in INFILE. If INFILE is not present, stdin is used."
             ),
      strategy(small_progr_measures)
    {}

    bool run() override
    {
      bes::boolean_equation_system bes;
      load_bes(bes,input_filename(),bes_input_format());

      mCRL2log(verbose) << "solving BES in " <<
                   (input_filename().empty()?"standard input":input_filename()) << " using " <<
                   solution_strategy_to_string(strategy) << "" << std::endl;

      bool result;
      std::vector<bool> full_solution;

      timer().start("solving");
      switch (strategy)
      {
        case gauss:
          result = gauss_elimination(bes);
          break;
        case small_progr_measures:
          result = small_progress_measures(bes);
          break;
        case local_fixed_point:
          result = local_fixpoints(bes, &full_solution);
          break;
        default:
          throw mcrl2::runtime_error("unhandled strategy provided");
      }
      timer().finish("solving");

      mCRL2log(info) << "The solution for the initial variable of the BES is " << (result?"true":"false") << std::endl;

      if (print_justification)
      {
        print_justification_tree(bes, full_solution, result);
      }

      return true;
    }

  protected:
    solution_strategy_t strategy;
    bool print_justification = false;

    void add_options(interface_description& desc) override
    {
      super::add_options(desc);
      desc.add_option("strategy", make_enum_argument<solution_strategy_t>("STRATEGY")
                      .add_value(small_progr_measures, true)
                      .add_value(gauss)
                      .add_value(local_fixed_point),
                      "solve the BES using the specified STRATEGY:", 's');
      desc.add_option("print-justification", "print justification for solution. Works only with the local fixpoint strategy.", 'j');
    }

    void parse_options(const command_line_parser& parser) override
    {
      super::parse_options(parser);
      strategy = parser.option_argument_as<solution_strategy_t>("strategy");
      print_justification = parser.options.count("print-justification") > 0;
      if (print_justification && strategy!=local_fixed_point)
      {
        throw mcrl2::runtime_error("Justifications can only be printed when the solving strategy is lf.");
      } 
    }
};

int main(int argc, char* argv[])
{
  return bessolve_tool().execute(argc, argv);
}


// Author(s): Frank Stappers, Jan Friso Groote
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file lts2lps.cpp
/// \brief This tool reads a mcrl2 specification of a linear process,
/// and translates it directly into LPS format.

#include "boost.hpp" // precompiled headers

#define TOOLNAME "lts2lps"
#define AUTHOR "Frank Stappers"

#include <fstream>
#include <iostream>
#include <string>
#include "mcrl2/core/messaging.h"
#include "mcrl2/atermpp/aterm_init.h"

#include "mcrl2/utilities/input_output_tool.h"
#include "mcrl2/utilities/mcrl2_gui_tool.h"

#include "mcrl2/data/parse.h"
#include "mcrl2/data/detail/internal_format_conversion.h"
#include "mcrl2/process/parse.h"
#include "mcrl2/process/typecheck.h"
#include "mcrl2/lps/parse.h"
#include "mcrl2/lps/typecheck.h"
#include "mcrl2/lts/lts_io.h"
#include "mcrl2/lts/detail/lts_convert.h"




using namespace mcrl2;
using mcrl2::utilities::tools::input_output_tool;
using namespace mcrl2::utilities;
using namespace mcrl2::core;
using namespace mcrl2::lts;
using namespace mcrl2::lps;
using namespace mcrl2::data;



class lts2lps_tool : public input_output_tool
{
  private:
    typedef input_output_tool super;

  protected:
  
    typedef enum { none_e, data_e, lps_e, mcrl2_e } data_file_type_t;

    std::string infilename;
    std::string outfilename;
    mcrl2::lts::lts_type intype;
    data_file_type_t data_file_type;
    std::string datafile;
    

    void add_options(interface_description &desc)
    {
      super::add_options(desc);

      desc.add_option("data", make_mandatory_argument("FILE"),
          "use FILE as the data and action specification. "
          "FILE must be a .mcrl2 file which does not contain an init clause. ", 'D');
    
      desc.add_option("lps", make_mandatory_argument("FILE"),
          "use FILE for the data and action specification. "
          "FILE must be a .lps file. ", 'l');
    
      desc.add_option("mcrl2", make_mandatory_argument("FILE"),
          "use FILE as the data and action specification for the LTS. "
          "FILE must be a .mcrl2 file. ", 'm');
    }

    void parse_options(const command_line_parser& parser)
    {
      using namespace mcrl2::lts;
      super::parse_options(parser);

      if (parser.options.count("data")) 
      {
        if (1 < parser.options.count("data")) 
        {
          std::cerr << "warning: multiple data specification files are specified; can only use one.\n";
        }
        data_file_type=data_e;
        datafile = parser.option_argument("data");
      }  
 
      if (parser.options.count("lps")) 
      {
        if (1 < parser.options.count("lps") || data_file_type!=none_e) 
        {
          std::cerr << "warning: multiple data specification files are specified; can only use one.\n";
        }
        
        data_file_type=lps_e;
        datafile = parser.option_argument("lps");
      }  
 
      if (parser.options.count("mcrl2")) 
      {
        if (1 < parser.options.count("mcrl2") || data_file_type!=none_e) 
        {
          std::cerr << "warning: multiple data specification files are specified; can only use one.\n";
        }
        
        data_file_type=mcrl2_e;
        datafile = parser.option_argument("mcrl2");
      }  
 
      if (parser.options.count("in")) {
        if (1 < parser.options.count("in")) {
          parser.error("multiple input formats specified; can only use one");
        }

        intype = mcrl2::lts::detail::parse_format(parser.option_argument("in"));
        if (intype == lts_none || intype == lts_dot)  
        {
          parser.error("option -i/--in has illegal argument '" +
            parser.option_argument("in") + "'");
        }
      }


      infilename       = input_filename();
      outfilename      = output_filename();
    } 

  public:

    lts2lps_tool() : super(
             TOOLNAME,
             AUTHOR,
             "translates an LTS into an LPS",
             "Translates an LTS in INFILE and writes the resulting LPS to "
             "OUTFILE. If OUTFILE is not present, standard output is used. If INFILE is not "
             "present, standard input is used."),
      intype(mcrl2::lts::lts_none),data_file_type(none_e)
    {}


  protected:  

    void local_transform(lts_lts_t &l1, lts_lts_t &l2)
    {
      if (data_file_type!=none_e)
      { 
        std::cerr << "The lts file comes with a data specification. Ignoring the extra data and action label specification provided." << std::endl;
      }
      l1.swap(l2);
    }


    template <class LTS_TYPE>
    void local_transform(LTS_TYPE &l1, lts_lts_t &l2)
    { /* All other LTS_TYPEs than lts_lts_t require an external
         datatype.
      */
      data_specification data;
      action_label_list action_labels;
      const variable_list process_parameters;  // Process parameters remain empty.
      bool extra_data_is_defined=false;

      /* Read data specification (if any) */ 
      if (data_file_type==none_e)
      {
         std::cerr << "No data and action label specification is provided. Only the standard data types and no action labels can be used." << std::endl;
      }
      else if (data_file_type==lps_e)
      {
        // First try to read the provided file as a .lps file.
        lps::specification spec;
        spec.load(datafile.c_str());
        data=spec.data();
        action_labels=spec.action_labels();
        extra_data_is_defined=true;
      }
      else
      { 
        // data_file_type==data_e or data_file_type==mcrl2_e
        std::ifstream dfile( datafile.c_str() );

        if( !dfile )
        {
          std::cerr << "Cannot read data specification file. Only the standard data types and no action labels can be used." << std::endl;
        }
        else 
        {
          std::stringstream lps;
          char ch;
          while(dfile)
          {
            dfile.get(ch);
            if(dfile) lps << ch;
          }
          dfile.close();
          lps << std::endl;

          if (data_file_type==data_e)
          { 
            lps <<"init delta;\n";
          }

          using namespace mcrl2::process;
          // The function below parses and typechecks the process specification.
          process_specification process_spec = parse_process_specification(lps.str(),false);
          data=process_spec.data();
          action_labels=process_spec.action_labels();
          extra_data_is_defined=true;
        }
      }

      mcrl2::lts::detail::lts_convert(l1,l2,data,action_labels,process_parameters,extra_data_is_defined);

      /* if (gsVerbose)
      {
        std::cerr << "Start type checking action labels\n";
      }
      action_summand_vector action_summands;
      const variable process_parameter("x",mcrl2::data::sort_pos::pos());
      const variable_list process_parameters=push_back(variable_list(),process_parameter);
      const atermpp::set< data::variable> global_variables;
      // Add a single delta.
      const deadlock_summand_vector deadlock_summands(1,deadlock_summand(variable_list(), sort_bool::true_(), deadlock()));
      const linear_process lps(process_parameters,deadlock_summands,action_summand_vector());
      const process_initializer initial_process(push_back(assignment_list(),
                                                          assignment(process_parameter,sort_pos::pos(l.initial_state()+1))));

      const lps::specification spec(data,action_labels,global_variables,lps,initial_process);
      
      // First collect all the multi actions to allow them to be type checked as a whole, which
      // is much more efficient than type checking them individually (because this requires the
      // data specification to be transformed in the type checker each time.
      
      atermpp::vector <multi_action> all_multi_actions;
      for(mcrl2::lts::transition_const_range r = l.get_transitions(); !r.empty(); r.advance_begin(1))
      { 
        const transition t=r.front();
        const lps::multi_action actions=parse_multi_action(
                     mcrl2::lts::detail::pretty_print_label_value(l.label_value(t.label())));
        all_multi_actions.push_back(actions);
      }
      type_check(all_multi_actions,spec.data(),spec.action_labels());   */
    }
    
    template <class LTS_TYPE>
    bool transform_lps2lts()
    {   
      /* Read LTS */
      LTS_TYPE l1;
      l1.load(infilename);
      lts_lts_t l;
      local_transform(l1,l);

      if (gsVerbose)
      {
        std::cerr << "Start generating linear process\n";
      }
       
      action_summand_vector action_summands;
      const variable process_parameter("x",mcrl2::data::sort_pos::pos());
      const variable_list process_parameters=push_back(variable_list(),process_parameter);
      const atermpp::set< data::variable> global_variables;
      // Add a single delta.
      const deadlock_summand_vector deadlock_summands(1,deadlock_summand(variable_list(), sort_bool::true_(), deadlock()));
      const linear_process lps(process_parameters,deadlock_summands,action_summand_vector());
      const process_initializer initial_process(push_back(assignment_list(),
                                                          assignment(process_parameter,sort_pos::pos(l.initial_state()+1))));
      const lps::specification spec(l.data(),l.action_labels(),global_variables,lps,initial_process);

      // atermpp::vector <multi_action>::const_iterator multi_action_iterator=all_multi_actions.begin();
      // for(mcrl2::lts::transition_const_range r = l.get_transitions(); !r.empty(); r.advance_begin(1),++multi_action_iterator)
      for(mcrl2::lts::transition_const_range r = l.get_transitions(); !r.empty(); r.advance_begin(1))
      {
        const transition t=r.front();
        // const lps::multi_action actions=lps::multi_action(mcrl2::data::detail::internal_format_conversion_list(multi_action_iterator->actions(),spec.data()));
        const lps::multi_action actions=l.action_label(t.label()).label();
        
        assignment_list assignments;
        if (t.from()!=t.to())
        { 
          assignments=push_back(assignments,assignment(process_parameter,sort_pos::pos(t.to()+1)));
        }
        
        const action_summand summand(
                 variable_list(),
                 equal_to(process_parameter,sort_pos::pos(t.from()+1)),
                 actions,
                 assignments);
        action_summands.push_back(summand);
      }

      const linear_process lps1(process_parameters,deadlock_summands,action_summands);
      const lps::specification spec1(l.data(),l.action_labels(),global_variables,lps1,initial_process);

      if (gsVerbose)
      {
        std::cerr << "Start saving the linear process\n";
      }
      spec1.save(output_filename());
      return true;
    }

  public:
    bool run()
    {

      if (intype==lts_none)
      {
        intype = mcrl2::lts::detail::guess_format(infilename);
      }
      switch (intype)
      {
        case lts_lts:
        {
          return transform_lps2lts<lts_lts_t>();
        }
        case lts_none:
          std::cerr << "Cannot determine type of input. Assuming .aut.\n";
        case lts_aut:
        {
          return transform_lps2lts<lts_aut_t>();
        }
        case lts_fsm:
        {
          return transform_lps2lts<lts_fsm_t>();
        }
#ifdef USE_BCG
        case lts_bcg:
        {
          return transform_lps2lts<lts_bcg_t>();
        }
#endif
        case lts_dot:
        {
          return transform_lps2lts<lts_dot_t>();
        }
        case lts_svc:
        { 
          return transform_lps2lts<lts_svc_t>();
        } 
      }
      return true;
    }
};

class lts2lps_gui_tool: public mcrl2::utilities::mcrl2_gui_tool<lts2lps_tool>
{
  public:
	lts2lps_gui_tool()
    {

      m_gui_options["data"] = create_filepicker_widget("Text Files (*.txt)|*.txt|mCRL2 files (*.mcrl2)|*.mcrl2|All Files (*.*)|*.*");
      m_gui_options["lps"]  = create_filepicker_widget("LPS File (*.lps)|*.lps|All Files (*.*)|*.*");
      m_gui_options["mcrl2"] = create_filepicker_widget("mCRL2 files (*.mcrl2)|*.mcrl2|Text Files (*.txt)|*.txt|All Files (*.*)|*.*");

    }
};

int main(int argc, char** argv)
{
  MCRL2_ATERMPP_INIT(argc, argv)
 
  return lts2lps_gui_tool().execute(argc, argv);
}

/*******************************************************************\

Module: Solvers for VCs Generated by Symbolic Execution of ANSI-C

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include <solvers/sat/satcheck.h>

#ifdef HAVE_BV_REFINEMENT
#include <bv_refinement/bv_refinement_loop.h>
#endif

#include <solvers/smt1/smt1_dec.h>
#include <solvers/smt2/smt2_dec.h>
#include <solvers/cvc/cvc_dec.h>

#include "bmc.h"
#include "bv_cbmc.h"
#include "counterexample_beautification_greedy.h"
#include "version.h"

/*******************************************************************\

Function: bmc_baset::decide_default

  Inputs:

 Outputs:

 Purpose: Decide using "default" decision procedure

\*******************************************************************/

bool bmc_baset::decide_default()
{
  #if 1
  sat_minimizert satcheck;
  satcheck.set_message_handler(get_message_handler());
  satcheck.set_verbosity(get_verbosity());
  
  bv_cbmct bv_cbmc(ns, satcheck);
  
  if(options.get_option("arrays-uf")=="never")
    bv_cbmc.unbounded_array=bv_cbmct::U_NONE;
  else if(options.get_option("arrays-uf")=="always")
    bv_cbmc.unbounded_array=bv_cbmct::U_ALL;
    
  bool result=true;

  switch(run_decision_procedure(bv_cbmc))
  {
  case decision_proceduret::D_UNSATISFIABLE:
    result=false;
    report_success();
    break;

  case decision_proceduret::D_SATISFIABLE:
    if(options.get_bool_option("beautify-pbs"))
      throw "beautify-pbs is no longer supported";
    else if(options.get_bool_option("beautify-greedy"))
      counterexample_beautification_greedyt()(
        satcheck, bv_cbmc, equation, ns);

    error_trace(bv_cbmc);
    report_failure();
    break;

  default:
    error("decision procedure failed");
  }
  #else
  
  satcheck_minisat_simpt satcheck;
  satcheck.set_message_handler(get_message_handler());
  satcheck.set_verbosity(get_verbosity());
  
  bv_cbmct bv_cbmc(satcheck);
  
  if(options.get_option("arrays-uf")=="never")
    bv_cbmc.unbounded_array=bv_cbmct::U_NONE;
  else if(options.get_option("arrays-uf")=="always")
    bv_cbmc.unbounded_array=bv_cbmct::U_ALL;
    
  bool result=true;

  switch(run_decision_procedure(bv_cbmc))
  {
  case decision_proceduret::D_UNSATISFIABLE:
    result=false;
    report_success();
    break;

  case decision_proceduret::D_SATISFIABLE:
    error_trace(bv_cbmc);
    report_failure();
    break;

  default:
    error("decision procedure failed");
  }
  
  #endif

  return result;
}

/*******************************************************************\

Function: bmc_baset::bv_refinement

  Inputs:

 Outputs:

 Purpose: Decide using refinement decision procedure

\*******************************************************************/

bool bmc_baset::decide_bv_refinement()
{
  #ifdef HAVE_BV_REFINEMENT
  satcheckt satcheck;
  satcheck.set_message_handler(get_message_handler());
  satcheck.set_verbosity(get_verbosity());

  bv_refinement_loopt bv_refinement_loop(ns, satcheck);
  
  return decide(bv_refinement_loop);
  #else
  throw "bv refinement not linked in";
  #endif
}

/*******************************************************************\

Function: bmc_baset::decide_smt1

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool bmc_baset::decide_smt1(smt1_dect::solvert solver)
{
  const std::string &filename=options.get_option("outfile");
  
  if(filename=="")
  {
    smt1_dect smt1_dec(
      ns,
      "cbmc",
      "Generated by CBMC " CBMC_VERSION,
      "QF_AUFBV",
      solver);
    return decide(smt1_dec);
  }
  else if(filename=="-")
    smt1_convert(std::cout);
  else
  {
    std::ofstream out(filename.c_str());
    if(!out)
    {
      std::cerr << "failed to open " << filename << std::endl;
      return false;
    }
    
    smt1_convert(out);
  }
  
  return false;
}

/*******************************************************************\

Function: bmc_baset::smt1_convert

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void bmc_baset::smt1_convert(std::ostream &out)
{
  smt1_convt smt1_conv(
    ns,
    "cbmc",
    "Generated by CBMC " CBMC_VERSION,
    "QF_AUFBV",
    out);

  smt1_conv.set_message_handler(get_message_handler());
  
  do_conversion(smt1_conv);

  smt1_conv.dec_solve();
}

/*******************************************************************\

Function: bmc_baset::decide_smt2

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool bmc_baset::decide_smt2(smt2_dect::solvert solver)
{
  const std::string &filename=options.get_option("outfile");
  
  if(filename=="")
  {
    smt2_dect smt2_dec(
      ns,
      "cbmc",
      "Generated by CBMC " CBMC_VERSION,
      "QF_AUFBV",
      solver);
    return decide(smt2_dec);
  }
  else if(filename=="-")
    smt2_convert(std::cout);
  else
  {
    std::ofstream out(filename.c_str());
    if(!out)
    {
      std::cerr << "failed to open " << filename << std::endl;
      return false;
    }
    
    smt2_convert(out);
  }
  
  return false;
}

/*******************************************************************\

Function: bmc_baset::smt2_convert

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void bmc_baset::smt2_convert(std::ostream &out)
{
  smt2_convt smt2_conv(
    ns,
    "cbmc",
    "Generated by CBMC " CBMC_VERSION,
    "QF_AUFBV",
    out);

  smt2_conv.set_message_handler(get_message_handler());
  
  do_conversion(smt2_conv);

  smt2_conv.dec_solve();
}

/*******************************************************************\

Function: bmc_baset::decide_cvc

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool bmc_baset::decide_cvc()
{
  return decide_smt1(smt1_dect::CVC3);
}

/*******************************************************************\

Function: bmc_baset::decide_boolector

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool bmc_baset::decide_boolector()
{
  return decide_smt1(smt1_dect::BOOLECTOR);
}

/*******************************************************************\

Function: bmc_baset::decide_opensmt

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool bmc_baset::decide_opensmt()
{
  return decide_smt1(smt1_dect::OPENSMT);
}

/*******************************************************************\

Function: bmc_baset::decide_z3

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool bmc_baset::decide_z3()
{
  return decide_smt1(smt1_dect::Z3);
}

/*******************************************************************\

Function: bmc_baset::decide_yices

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool bmc_baset::decide_yices()
{
  return decide_smt1(smt1_dect::YICES);
}

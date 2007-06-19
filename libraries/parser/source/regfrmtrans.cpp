#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#ifdef _MSC_VER
#include <boost/format.hpp>
#endif

#include "dataimpl.h"
#include "libstruct.h"
#include "print/messaging.h"
#include "mcrl2/utilities/aterm_ext.h"

#ifdef __cplusplus
using namespace mcrl2::utilities;
#endif

//local declarations
//------------------

ATermAppl translate_reg_frms_appl(ATermAppl part);
/*Pre: part represents a part of a state formula that adheres to the internal
 *     ATerm structure after the data implementation phase
 *Ret: part in which all regular formulas are translated in terms of state and
 *     action formulas
 */

ATermList translate_reg_frms_list(ATermList parts);
/*Pre: parts represents a part of a state formula that adheres to the internal
 *     ATerm structure after the data implementation phase
 *Ret: parts in which all regular formulas are translated in terms of state and
 *     action formulas
 */

ATermAppl create_new_var_name(bool cap, int index);
//Pre: index >= 0
//Ret: a quoted ATermAppl of the form 'vn', where:
//     - v = "x", "y" or "z", in case of !cap
//     - v = "X", "Y" or "Z", in case of cap
//     - n = "",      in case n div 3 = 0
//     - n = n mod 3, in case n div 3 > 0

ATermAppl create_fresh_var_name(bool cap, ATermList terms);
//Pre: terms is a list of arbitrary terms that adhere to the internal format
//Ret: a quoted ATermAppl x that satisfies the following:
//     - x is a variable name generated by create_new_var_name(cap, i), for
//       some i
//     - x is different from the quoted ATermAppl's in terms


//implementation
//--------------

ATermAppl translate_reg_frms(ATermAppl state_frm)
{
  return translate_reg_frms_appl(state_frm);
}

ATermAppl translate_reg_frms_appl(ATermAppl part)
{
  gsDebugMsg("reducing expression\n  %P\n", part);
  if (gsIsDataExpr(part) || gsIsMultAct(part) ||
      gsIsStateVar(part) || gsIsDataVarIdInit(part))
  {
    //part is a data expression, a multiaction, a state variable or a data
    //variable declaration (with or without initialisation); return part
  } else if (gsIsStateMust(part)) {
    //part is the must operator; return equivalent non-regular formula
    ATermAppl reg_frm = ATAgetArgument(part, 0);
    ATermAppl phi = ATAgetArgument(part, 1);
    if (gsIsRegNil(reg_frm)) {
      //red([nil]phi) -> red([false*]phi)
      part = translate_reg_frms_appl(
        gsMakeStateMust(gsMakeRegTransOrNil(gsMakeStateFalse()), phi));
    } else if (gsIsRegSeq(reg_frm)) {
      ATermAppl R1 = ATAgetArgument(reg_frm, 0);
      ATermAppl R2 = ATAgetArgument(reg_frm, 1);
      //red([R1.R2]phi) -> red([R1][R2]phi)
      part = translate_reg_frms_appl(
        gsMakeStateMust(R1, gsMakeStateMust(R2, phi)));
    } else if (gsIsRegAlt(reg_frm)) {
      ATermAppl R1 = ATAgetArgument(reg_frm, 0);
      ATermAppl R2 = ATAgetArgument(reg_frm, 1);
      //red([R1+R2]phi) -> red([R1]phi) && red([R2]phi)
      part = gsMakeStateAnd(
        translate_reg_frms_appl(gsMakeStateMust(R1,phi)),
        translate_reg_frms_appl(gsMakeStateMust(R2,phi))
      );
    } else if (gsIsRegTrans(reg_frm)) {
      ATermAppl R = ATAgetArgument(reg_frm, 0);
      //red([R+]phi) -> red([R.R*]phi)
      part = translate_reg_frms_appl(
        gsMakeStateMust(gsMakeRegSeq(R,gsMakeRegTransOrNil(R)),phi));
    } else if (gsIsRegTransOrNil(reg_frm)) {
      ATermAppl R = ATAgetArgument(reg_frm, 0);
      //red([R*]phi) -> nu X. red(phi) && red([R]X),
      //where X does not occur free in phi and R
      ATermAppl X =
        create_fresh_var_name(true, ATmakeList2((ATerm) phi, (ATerm) R));
      part = gsMakeStateNu(X, ATmakeList0(), gsMakeStateAnd(
        translate_reg_frms_appl(phi),
        translate_reg_frms_appl(gsMakeStateMust(R,gsMakeStateVar(X, ATmakeList0())))
      ));
    } else {
      //reg_frm is an action formula; reduce phi
      part = gsMakeStateMust(reg_frm, translate_reg_frms_appl(phi));
    }
  } else if (gsIsStateMay(part)) {
    //part is the may operator; return equivalent non-regular formula
    ATermAppl reg_frm = ATAgetArgument(part, 0);
    ATermAppl phi = ATAgetArgument(part, 1);
    if (gsIsRegNil(reg_frm)) {
      //red(<nil>phi) -> red(<false*>phi)
      part = translate_reg_frms_appl(
        gsMakeStateMay(gsMakeRegTransOrNil(gsMakeStateFalse()), phi));
    } else if (gsIsRegSeq(reg_frm)) {
      ATermAppl R1 = ATAgetArgument(reg_frm, 0);
      ATermAppl R2 = ATAgetArgument(reg_frm, 1);
      //red(<R1.R2>phi) -> red(<R1><R2>phi)
      part = translate_reg_frms_appl(
        gsMakeStateMay(R1, gsMakeStateMay(R2, phi)));
    } else if (gsIsRegAlt(reg_frm)) {
      ATermAppl R1 = ATAgetArgument(reg_frm, 0);
      ATermAppl R2 = ATAgetArgument(reg_frm, 1);
      //red(<R1+R2>phi) -> red(<R1>phi) || red(<R2>phi)
      part = gsMakeStateOr(
        translate_reg_frms_appl(gsMakeStateMay(R1,phi)),
        translate_reg_frms_appl(gsMakeStateMay(R2,phi))
      );
    } else if (gsIsRegTrans(reg_frm)) {
      ATermAppl R = ATAgetArgument(reg_frm, 0);
      //red(<R+>phi) -> red(<R.R*>phi)
      part = translate_reg_frms_appl(
        gsMakeStateMay(gsMakeRegSeq(R,gsMakeRegTransOrNil(R)),phi));
    } else if (gsIsRegTransOrNil(reg_frm)) {
      ATermAppl R = ATAgetArgument(reg_frm, 0);
      //red(<R*>phi) -> mu X. red(phi) || red(<R>X),
      //where X does not occur free in phi and R
      ATermAppl X =
        create_fresh_var_name(true, ATmakeList2((ATerm) phi, (ATerm) R));
      part = gsMakeStateMu(X, ATmakeList0(), gsMakeStateOr(
        translate_reg_frms_appl(phi),
        translate_reg_frms_appl(gsMakeStateMay(R,gsMakeStateVar(X, ATmakeList0())))
      ));
    } else {
      //reg_frm is an action formula; reduce phi
      part = gsMakeStateMay(reg_frm, translate_reg_frms_appl(phi));
    }
  } else {
    //implement expressions in the arguments of part
    AFun head = ATgetAFun(part);
    int nr_args = ATgetArity(head);      
    if (nr_args > 0) {
      DECL_A(args,ATerm,nr_args);
      for (int i = 0; i < nr_args; i++) {
        ATerm arg = ATgetArgument(part, i);
        if (ATgetType(arg) == AT_APPL)
          args[i] = (ATerm) translate_reg_frms_appl((ATermAppl) arg);
        else //ATgetType(arg) == AT_LIST
          args[i] = (ATerm) translate_reg_frms_list((ATermList) arg);
      }
      part = ATmakeApplArray(head, args);
      FREE_A(args);
    }
  }
  return part;
}

ATermList translate_reg_frms_list(ATermList parts)
{
  ATermList result = ATmakeList0();
  while (!ATisEmpty(parts)) {
    result =
      ATinsert(result, (ATerm) translate_reg_frms_appl(ATAgetFirst(parts)));
    parts = ATgetNext(parts);
  }
  return ATreverse(result);
}

ATermAppl create_new_var_name(bool cap, int index)
{
  gsDebugMsg("creating variable with index %d and cap %s\n",
    index, cap?"true":"false");
  int suffix = index / 3;
#ifndef _MSC_VER
  char name[suffix+2];
#endif // _MSC_VER
  char base;
  //choose x/X, y/Y or z/Z
  switch (index % 3) {
    case 0:
      base = cap?'X':'x';
      break;
    case 1:
      base = cap?'Y':'y';
      break;
    default:
      base = cap?'Z':'z';
      break;
  }
#ifndef _MSC_VER
  //append suffix if necessary
  if (suffix == 0) {
    sprintf(name, "%c", base);
  } else {
    sprintf(name, "%c%d", base, suffix);
  }
  return gsString2ATermAppl(name);
#endif // _MSC_VER
#ifdef _MSC_VER
  std::string name;
  if (suffix == 0)
    name = str(boost::format("%c") % base);
  else
    name = str(boost::format("%c%d") % base % suffix);
  return gsString2ATermAppl(name.c_str());
#endif
}

ATermAppl create_fresh_var_name(bool cap, ATermList terms)
{
  gsDebugMsg("creating fresh variable for terms %t\n", terms);
  ATermAppl result = NULL;
  bool done = false;
  //iteratively create a unique variable and perform a number of checks on it;
  for (int i = 0; !done; i++) {
    //create variable with index i; capitalise variable if cap
    result = create_new_var_name(cap, i);
    //check if the new variable occurs in terms
    done = !gsOccurs((ATerm) result, (ATerm) terms);
  }
  return result;  
}

/*******************************************************************\

Module: Generates string constraints for string functions that return
        Boolean values

Author: Romain Brenguier, romain.brenguier@diffblue.com

\*******************************************************************/

#include <solvers/refinement/string_constraint_generator.h>

/*******************************************************************\

Function: string_constraint_generatort::add_axioms_for_is_prefix

  Inputs: a prefix string, a string and an integer offset

 Outputs: a Boolean expression

 Purpose: add axioms stating that the returned expression is true exactly
          when the first string is a prefix of the second one, starting at
          position offset

\*******************************************************************/

exprt string_constraint_generatort::add_axioms_for_is_prefix(
  const string_exprt &prefix, const string_exprt &str, const exprt &offset)
{
  symbol_exprt isprefix=fresh_boolean("isprefix");

  // We add axioms:
  // a1 : isprefix => |str| >= |prefix|+offset
  // a2 : forall 0<=qvar<prefix.length. isprefix =>
  //   s0[witness+offset]=s2[witness]
  // a3 : !isprefix => |str| < |prefix|+offset
  //   || (|str| >= |prefix|+offset &&0<=witness<|prefix|
  //     &&str[witness+ofsset]!=prefix[witness])

  implies_exprt a1(
    isprefix,
    str.axiom_for_is_longer_than(plus_exprt(prefix.length(), offset)));
  axioms.push_back(a1);

  symbol_exprt qvar=fresh_univ_index("QA_isprefix");
  string_constraintt a2(
    qvar,
    prefix.length(),
    isprefix,
    equal_exprt(str[plus_exprt(qvar, offset)], prefix[qvar]));
  axioms.push_back(a2);

  symbol_exprt witness=fresh_exist_index("witness_not_isprefix");
  and_exprt witness_diff(
    axiom_for_is_positive_index(witness),
    and_exprt(
      prefix.axiom_for_is_strictly_longer_than(witness),
      notequal_exprt(str[plus_exprt(witness, offset)], prefix[witness])));
  or_exprt s0_notpref_s1(
    not_exprt(
      str.axiom_for_is_longer_than(plus_exprt(prefix.length(), offset))),
    and_exprt(
      witness_diff,
      str.axiom_for_is_longer_than(
        plus_exprt(prefix.length(), offset))));

  implies_exprt a3(not_exprt(isprefix), s0_notpref_s1);
  axioms.push_back(a3);
  return isprefix;
}

/*******************************************************************\

Function: string_constraint_generatort::add_axioms_for_is_prefix

  Inputs: a function application with 2 or 3 arguments and a Boolean telling
          whether the prefix is the second argument (when swap_arguments is
          true) or the first argument

 Outputs: a Boolean expression

 Purpose: add axioms corresponding to the String.isPrefix java function

\*******************************************************************/

exprt string_constraint_generatort::add_axioms_for_is_prefix(
  const function_application_exprt &f, bool swap_arguments)
{
  const function_application_exprt::argumentst &args=f.arguments();
  assert(f.type()==bool_typet() || f.type().id()==ID_c_bool);
  string_exprt s0=add_axioms_for_string_expr(args[swap_arguments?1:0]);
  string_exprt s1=add_axioms_for_string_expr(args[swap_arguments?0:1]);
  exprt offset;
  if(args.size()==2)
    offset=from_integer(0, get_index_type());
  else if(args.size()==3)
    offset=args[2];
  return typecast_exprt(add_axioms_for_is_prefix(s0, s1, offset), f.type());
}

/*******************************************************************\

Function: string_constraint_generatort::add_axioms_for_is_empty

  Inputs: function application with a string argument

 Outputs: a Boolean expression

 Purpose: add axioms stating that the returned value is true exactly when
          the argument string is empty

\*******************************************************************/

exprt string_constraint_generatort::add_axioms_for_is_empty(
  const function_application_exprt &f)
{
  assert(f.type()==bool_typet() || f.type().id()==ID_c_bool);

  // We add axioms:
  // a1 : is_empty => |s0| = 0
  // a2 : s0 => is_empty

  symbol_exprt is_empty=fresh_boolean("is_empty");
  string_exprt s0=add_axioms_for_string_expr(args(f, 1)[0]);
  axioms.push_back(implies_exprt(is_empty, s0.axiom_for_has_length(0)));
  axioms.push_back(implies_exprt(s0.axiom_for_has_length(0), is_empty));
  return typecast_exprt(is_empty, f.type());
}

/*******************************************************************\

Function: string_constraint_generatort::add_axioms_for_is_suffix

  Inputs: a function application with 2 or 3 arguments and a Boolean telling
          whether the suffix is the second argument (when swap_arguments is
          true) or the first argument

 Outputs: a Boolean expression

 Purpose: add axioms corresponding to the String.isSuffix java function

\*******************************************************************/

exprt string_constraint_generatort::add_axioms_for_is_suffix(
  const function_application_exprt &f, bool swap_arguments)
{
  const function_application_exprt::argumentst &args=f.arguments();
  assert(args.size()==2); // bad args to string issuffix?
  assert(f.type()==bool_typet() || f.type().id()==ID_c_bool);

  symbol_exprt issuffix=fresh_boolean("issuffix");
  typecast_exprt tc_issuffix(issuffix, f.type());
  string_exprt s0=add_axioms_for_string_expr(args[swap_arguments?1:0]);
  string_exprt s1=add_axioms_for_string_expr(args[swap_arguments?0:1]);

  // We add axioms:
  // a1 : issufix => s0.length >= s1.length
  // a2 : forall witness<s1.length.
  //     issufix => s1[witness]=s0[witness + s0.length-s1.length]
  // a3 : !issuffix =>
  //   s1.length > s0.length
  //     || (s1.length > witness>=0
  //       &&s1[witness]!=s0[witness + s0.length-s1.length]

  implies_exprt a1(issuffix, s1.axiom_for_is_longer_than(s0));
  axioms.push_back(a1);

  symbol_exprt qvar=fresh_univ_index("QA_suffix");
  exprt qvar_shifted=plus_exprt(
    qvar, minus_exprt(s1.length(), s0.length()));
  string_constraintt a2(
    qvar, s0.length(), issuffix, equal_exprt(s0[qvar], s1[qvar_shifted]));
  axioms.push_back(a2);

  symbol_exprt witness=fresh_exist_index("witness_not_suffix");
  exprt shifted=plus_exprt(
    witness, minus_exprt(s1.length(), s0.length()));
  or_exprt constr3(
    s0.axiom_for_is_strictly_longer_than(s1),
    and_exprt(
      notequal_exprt(s0[witness], s1[shifted]),
      and_exprt(
        s0.axiom_for_is_strictly_longer_than(witness),
        axiom_for_is_positive_index(witness))));
  implies_exprt a3(not_exprt(issuffix), constr3);

  axioms.push_back(a3);
  return tc_issuffix;
}

/*******************************************************************\

Function: string_constraint_generatort::add_axioms_for_contains

  Inputs: function application with two string arguments

 Outputs: a Boolean expression

 Purpose: add axioms corresponding to the String.contains java function

\*******************************************************************/

exprt string_constraint_generatort::add_axioms_for_contains(
  const function_application_exprt &f)
{
  assert(f.type()==bool_typet() || f.type().id()==ID_c_bool);
  symbol_exprt contains=fresh_boolean("contains");
  typecast_exprt tc_contains(contains, f.type());
  string_exprt s0=add_axioms_for_string_expr(args(f, 2)[0]);
  string_exprt s1=add_axioms_for_string_expr(args(f, 2)[1]);

  // We add axioms:
  // a1 : contains => s0.length >= s1.length
  // a2 : 0 <= startpos <= s0.length-s1.length
  // a3 : forall qvar<s1.length. contains => s1[qvar]=s0[startpos + qvar]
  // a4 : !contains => s1.length > s0.length
  //       || (forall startpos <= s0.length-s1.length.
  //             exists witness<s1.length &&s1[witness]!=s0[witness + startpos]

  implies_exprt a1(contains, s0.axiom_for_is_longer_than(s1));
  axioms.push_back(a1);

  symbol_exprt startpos=fresh_exist_index("startpos_contains");
  minus_exprt length_diff(s0.length(), s1.length());
  and_exprt a2(
    axiom_for_is_positive_index(startpos),
    binary_relation_exprt(startpos, ID_le, length_diff));
  axioms.push_back(a2);

  symbol_exprt qvar=fresh_univ_index("QA_contains");
  exprt qvar_shifted=plus_exprt(qvar, startpos);
  string_constraintt a3(
    qvar, s1.length(), contains, equal_exprt(s1[qvar], s0[qvar_shifted]));
  axioms.push_back(a3);

  // We rewrite the axiom for !contains as:
  // forall startpos <= |s0|-|s1|.  (!contains &&|s0| >= |s1| )
  //      ==> exists witness<|s1|. s1[witness]!=s0[startpos+witness]
  string_not_contains_constraintt a4(
    from_integer(0, get_index_type()),
    plus_exprt(from_integer(1, get_index_type()), length_diff),
    and_exprt(not_exprt(contains), s0.axiom_for_is_longer_than(s1)),
    from_integer(0, get_index_type()), s1.length(), s0, s1);
  axioms.push_back(a4);

  return tc_contains;
}

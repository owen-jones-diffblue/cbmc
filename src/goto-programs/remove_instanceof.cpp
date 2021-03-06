/*******************************************************************\

Module: Remove Instance-of Operators

Author: Chris Smowton, chris.smowton@diffblue.com

\*******************************************************************/

#include "class_hierarchy.h"
#include "class_identifier.h"
#include "remove_instanceof.h"

#include <sstream>

class remove_instanceoft
{
public:
  remove_instanceoft(
    symbol_tablet &_symbol_table,
    goto_functionst &_goto_functions):
    symbol_table(_symbol_table),
    ns(_symbol_table),
    goto_functions(_goto_functions),
    lowered_count(0)
  {
    class_hierarchy(_symbol_table);
  }

  // Lower instanceof for all functions:
  void lower_instanceof();

protected:
  symbol_tablet &symbol_table;
  namespacet ns;
  class_hierarchyt class_hierarchy;
  goto_functionst &goto_functions;
  int lowered_count;

  bool lower_instanceof(goto_programt &);

  typedef std::vector<
    std::pair<goto_programt::targett, goto_programt::targett>> instanceof_instt;

  void lower_instanceof(
    goto_programt &,
    goto_programt::targett,
    instanceof_instt &);

  void lower_instanceof(
    exprt &,
    goto_programt &,
    goto_programt::targett,
    instanceof_instt &);

  bool contains_instanceof(const exprt &);
};

/*******************************************************************\

Function: remove_instanceoft::contains_instanceof

  Inputs: Expression `expr`

 Outputs: Returns true if `expr` contains any instanceof ops

 Purpose: Avoid breaking sharing by checking for instanceof
          before calling lower_instanceof.

\*******************************************************************/

bool remove_instanceoft::contains_instanceof(
  const exprt &expr)
{
  if(expr.id()==ID_java_instanceof)
    return true;
  forall_operands(it, expr)
    if(contains_instanceof(*it))
      return true;
  return false;
}

/*******************************************************************\

Function: remove_instanceoft::lower_instanceof

  Inputs: Expression to lower `expr` and the `goto_program` and
          instruction `this_inst` it belongs to.

 Outputs: Side-effect on `expr` replacing it with an explicit clsid test

 Purpose: Replaces an expression like
          e instanceof A
          with
          e.@class_identifier == "A"
          Or a big-or of similar expressions if we know of subtypes
          that also satisfy the given test.

\*******************************************************************/

void remove_instanceoft::lower_instanceof(
  exprt &expr,
  goto_programt &goto_program,
  goto_programt::targett this_inst,
  instanceof_instt &inst_switch)
{
  if(expr.id()==ID_java_instanceof)
  {
    const exprt &check_ptr=expr.op0();
    assert(check_ptr.type().id()==ID_pointer);
    const exprt &target_arg=expr.op1();
    assert(target_arg.id()==ID_type);
    const typet &target_type=target_arg.type();

    // Find all types we know about that satisfy the given requirement:
    assert(target_type.id()==ID_symbol);
    const irep_idt &target_name=
      to_symbol_type(target_type).get_identifier();
    std::vector<irep_idt> children=
      class_hierarchy.get_children_trans(target_name);
    children.push_back(target_name);

    assert(!children.empty() && "Unable to execute instanceof");

    // Insert an instruction before this one that assigns the clsid we're
    // checking against to a temporary, as GOTO program if-expressions should
    // not contain derefs.

    symbol_typet jlo("java::java.lang.Object");
    exprt object_clsid=get_class_identifier_field(check_ptr, jlo, ns);

    std::ostringstream symname;
    symname << "instanceof_tmp::instanceof_tmp" << (++lowered_count);
    auxiliary_symbolt newsym;
    newsym.name=symname.str();
    newsym.type=object_clsid.type();
    newsym.base_name=newsym.name;
    newsym.mode=ID_java;
    newsym.is_type=false;
    assert(!symbol_table.add(newsym));

    auto newinst=goto_program.insert_after(this_inst);
    newinst->make_assignment();
    newinst->code=code_assignt(newsym.symbol_expr(), object_clsid);
    newinst->source_location=this_inst->source_location;

    // Insert the check instruction after the existing one.
    // This will briefly be ill-formed (use before def of
    // instanceof_tmp) but the two will subsequently switch
    // places in lower_instanceof(goto_programt &) below.
    inst_switch.push_back(make_pair(this_inst, newinst));
    // Replace the instanceof construct with a big-or.
    exprt::operandst or_ops;
    for(const auto &clsname : children)
    {
      constant_exprt clsexpr(clsname, string_typet());
      equal_exprt test(newsym.symbol_expr(), clsexpr);
      or_ops.push_back(test);
    }
    expr=disjunction(or_ops);
  }
  else
  {
    Forall_operands(it, expr)
      lower_instanceof(*it, goto_program, this_inst, inst_switch);
  }
}

/*******************************************************************\

Function: remove_instanceoft::lower_instanceof

  Inputs: GOTO program instruction `target` whose instanceof expressions,
          if any, should be replaced with explicit tests, and the
          `goto_program` it is part of.

 Outputs: Side-effect on `target` as above.

 Purpose: See function above

\*******************************************************************/

void remove_instanceoft::lower_instanceof(
  goto_programt &goto_program,
  goto_programt::targett target,
  instanceof_instt &inst_switch)
{
  bool code_iof=contains_instanceof(target->code);
  bool guard_iof=contains_instanceof(target->guard);
  // The instruction-switching step below will malfunction if a check
  // has been added for the code *and* for the guard. This should
  // be impossible, because guarded instructions currently have simple
  // code (e.g. a guarded-goto), but this assertion checks that this
  // assumption is correct and remains true on future evolution of the
  // allowable goto instruction types.
  assert(!(code_iof && guard_iof));
  if(code_iof)
    lower_instanceof(target->code, goto_program, target, inst_switch);
  if(guard_iof)
    lower_instanceof(target->guard, goto_program, target, inst_switch);
}

/*******************************************************************\

Function: remove_instanceoft::lower_instanceof

  Inputs: `goto_program`, all of whose instanceof expressions will
          be replaced by explicit class-identifier tests.

 Outputs: Side-effect on `goto_program` as above.

 Purpose: See function above

\*******************************************************************/
bool remove_instanceoft::lower_instanceof(goto_programt &goto_program)
{
  instanceof_instt inst_switch;
  Forall_goto_program_instructions(target, goto_program)
    lower_instanceof(goto_program, target, inst_switch);
  if(!inst_switch.empty())
  {
    for(auto &p : inst_switch)
    {
      const goto_programt::targett &this_inst=p.first;
      const goto_programt::targett &newinst=p.second;
      this_inst->swap(*newinst);
    }
    goto_program.update();
    return true;
  }
  else
    return false;
}

/*******************************************************************\

Function: remove_instanceoft::lower_instanceof

  Inputs: None

 Outputs: Side-effects on this->goto_functions, replacing every
          instanceof in every function with an explicit test.

 Purpose: See function above

\*******************************************************************/

void remove_instanceoft::lower_instanceof()
{
  bool changed=false;
  for(auto &f : goto_functions.function_map)
    changed=(lower_instanceof(f.second.body) || changed);
  if(changed)
    goto_functions.compute_location_numbers();
}

/*******************************************************************\

Function: remove_instanceof

  Inputs: `goto_functions`, a function map, and the corresponding
          `symbol_table`.

 Outputs: Side-effects on goto_functions, replacing every
          instanceof in every function with an explicit test.
          Extra auxiliary variables may be introduced into
          `symbol_table`.

 Purpose: See function above

\*******************************************************************/

void remove_instanceof(
  symbol_tablet &symbol_table,
  goto_functionst &goto_functions)
{
  remove_instanceoft rem(symbol_table, goto_functions);
  rem.lower_instanceof();
}

void remove_instanceof(goto_modelt &goto_model)
{
  remove_instanceof(
    goto_model.symbol_table, goto_model.goto_functions);
}

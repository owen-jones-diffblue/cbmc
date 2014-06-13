/*******************************************************************\

Module: ANSI-C Conversion

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include <util/std_types.h>
#include <util/config.h>

#include "ansi_c_convert.h"
#include "ansi_c_convert_type.h"
#include "ansi_c_declaration.h"

/*******************************************************************\

Function: ansi_c_convertt::convert

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void ansi_c_convertt::convert(ansi_c_parse_treet &ansi_c_parse_tree)
{
  for(ansi_c_parse_treet::itemst::iterator
      it=ansi_c_parse_tree.items.begin();
      it!=ansi_c_parse_tree.items.end();
      it++)
  {
    assert(it->id()==ID_declaration);
    convert_declaration(*it);
  }
}

/*******************************************************************\

Function: ansi_c_convertt::convert_declaration

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void ansi_c_convertt::convert_declaration(ansi_c_declarationt &declaration)
{
  c_storage_spect c_storage_spec;

  convert_type(declaration.type(), c_storage_spec);

  declaration.set_is_inline(c_storage_spec.is_inline);
  declaration.set_is_static(c_storage_spec.is_static);
  declaration.set_is_extern(c_storage_spec.is_extern);
  declaration.set_is_thread_local(c_storage_spec.is_thread_local);
  declaration.set_is_register(c_storage_spec.is_register);
  declaration.set_is_typedef(c_storage_spec.is_typedef);
  
  // convert the types and values of the declarators
  for(ansi_c_declarationt::declaratorst::iterator
      d_it=declaration.declarators().begin();
      d_it!=declaration.declarators().end();
      d_it++)
  {
    c_storage_spect declarator_storage_spec;
    convert_type(d_it->type(), declarator_storage_spec);
  
    if(d_it->value().is_not_nil())
    {
      if(d_it->value().type().id()==ID_code)
        convert_code(to_code(d_it->value()));
      else
        convert_expr(d_it->value());
    }
  }
}

/*******************************************************************\

Function: ansi_c_convertt::convert_expr

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void ansi_c_convertt::convert_expr(exprt &expr)
{
  if(expr.id()==ID_sideeffect)
  {
    const irep_idt &statement=expr.get(ID_statement);

    if(statement==ID_statement_expression)
    {
      assert(expr.operands().size()==1);
      convert_code(to_code(expr.op0()));
      return;
      // done
    }
  }

  Forall_operands(it, expr)
    convert_expr(*it);

  if(expr.id()==ID_symbol)
  {
    expr.remove(ID_C_id_class);
    expr.remove(ID_C_base_name);
  }
  else if(expr.id()==ID_sizeof)
  {
    if(expr.operands().size()==0)
    {
      typet &type=static_cast<typet &>(expr.add(ID_type_arg));
      convert_type(type);
    }
  }
  else if(expr.id()==ID_designated_initializer)
  {
    exprt &designator=static_cast<exprt &>(expr.add(ID_designator));
    convert_expr(designator);
  }
  else if(expr.id()==ID_alignof)
  {
    if(expr.operands().size()==0)
    {
      typet &type=static_cast<typet &>(expr.add(ID_type_arg));
      convert_type(type);
    }
  }
  else if(expr.id()==ID_gcc_builtin_va_arg)
  {
    convert_type(expr.type());
  }
  else if(expr.id()==ID_generic_selection)
  {
    assert(expr.operands().size()==1);

    irept::subt &generic_associations=
      expr.add(ID_generic_associations).get_sub();

    Forall_irep(it, generic_associations)
    {
      convert_expr(static_cast<exprt &>(it->add(ID_value)));
      convert_type(static_cast<typet &>(it->add(ID_type_arg)));
    }
  }
  else if(expr.id()==ID_gcc_builtin_types_compatible_p)
  {
    typet &type=(typet &)expr;
    assert(type.subtypes().size()==2);
    convert_type(type.subtypes()[0]);
    convert_type(type.subtypes()[1]);
  }
  else if(expr.id()==ID_builtin_offsetof)
  {
    typet &type=static_cast<typet &>(expr.add(ID_type_arg));
    convert_type(type);
    exprt &designator=static_cast<exprt &>(expr.add(ID_designator));
    convert_expr(designator);
  }
  else if(expr.id()==ID_cw_va_arg_typeof)
  {
    typet &type=static_cast<typet &>(expr.add(ID_type_arg));
    convert_type(type);
  }
  else if(expr.id()==ID_typecast)
  {
    convert_type(expr.type());
  }
}

/*******************************************************************\

Function: ansi_c_convertt::convert_code

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void ansi_c_convertt::convert_code(codet &code)
{
  const irep_idt &statement=code.get_statement();

  if(statement==ID_expression)
  {
    assert(code.operands().size()==1);
    convert_expr(code.op0());
  }
  else if(statement==ID_decl)
  {
    // 1 operand, whch is a declaration
    assert(code.operands().size()==1);
    convert_declaration(to_ansi_c_declaration(code.op0()));
  }
  else if(statement==ID_label)
  {
    assert(code.operands().size()==1);
    convert_code(to_code(code.op0()));
  }
  else if(statement==ID_switch_case)
  {
    assert(code.operands().size()==2);
    convert_expr(code.op0());
    convert_code(to_code(code.op1()));
  }
  else if(statement==ID_gcc_switch_case_range)
  {
    assert(code.operands().size()==3);
    convert_expr(code.op0());
    convert_expr(code.op1());
    convert_code(to_code(code.op2()));
  }
  else if(statement==ID_block)
  {
    Forall_operands(it, code)
      convert_code(to_code(*it));
  }
  else if(statement==ID_ifthenelse)
  {
    assert(code.operands().size()==3);

    convert_expr(code.op0());
    convert_code(to_code(code.op1()));

    if(code.op2().is_not_nil())
      convert_code(to_code(code.op2()));
  }
  else if(statement==ID_while ||
          statement==ID_dowhile)
  {
    assert(code.operands().size()==2);

    convert_expr(code.op0());
    convert_code(to_code(code.op1()));
  }
  else if(statement==ID_for)
  {
    assert(code.operands().size()==4);

    if(code.op0().is_not_nil())
      convert_code(to_code(code.op0()));

    if(code.op1().is_not_nil())
      convert_expr(code.op1());

    if(code.op2().is_not_nil())
      convert_expr(code.op2());

    convert_code(to_code(code.op3()));
  }
  else if(statement==ID_msc_try_except)
  {
    assert(code.operands().size()==3);
    convert_code(to_code(code.op0()));
    convert_expr(code.op1());
    convert_code(to_code(code.op2()));
  }
  else if(statement==ID_msc_try_finally)
  {
    assert(code.operands().size()==2);
    convert_code(to_code(code.op0()));
    convert_code(to_code(code.op1()));
  }
  else if(statement==ID_msc_leave)
  {
    assert(code.operands().size()==0);
  }
  else if(statement==ID_switch)
  {
    assert(code.operands().size()==2);

    convert_expr(code.op0());
    convert_code(to_code(code.op1()));
  }
  else if(statement==ID_break)
  {
  }
  else if(statement==ID_goto)
  {
  }
  else if(statement=="computed-goto")
  {
    assert(code.operands().size()==1);

    convert_expr(code.op0());
  }
  else if(statement==ID_continue)
  {
  }
  else if(statement==ID_return)
  {
    if(code.operands().size()==1)
      convert_expr(code.op0());
  }
  else if(statement==ID_static_assert)
  {
    assert(code.operands().size()==2);
    convert_expr(code.op0());
    convert_expr(code.op1());
  }
  else if(statement==ID_decl)
  {
    assert(code.operands().size()==1 ||
           code.operands().size()==2);

    convert_type(code.op0().type());

    if(code.operands().size()==2)
      convert_expr(code.op1());
  }
  else if(statement==ID_skip)
  {
  }
  else if(statement==ID_asm || statement==ID_msc)
  {
  }
  else if(statement==ID_gcc_local_label)
  {
  }
  else if(statement==ID_CPROVER_try_catch ||
          statement==ID_CPROVER_try_finally)
  {
    assert(code.operands().size()==2);
    convert_code(to_code(code.op0()));
    convert_code(to_code(code.op1()));
  }
  else if(statement==ID_CPROVER_throw)
  {
    assert(code.operands().size()==0);
  }
  else
  {
    err_location(code);
    str << "unexpected statement during conversion: " << statement;
    throw 0;
  }
}

/*******************************************************************\

Function: ansi_c_convertt::convert_type

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void ansi_c_convertt::convert_type(typet &type)
{
  c_storage_spect c_storage_spec;
  convert_type(type, c_storage_spec);
}

/*******************************************************************\

Function: ansi_c_convertt::convert_type

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void ansi_c_convertt::convert_type(
  typet &type,
  c_storage_spect &c_storage_spec)
{
  {
    ansi_c_convert_typet ansi_c_convert_type(get_message_handler());

    ansi_c_convert_type.read(type);
    ansi_c_convert_type.write(type);
    c_storage_spec=ansi_c_convert_type.c_storage_spec;
  }

  // do we have alignment?
  if(type.find(ID_C_alignment).is_not_nil())
    convert_expr(static_cast<exprt &>(type.add(ID_C_alignment)));

  if(type.id()==ID_pointer)
  {
    c_storage_spect sub_storage_spec;

    convert_type(type.subtype(), sub_storage_spec);
    c_storage_spec|=sub_storage_spec;
  }
  else if(type.id()==ID_c_bitfield)
  {
    convert_type(type.subtype());
    convert_expr(static_cast<exprt &>(type.add(ID_size)));
  }
  else if(type.id()==ID_symbol)
  {
    type.remove(ID_C_id_class);
    type.remove(ID_C_base_name);
  }
  else if(type.id()==ID_code)
  {
    code_typet &code_type=to_code_type(type);
    
    code_type.remove(ID_subtype);
    code_type.return_type().make_nil();
    
    // take care of parameter declarations
    code_typet::parameterst &parameters=code_type.parameters();

    for(code_typet::parameterst::iterator
        it=parameters.begin();
        it!=parameters.end();
        it++)
    {
      if(it->id()==ID_declaration)
      {
        ansi_c_declarationt &declaration=
          to_ansi_c_declaration(*it);

        assert(declaration.declarators().size()==1);

        convert_type(declaration.type());
        convert_type(declaration.declarator().type());
      }
      else if(it->id()==ID_ellipsis)
      {
      }
      else
        throw "unexpected parameter during conversion: "+it->id_string();
    }
  }
  else if(type.id()==ID_c_enum)
  {
    // take care of enum constant declarations
    exprt &as_expr=static_cast<exprt &>(static_cast<irept &>(type));
    
    Forall_operands(it, as_expr)
    {
      if(it->id()==ID_declaration)
      {
        ansi_c_declarationt &declaration=
          to_ansi_c_declaration(*it);

        assert(declaration.declarators().size()==1);

        convert_type(declaration.type());
        convert_type(declaration.declarator().type());
      }
      else
        throw "unexpected enum constant during conversion: "+it->id_string();
    }
  }
  else if(type.id()==ID_array)
  {
    array_typet &array_type=to_array_type(type);

    c_storage_spect sub_storage_spec;

    convert_type(array_type.subtype(), sub_storage_spec);
    c_storage_spec|=sub_storage_spec;

    if(array_type.size().is_not_nil())
      convert_expr(array_type.size());
  }
  else if(type.id()==ID_custom_unsignedbv ||
          type.id()==ID_custom_signedbv)
  {
    exprt &size=static_cast<exprt &>(type.add(ID_size));
    convert_expr(size);
  }
  else if(type.id()==ID_custom_floatbv)
  {
    exprt &size=static_cast<exprt &>(type.add(ID_size));
    exprt &f=static_cast<exprt &>(type.add(ID_f));
    convert_expr(size);
    convert_expr(f);
  }
  else if(type.id()==ID_custom_fixedbv)
  {
    exprt &size=static_cast<exprt &>(type.add(ID_size));
    exprt &f=static_cast<exprt &>(type.add(ID_f));
    convert_expr(size);
    convert_expr(f);
  }
  else if(type.id()==ID_struct ||
          type.id()==ID_union)
  {
    struct_union_typet::componentst old_components;
    old_components.swap(to_struct_union_type(type).components());
    struct_union_typet::componentst &new_components=
      to_struct_union_type(type).components();

    for(struct_union_typet::componentst::iterator
        it=old_components.begin();
        it!=old_components.end();
        it++)
    {
      // the arguments are member declarations or static assertions
      assert(it->id()==ID_declaration);
      
      ansi_c_declarationt &declaration=
        to_ansi_c_declaration(static_cast<exprt &>(*it));
        
      if(declaration.get_is_static_assert())
      {
        struct_union_typet::componentt new_component;
        new_component.id(ID_static_assert);
        new_component.operands().swap(declaration.operands());
        assert(new_component.operands().size()==2);
        convert_expr(new_component.op0());
        convert_expr(new_component.op1());
        new_components.push_back(new_component);
      }
      else
      {
        for(ansi_c_declarationt::declaratorst::iterator
            d_it=declaration.declarators().begin();
            d_it!=declaration.declarators().end();
            d_it++)
        {
          struct_union_typet::componentt new_component;

          new_component.location()=d_it->location();
          new_component.set(ID_name, d_it->get_base_name());
          new_component.set(ID_pretty_name, d_it->get_base_name());
          new_component.type()=declaration.full_type(*d_it);

          convert_type(new_component.type());

          new_components.push_back(new_component);
        }
      }
    }
  }
  else if(type.id()==ID_typeof)
  {
    exprt &expr=(exprt &)type;
    
    if(!expr.has_operands())
    {
      convert_type(static_cast<typet &>(type.add(ID_type_arg)));
    }
    else
    {
      assert(expr.operands().size()==1);
      convert_expr(expr.op0());
    }
  }
  else if(type.id()==ID_c_enum ||
          type.id()==ID_incomplete_c_enum)
  {
    // add width
    type.set(ID_width, config.ansi_c.int_width);
  }
  else if(type.id()==ID_expression)
  {
    convert_expr((exprt &)(type.subtype()));
  }
  else if(type.id()==ID_vector)
  {
    vector_typet &vector_type=to_vector_type(type);
    convert_expr(vector_type.size());

    c_storage_spect sub_storage_spec;

    convert_type(vector_type.subtype(), sub_storage_spec);
    c_storage_spec|=sub_storage_spec;
  }
  
}

/*******************************************************************\

Function: ansi_c_convert

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool ansi_c_convert(
  ansi_c_parse_treet &ansi_c_parse_tree,
  const std::string &module,
  message_handlert &message_handler)
{
  ansi_c_convertt ansi_c_convert(module, message_handler);

  try
  {
    ansi_c_convert.convert(ansi_c_parse_tree);
  }

  catch(int)
  {
    ansi_c_convert.error();
    return true;
  }

  catch(const char *e)
  {
    ansi_c_convert.error(e);
    return true;
  }

  catch(const std::string &e)
  {
    ansi_c_convert.error(e);
    return true;
  }

  return false;
}

/*******************************************************************\

Function: ansi_c_convert

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool ansi_c_convert(
  exprt &expr,
  const std::string &module,
  message_handlert &message_handler)
{
  ansi_c_convertt ansi_c_convert(module, message_handler);

  try
  {
    ansi_c_convert.convert_expr(expr);
  }

  catch(int)
  {
    ansi_c_convert.error();
  }

  catch(const char *e)
  {
    ansi_c_convert.error(e);
  }

  catch(const std::string &e)
  {
    ansi_c_convert.error(e);
  }

  return ansi_c_convert.get_error_found();
}

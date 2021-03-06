/*******************************************************************\

Module: C++ Language Type Checking

Author: Daniel Kroening, kroening@cs.cmu.edu

\*******************************************************************/

#include <expr_util.h>
#include <i2string.h>
#include <std_types.h>
#include <arith_tools.h>
#include <bitvector.h>
#include <std_expr.h>
#include <config.h>
#include <simplify_expr.h>

#include <ansi-c/c_types.h>
#include <ansi-c/c_qualifiers.h>
#include <ansi-c/c_sizeof.h>

#include "cpp_type2name.h"
#include "cpp_typecheck.h"
#include "cpp_convert_type.h"
#include "cpp_class_type.h"
#include "expr2cpp.h"

/*******************************************************************\

Function: cpp_typecheckt::find_parent

Inputs:

Outputs:

Purpose:

\*******************************************************************/

bool cpp_typecheckt::find_parent(
  const symbolt& symb,
  const irep_idt &base_name,
  irep_idt &identifier)
{
  forall_irep(bit, symb.type.find(ID_bases).get_sub())
  {
    if(lookup(bit->find(ID_type).get(ID_identifier)).base_name == base_name)
    {
      identifier = bit->find(ID_type).get(ID_identifier);
      return true;
    }
  }

  return false;
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_main

Inputs:

Outputs:

Purpose: Called after the operands are done

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_main(exprt &expr)
{
  if(expr.id()==ID_cpp_name)
    typecheck_expr_cpp_name(expr, cpp_typecheck_fargst());
  else if(expr.id()=="cpp-this")
    typecheck_expr_this(expr);
  else if(expr.id()=="pointer-to-member")
    convert_pmop(expr);
  else if(expr.id()=="new_object")
  {
  }
  else if(operator_is_overloaded(expr))
  {
  }
  else if(expr.id()=="explicit-typecast")
    typecheck_expr_explicit_typecast(expr);
  else if(expr.id()=="explicit-constructor-call")
    typecheck_expr_explicit_constructor_call(expr);
  else if(expr.id()==ID_string_constant)
  {
    c_typecheck_baset::typecheck_expr_main(expr);

    // we need to adjust the subtype to 'char'
    assert(expr.type().id()==ID_array);
    expr.type().subtype().set(ID_C_cpp_type, ID_char);
  }
  else if(expr.is_nil())
  {
    std::cerr << "cpp_typecheckt::typecheck_expr_main got nil" << std::endl;
    abort();
  }
  else if(expr.id()==ID_code)
  {
    std::cerr << "cpp_typecheckt::typecheck_expr_main got code" << std::endl;
    abort();
  }
  else if(expr.id()==ID_symbol)
  {
    #if 0
    std::cout << "E: " << expr.pretty() << std::endl;
    std::cerr << "cpp_typecheckt::typecheck_expr_main got symbol" << std::endl;
    abort();
    #endif
  }
  else if(expr.id()=="__is_base_of")
  {
    // an MS extension
    // http://msdn.microsoft.com/en-us/library/ms177194(v=vs.80).aspx
    
    typet base=static_cast<const typet &>(expr.find("type_arg1"));
    typet deriv=static_cast<const typet &>(expr.find("type_arg2"));
    
    typecheck_type(base);
    typecheck_type(deriv);
    
    follow_symbol(base);
    follow_symbol(deriv);

    if(base.id()!=ID_struct || deriv.id()!=ID_struct)
      expr.make_false();
    else
    {
      irep_idt base_name=base.get(ID_name);
      const class_typet &class_type=to_class_type(deriv);

      if(class_type.has_base(base_name))
        expr.make_true();
      else
        expr.make_false();
    }
  }
  else if(expr.id()==ID_msc_uuidof)
  {
    // these appear to have type "struct _GUID"
    // and they are lvalues!
    expr.type()=symbol_typet("c::tag._GUID");
    follow(expr.type());
    expr.set(ID_C_lvalue, true);
  }
  else
    c_typecheck_baset::typecheck_expr_main(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_trinary

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_trinary(if_exprt &expr)
{
  assert(expr.operands().size()==3);

  implicit_typecast(expr.op0(), bool_typet());

  if(expr.op1().type().id()==ID_empty ||
     expr.op1().type().id()==ID_empty)
  {
    if(expr.op1().get_bool(ID_C_lvalue))
    {
      exprt e1(expr.op1());
      if(!standard_conversion_lvalue_to_rvalue(e1, expr.op1()))
      {
        err_location(e1);
        str << "error: lvalue to rvalue conversion";
        throw 0;
      }
    }

    if(expr.op1().type().id()==ID_array)
    {
      exprt e1(expr.op1());
      if(!standard_conversion_array_to_pointer(e1, expr.op1()))
      {
        err_location(e1);
        str << "error: array to pointer conversion";
        throw 0;
      }
    }

    if(expr.op1().type().id()==ID_code)
    {
      exprt e1(expr.op1());
      if(!standard_conversion_function_to_pointer(e1, expr.op1()))
      {
        err_location(e1);
        str << "error: function to pointer conversion";
        throw 0;
      }
    }

    if(expr.op2().get_bool(ID_C_lvalue))
    {
      exprt e2(expr.op2());
      if(!standard_conversion_lvalue_to_rvalue(e2, expr.op2()))
      {
        err_location(e2);
        str << "error: lvalue to rvalue conversion";
        throw 0;
      }
    }

    if(expr.op2().type().id()==ID_array)
    {
      exprt e2(expr.op2());
      if(!standard_conversion_array_to_pointer(e2, expr.op2()))
      {
        err_location(e2);
        str << "error: array to pointer conversion";
        throw 0;
      }
    }

    if(expr.op2().type().id()==ID_code)
    {
      exprt e2(expr.op2());
      if(!standard_conversion_function_to_pointer(e2, expr.op2()))
      {
        err_location(expr);
        str << "error: function to pointer conversion";
        throw 0;
      }
    }

    if(expr.op1().get(ID_statement)==ID_throw &&
       expr.op2().get(ID_statement)!=ID_throw)
      expr.type()=expr.op2().type();
    else if(expr.op2().get(ID_statement)==ID_throw &&
            expr.op1().get(ID_statement)!=ID_throw)
      expr.type()=expr.op1().type();
    else if(expr.op1().type().id()==ID_empty &&
            expr.op2().type().id()==ID_empty)
      expr.type()=empty_typet();
    else
    {
      err_location(expr);
      str << "error: bad types for operands";
      throw 0;
    }
    return;
  }

  if(expr.op1().type() == expr.op2().type())
  {
    c_qualifierst qual1, qual2;
    qual1.read(expr.op1().type());
    qual2.read(expr.op2().type());

    if(qual1.is_subset_of(qual2))
      expr.type() = expr.op1().type();
    else
      expr.type() = expr.op2().type();
  }
  else
  {
    exprt e1 = expr.op1();
    exprt e2 = expr.op2();

    if(implicit_conversion_sequence(expr.op1(), expr.op2().type(), e1))
    {
      expr.type()=e1.type();
      expr.op1().swap(e1);
    }
    else if(implicit_conversion_sequence(expr.op2(),expr.op1().type(), e2))
    {
      expr.type()=e2.type();
      expr.op2().swap(e2);
    }
    else if(expr.op1().type().id()==ID_array &&
	    expr.op2().type().id()==ID_array &&
	    expr.op1().type().subtype() == expr.op2().type().subtype())
    {
      // array-to-pointer conversion
      
      index_exprt index1;
      index1.array() = expr.op1();
      index1.index() = from_integer(0,int_type());
      index1.type() = expr.op1().type().subtype();

      index_exprt index2;
      index2.array() = expr.op2();
      index2.index() = from_integer(0,int_type());
      index2.type() = expr.op2().type().subtype();

      address_of_exprt addr1(index1);
      address_of_exprt addr2(index2);
 
      expr.op1() = addr1;
      expr.op2() = addr2;
      expr.type() = addr1.type();
      return;
    }
    else
    {
      err_location(expr);
      str << "error: types are incompatible.\n"
          << "I got `" << type2cpp(expr.op1().type(), *this)
	  << "' and `" << type2cpp(expr.op2().type(), *this)
	  << "'.";
      throw 0;
    }
  }

  if(expr.op1().get_bool(ID_C_lvalue) &&
     expr.op2().get_bool(ID_C_lvalue))
    expr.set(ID_C_lvalue, true);

  return;
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_member

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_member(exprt &expr)
{
  typecheck_expr_member(
    expr,
    cpp_typecheck_fargst());
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_sizeof

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_sizeof(exprt &expr)
{
  // we need to overload, as "sizeof-type" may be
  // parsed as an expression!

  if(expr.operands().size()==0)
  {
    const typet &type=
      static_cast<const typet &>(expr.find(ID_type_arg));

    if(type.id()==ID_cpp_name)
    {
      // this may be ambiguous -- it can be either a type or
      // an expression

      cpp_typecheck_fargst fargs;

      exprt symbol_expr=resolve(
        to_cpp_name(static_cast<const irept &>(type)),
        cpp_typecheck_resolvet::BOTH,
        fargs);

      if(symbol_expr.id()!=ID_type)
      {
        expr.copy_to_operands(symbol_expr);
        expr.remove(ID_type_arg);
      }
    }
  }

  c_typecheck_baset::typecheck_expr_sizeof(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_ptrmember

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_ptrmember(exprt &expr)
{
  typecheck_expr_ptrmember(expr, cpp_typecheck_fargst());
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_function_expr

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_function_expr(
  exprt &expr,
  const cpp_typecheck_fargst &fargs)
{
  if(expr.id()==ID_cpp_name)
    typecheck_expr_cpp_name(expr, fargs);
  else if(expr.id()==ID_member)
  {
    typecheck_expr_operands(expr);
    typecheck_expr_member(expr, fargs);
  }
  else if(expr.id()==ID_ptrmember)
  {
    typecheck_expr_operands(expr);
    add_implicit_dereference(expr.op0());

    // is operator-> overloaded?
    if(expr.op0().type().id() != ID_pointer)
    {
      std::string op_name="operator->";

      // turn this into a function call
      side_effect_expr_function_callt functioncall;
      functioncall.arguments().reserve(expr.operands().size());
      functioncall.location()=expr.location();

      // first do function/operator
      cpp_namet cpp_name;
      cpp_name.get_sub().push_back(irept(ID_name));
      cpp_name.get_sub().back().set(ID_identifier, op_name);
      cpp_name.get_sub().back().add(ID_C_location)=expr.location();

      functioncall.function()=
        static_cast<const exprt &>(
          static_cast<const irept &>(cpp_name));

      // now do the argument
      functioncall.arguments().push_back(expr.op0());
      typecheck_side_effect_function_call(functioncall);

      exprt tmp("already_typechecked");
      tmp.copy_to_operands(functioncall);
      functioncall.swap(tmp);
      
      expr.op0().swap(functioncall);
      typecheck_function_expr(expr, fargs);
      return;
    }

    typecheck_expr_ptrmember(expr, fargs);
  }
  else
    typecheck_expr(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::overloadable

Inputs:

Outputs:

Purpose:

\*******************************************************************/

bool cpp_typecheckt::overloadable(const exprt &expr)
{
  forall_operands(it, expr)
  {
    typet t(it->type());

    follow_symbol(t);

    if(is_reference(t))
      t=t.subtype();

      if(t.id()==ID_struct ||
         t.id()==ID_union)
      return true;
  }

  return false;
}

/*******************************************************************\

Function: cpp_typecheckt::operator_is_overloaded

Inputs:

Outputs:

Purpose:

\*******************************************************************/

struct operator_entryt
{
  const char *id_name;
  const char *op_name;
} const operators[] =
{
  { "+", "+" },
  { "-", "-" },
  { "*", "*" },
  { "/", "/" },
  { "bitnot", "~" },
  { "bitand", "&" },
  { "bitor", "|" },
  { "bitxor", "^" },
  { "not", "!" },
  { "unary-", "-" },
  { "and", "&&" },
  { "or", "||" },
  { "not", "!" },
  { "index", "[]" },
  { "=", "==" },
  { "<", "<"},
  { "<=", "<="},
  { ">", ">"},
  { ">=", ">="},
  { "shl", "<<"},
  { "shr", ">>"},
  { "notequal", "!=" },
  { "dereference", "*" },
  { "ptrmember", "->" },
  { NULL, NULL }
};

bool cpp_typecheckt::operator_is_overloaded(exprt &expr)
{
  // Check argument types first.
  // At least one struct/enum operand is required.
  
  if(!overloadable(expr))
    return false;
  else if(expr.id()==ID_dereference &&
          expr.get_bool(ID_C_implicit))
    return false;
    
  assert(expr.operands().size()>=1);

  if(expr.id()=="explicit-typecast")
  {
    // the cast operator can be overloaded

    typet t=expr.type();
    typecheck_type(t);
    std::string op_name=std::string("operator")+"("+cpp_type2name(t)+")";

    // turn this into a function call
    side_effect_expr_function_callt functioncall;
    functioncall.arguments().reserve(expr.operands().size());
    functioncall.location()=expr.location();

    cpp_namet cpp_name;
    cpp_name.get_sub().push_back(irept(ID_name));
    cpp_name.get_sub().back().set(ID_identifier, op_name);
    cpp_name.get_sub().back().add(ID_C_location)=expr.location();

    // See if the struct decalares the cast operator as a member
    bool found_in_struct = false;
    assert(!expr.operands().empty());
    typet t0(follow(expr.op0().type()));

    if(t0.id()==ID_struct)
    {
      const struct_typet &struct_type=
        to_struct_type(t0);

      const struct_typet::componentst &components=
        struct_type.components();

      for(struct_typet::componentst::const_iterator
          it=components.begin();
          it!=components.end();
          it++)
      {
        if(!it->get_bool(ID_from_base) &&
           it->get(ID_base_name) == op_name)
        {
          found_in_struct = true;
          break;
        }
      }
    }

    if(!found_in_struct)
      return false;

    {
      exprt member(ID_member);
      member.add("component_cpp_name")= cpp_name;

      exprt tmp("already_typechecked");
      tmp.copy_to_operands(expr.op0());
      member.copy_to_operands(tmp);

      functioncall.function()=member;
    }

    if(expr.operands().size()>1)
    {
      for(exprt::operandst::const_iterator
          it=(expr.operands().begin()+1);
          it!=(expr).operands().end();
          it++)
        functioncall.arguments().push_back(*it);
    }

    typecheck_side_effect_function_call(functioncall);

    if(expr.id()==ID_ptrmember)
    {
      add_implicit_dereference(functioncall);
      exprt tmp("already_typechecked");
      tmp.move_to_operands(functioncall);
      expr.op0().swap(tmp);
      typecheck_expr(expr);
      return true;
    }

    expr.swap(functioncall);
    return true;
  }

  for(const operator_entryt *e=operators;
      e->id_name!=NULL;
      e++)
    if(expr.id()==e->id_name)
    {
      if(expr.id()==ID_dereference)
        assert(!expr.get_bool(ID_C_implicit));

      std::string op_name=std::string("operator")+e->op_name;
      
      // first do function/operator
      cpp_namet cpp_name;
      cpp_name.get_sub().push_back(irept(ID_name));
      cpp_name.get_sub().back().set(ID_identifier, op_name);
      cpp_name.get_sub().back().add(ID_C_location)=expr.location();

      // turn this into a function call
      side_effect_expr_function_callt functioncall;
      functioncall.arguments().reserve(expr.operands().size());
      functioncall.location()=expr.location();

      // There are two options to overload an operator:
      //
      // 1. In the scope of a as a.operator(b, ...)
      // 2. Anywhere in scope as operator(a, b, ...)
      //
      // Both are not allowed.
      //
      // We try and fail silently, maybe conversions will work
      // instead.
  
      // go into scope of first operand        
      if(expr.op0().type().id()==ID_symbol &&
         follow(expr.op0().type()).id()==ID_struct)
      {
        const irep_idt &struct_identifier=
          expr.op0().type().get(ID_identifier);

        // get that scope
        cpp_save_scopet save_scope(cpp_scopes);
        cpp_scopes.set_scope(struct_identifier);
        
        // build fargs for resolver
        cpp_typecheck_fargst fargs;
        fargs.operands=expr.operands();
        fargs.has_object=true;
        fargs.in_use=true;
        
        // should really be a qualified search
        exprt resolve_result=resolve(
          cpp_name, cpp_typecheck_resolvet::VAR, fargs, false);
        
        if(resolve_result.is_not_nil())
        {
          // Found! We turn op(a, b, ...) into a.op(b, ...)
          {
            exprt member(ID_member);
            member.add("component_cpp_name")=cpp_name;

            exprt tmp("already_typechecked");
            tmp.copy_to_operands(expr.op0());
            member.copy_to_operands(tmp);

            functioncall.function()=member;
          }

          if(expr.operands().size()>1)
          {
            // skip first
            for(exprt::operandst::const_iterator
                it=expr.operands().begin()+1;
                it!=expr.operands().end();
                it++)
              functioncall.arguments().push_back(*it);
          }
          
          typecheck_side_effect_function_call(functioncall);
          
          expr=functioncall;

          return true;        
        }
      }

      // 2nd option!
      {
        cpp_typecheck_fargst fargs;
        fargs.operands=expr.operands();
        fargs.has_object=false;
        fargs.in_use=true;
        
        exprt resolve_result=resolve(
             cpp_name, cpp_typecheck_resolvet::VAR, fargs, false);
             
        if(resolve_result.is_not_nil())
        {
          // found!
          functioncall.function()=
            static_cast<const exprt &>(
              static_cast<const irept &>(cpp_name));

          // now do arguments
          forall_operands(it, expr)
            functioncall.arguments().push_back(*it);

          typecheck_side_effect_function_call(functioncall);

          if(expr.id()==ID_ptrmember)
          {
            add_implicit_dereference(functioncall);
            exprt tmp("already_typechecked");
            tmp.move_to_operands(functioncall);
            expr.op0()=tmp;
            typecheck_expr(expr);
            return true;
          }
          
          expr=functioncall;

          return true;
        }
      }

    }

  return false;
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_address_of

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_address_of(exprt &expr)
{
  if(expr.operands().size()!=1)
  {
    err_location(expr);
    throw "address_of expects one operand";
  }

  exprt &op=expr.op0();

  if(!op.get_bool(ID_C_lvalue) && expr.type().id()==ID_code)
  {
    err_location(expr.location());
    str << "expr not an lvalue";
    throw 0;
  }

  if(expr.op0().type().id()==ID_code)
  {
    // we take the address of the method.
    assert(expr.op0().id()==ID_member);
    exprt symb = cpp_symbol_expr(lookup(expr.op0().get(ID_component_name)));
    exprt address(ID_address_of, typet(ID_pointer));
    address.copy_to_operands(symb);
    address.type().subtype()=symb.type();
    address.set(ID_C_implicit, true);
    expr.op0().swap(address);
  }

  if(expr.op0().id()==ID_address_of && 
     expr.op0().get_bool(ID_C_implicit))
  {
    // must be the address of a function
    code_typet& code_type = to_code_type(op.type().subtype());

    code_typet::argumentst& args = code_type.arguments();
    if(args.size() > 0 && args[0].get(ID_C_base_name)==ID_this)
    {
      // it's a pointer to member function
      typet symbol(ID_symbol);
      symbol.set(ID_identifier, code_type.get("#member_name"));
      expr.op0().type().add("to-member") = symbol;

      if(code_type.get_bool("#is_virtual"))
      {
        err_location(expr.location());
        str << "error: pointers to virtual methods"
            << " are currently not implemented";
        throw 0;
      }
    }
  }

  c_typecheck_baset::typecheck_expr_address_of(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_throw

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_throw(exprt &expr)
{
  // these are of type void
  expr.type()=empty_typet();

  assert(expr.operands().size()==1 ||
         expr.operands().size()==0);

  // nothing really to do; one can throw _almost_ anything
  if(expr.operands().size()==1)
  {
    if(follow(expr.op0().type()).id()==ID_empty)
    {
      err_location(expr.op0());
      throw "cannot throw void";
    }
  }
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_new

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_new(exprt &expr)
{
  // next, find out if we do an array

  if(expr.type().id()==ID_array)
  {
    // first typecheck subtype
    typecheck_type(expr.type().subtype());

    // typecheck the size
    exprt &size=to_array_type(expr.type()).size();
    typecheck_expr(size);

    bool size_is_unsigned=(size.type().id()==ID_unsignedbv);
    typet integer_type(size_is_unsigned?ID_unsignedbv:ID_signedbv);
    integer_type.set(ID_width, config.ansi_c.int_width);
    implicit_typecast(size, integer_type);

    expr.set(ID_statement, ID_cpp_new_array);

    // save the size expression
    expr.set(ID_size, to_array_type(expr.type()).size());

    // new actually returns a pointer, not an array
    pointer_typet ptr_type;
    ptr_type.subtype()=expr.type().subtype();
    expr.type().swap(ptr_type);
  }
  else
  {
    // first typecheck type
    typecheck_type(expr.type());

    expr.set(ID_statement, ID_cpp_new);

    pointer_typet ptr_type;
    ptr_type.subtype().swap(expr.type());
    expr.type().swap(ptr_type);
  }

  exprt object_expr("new_object", expr.type().subtype());
  object_expr.set(ID_C_lvalue, true);

  {
    exprt tmp("already_typechecked");
    tmp.move_to_operands(object_expr);
    object_expr.swap(tmp);
  }

  // not yet typechecked-stuff
  exprt &initializer=static_cast<exprt &>(expr.add(ID_initializer));
  
  // arrays must not have an initializer
  if(!initializer.operands().empty() &&
     expr.get(ID_statement)==ID_cpp_new_array)
  {
    err_location(expr.op0());
    str << "new with array type must not use initializer";
    throw 0;
  }

  exprt code=
    cpp_constructor(
      expr.find_location(),
      object_expr,
      initializer.operands());

  expr.add(ID_initializer).swap(code);
  
  // we add the size of the object for convenience of the
  // runtime library

  exprt &sizeof_expr=static_cast<exprt &>(expr.add(ID_sizeof));
  sizeof_expr=c_sizeof(expr.type().subtype(), *this);
  sizeof_expr.add("#c_sizeof_type")=expr.type().subtype();
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_explicit_typecast

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_explicit_typecast(exprt &expr)
{
  typecheck_type(expr.type());
    
  // these can have 0 or 1 arguments

  if(expr.operands().size()==0)
  {
    // default value
    exprt new_expr=gen_zero(expr.type());
    
    if(new_expr.is_nil())
    {
      err_location(expr);
      str << "no default value for `" << to_string(expr.type())
          << "'";
      throw 0;
    }

    new_expr.location()=expr.location();
    expr=new_expr;
  }
  else if(expr.operands().size()==1)
  {
    // explicitly given value
    exprt new_expr;

    if(const_typecast(expr.op0(), expr.type(), new_expr) ||
       static_typecast(expr.op0(), expr.type(), new_expr, false) ||
       reinterpret_typecast(expr.op0(), expr.type(), new_expr, false))
    {
      expr=new_expr;
    }
    else
    {
      err_location(expr);
      str << "invalid explicit cast:" << std::endl;
      str << "operand type: `" << to_string(expr.op0().type()) << "'" << std::endl;
      str << "casting to: `" << to_string(expr.type()) << "'";
      throw 0;
    }
  }
  else
    throw "explicit typecast expects 0 or 1 operands";
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_explicit_constructor_call

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_explicit_constructor_call(exprt &expr)
{
  typecheck_type(expr.type());

  if(cpp_is_pod(expr.type()))
  {
    expr.id("explicit-typecast");
    typecheck_expr_main(expr);
  }
  else
  {
    assert(expr.type().id()==ID_struct);

    typet symb(ID_symbol);
    symb.set(ID_identifier, expr.type().get(ID_name));
    symb.location()=expr.location();
    
    exprt e=expr;
    new_temporary(e.location(), symb, e.operands(), expr);
  }
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_this

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_this(exprt &expr)
{
  if(cpp_scopes.current_scope().class_identifier.empty())
  {
    err_location(expr);
    error("`this' is not allowed here");
    throw 0;
  }

  const exprt &this_expr=cpp_scopes.current_scope().this_expr;
  const locationt location=expr.find_location();

  assert(this_expr.is_not_nil());
  assert(this_expr.type().id()==ID_pointer);

  expr=this_expr;
  expr.location()=location;
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_delete

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_delete(exprt &expr)
{
  if(expr.operands().size()!=1)
    throw "delete expects one operand";

  if(expr.get(ID_statement)==ID_cpp_delete)
  {
  }
  else if(expr.get(ID_statement)==ID_cpp_delete_array)
  {
  }
  else
    assert(false);
    
  typet pointer_type=follow(expr.op0().type());

  if(pointer_type.id()!=ID_pointer)
  {
    err_location(expr);
    str << "delete takes a pointer type operand, but got `"
        << to_string(pointer_type) << "'";
    throw 0;
  }

  // these are always void
  expr.type()=typet(ID_empty);
  
  // we provide the right destructor, for the convenience
  // of later stages
  exprt new_object(ID_new_object, pointer_type.subtype());
  new_object.location()=expr.location();
  new_object.set(ID_C_lvalue, true);
  
  already_typechecked(new_object);

  codet destructor_code=cpp_destructor(
    expr.location(),
    pointer_type.subtype(),
    new_object);
    
  // this isn't typechecked yet
  if(destructor_code.is_not_nil())
    typecheck_code(destructor_code);
    
  expr.set(ID_destructor, destructor_code);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_typecast

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_typecast(exprt &expr)
{
  // should not be called
  #if 0
  std::cout << "E: " << expr.pretty() << std::endl;
  assert(0);
  #endif
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_member

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_member(
  exprt &expr,
  const cpp_typecheck_fargst &fargs)
{
  if(expr.operands().size()!=1)
  {
    err_location(expr);
    str << "error: member operator expects one operand";
    throw 0;
  }

  exprt &op0=expr.op0();
  add_implicit_dereference(op0);

  #ifdef CPP_SYSTEMC_EXTENSION
  if(expr.op0().type().id()==ID_signedbv ||
     expr.op0().type().id()==ID_unsignedbv ||
     expr.op0().type().id()==ID_verilogbv)
  {
    // might be a SystemC expression
    typecheck_expr_sc_member(expr, fargs);
    return;
  }
  #endif

  if(op0.type().id()!=ID_symbol)
  {
    err_location(expr);
    str << "error: member operator requires type symbol "
           "on left hand side but got `"
        << to_string(op0.type()) << "'";
    throw 0;
  }

  const irep_idt &struct_identifier=
    to_symbol_type(op0.type()).get_identifier();

  const symbolt &struct_symbol=lookup(struct_identifier);

  if(struct_symbol.type.id()==ID_incomplete_struct ||
     struct_symbol.type.id()==ID_incomplete_union)
  {
    err_location(expr);
    str << "error: member operator got incomplete type "
           "on left hand side";
    throw 0;
  }

  if(struct_symbol.type.id()!=ID_struct &&
     struct_symbol.type.id()!=ID_union)
  {
    err_location(expr);
    str << "error: member operator requires struct/union type "
           "on left hand side but got `"
        << to_string(struct_symbol.type) << "'";
    throw 0;
  }

  const struct_union_typet &type=
    to_struct_union_type(struct_symbol.type);

  if(expr.find("component_cpp_name").is_not_nil())
  {
    cpp_namet component_cpp_name=
      to_cpp_name(expr.find("component_cpp_name"));

    // go to the scope of the struct/union
    cpp_save_scopet save_scope(cpp_scopes);
    cpp_scopes.set_scope(struct_identifier);

    // resolve the member name in this scope
    cpp_typecheck_fargst new_fargs(fargs);
    new_fargs.add_object(op0);

    exprt symbol_expr=resolve(
                        component_cpp_name,
                        cpp_typecheck_resolvet::VAR,
                        new_fargs);

    if(symbol_expr.id()==ID_dereference)
    {
      assert(symbol_expr.get_bool(ID_C_implicit));
      exprt tmp=symbol_expr.op0();
      symbol_expr.swap(tmp);
    }

    assert(symbol_expr.id()==ID_symbol ||
           symbol_expr.id()==ID_member ||
           symbol_expr.id()==ID_constant);

    // If it is a symbol or a constant, just return it!
    // note: the resolver returns a symbol if the member
    // is static or if it is a constructor

    if(symbol_expr.id()==ID_symbol)
    {
      if(symbol_expr.type().id()==ID_code &&
         symbol_expr.type().get(ID_return_type)==ID_constructor)
      {
        err_location(expr);
        str << "error: member `" 
            << lookup(symbol_expr.get(ID_identifier)).base_name
            << "' is a constructor";
        throw 0;
      }
      else
      {
        // it must be a static component
        const struct_typet::componentt pcomp=
          type.get_component(to_symbol_expr(symbol_expr).get_identifier());

        if(pcomp.is_nil())
        {
          err_location(expr);
          str << "error: `"
              << symbol_expr.get(ID_identifier)
              << "' is not static member "
              << "of class `" << struct_symbol.base_name << "'";
          throw 0;
        }
      }

      expr=symbol_expr;
      return;
    }
    else if(symbol_expr.id()==ID_constant)
    {
      expr=symbol_expr;
      return;
    }

    const irep_idt component_name=symbol_expr.get(ID_component_name);

    expr.remove("component_cpp_name");
    expr.set(ID_component_name, component_name);
  }

  const irep_idt &component_name=expr.get(ID_component_name);

  assert(component_name!="");

  exprt component;
  component.make_nil();

  assert(follow(expr.op0().type()).id()==ID_struct ||
         follow(expr.op0().type()).id()==ID_union);

  exprt member;

  if(get_component(expr.location(),
                   expr.op0(),
                   component_name,
                   member))
  {
    // because of possible anonymous members
    expr.swap(member);
  }
  else
  {
    err_location(expr);
    str << "error: member `" << component_name
        << "' of `" << struct_symbol.pretty_name
        << "' not found";
    throw 0;
  }

  add_implicit_dereference(expr);

  if(expr.type().id()==ID_code)
  {
    // Check if the function body has to be typechecked
    contextt::symbolst::iterator it=
      context.symbols.find(component_name);

    assert(it!=context.symbols.end());

    symbolt &func_symb=it->second;

    if(func_symb.value.id()=="cpp_not_typechecked")
      func_symb.value.set("is_used", true);
  }
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_ptrmember

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_ptrmember(
  exprt &expr,
  const cpp_typecheck_fargst &fargs)
{
  assert(expr.id()==ID_ptrmember);

  if(expr.operands().size()!=1)
  {
    err_location(expr);
    str << "error: ptrmember operator expects one operand";
    throw 0;
  }

  add_implicit_dereference(expr.op0());

  if(expr.op0().type().id()!=ID_pointer)
  {
    err_location(expr);
    str << "error: ptrmember operator requires pointer type "
           "on left hand side, but got `"
        << to_string(expr.op0().type()) << "'";
    throw 0;
  }

  exprt tmp;
  exprt &op=expr.op0();

  op.swap(tmp);

  op.id(ID_dereference);
  op.move_to_operands(tmp);
  op.set(ID_C_location, expr.find(ID_C_location));
  typecheck_expr_dereference(op);

  expr.id(ID_member);
  typecheck_expr_member(expr, fargs);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_cast_expr

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_cast_expr(exprt &expr)
{
  side_effect_expr_function_callt e =
    to_side_effect_expr_function_call(expr);

  if(e.arguments().size() != 1)
  {
    err_location(expr);
    throw "cast expressions expect one operand";
  }

  exprt &f_op=e.function();
  exprt &cast_op=e.arguments().front();

  add_implicit_dereference(cast_op);

  const irep_idt &id=
  f_op.get_sub().front().get(ID_identifier);

  if(f_op.get_sub().size()!=2 ||
     f_op.get_sub()[1].id()!=ID_template_args)
  {
    err_location(expr);
    str << id << " expects template argument";
    throw 0;
  }

  irept &template_arguments=f_op.get_sub()[1].add(ID_arguments);

  if(template_arguments.get_sub().size()!=1)
  {
    err_location(expr);
    str << id << " expects one template argument";
    throw 0;
  }

  irept &template_arg=template_arguments.get_sub().front();
  
  if(template_arg.id()!=ID_type &&
     template_arg.id()!="ambiguous")
  {
    err_location(expr);
    str << id << " expects a type as template argument";
    throw 0;
  }

  typet &type=static_cast<typet &>(
    template_arguments.get_sub().front().add(ID_type));

  typecheck_type(type);

  locationt location=expr.location();

  exprt new_expr;
  if(id==ID_const_cast)
  {
    if(!const_typecast(cast_op, type, new_expr))
    {
      err_location(cast_op);
      str << "type mismatch on const_cast:" << std::endl;
      str << "operand type: `" << to_string(cast_op.type()) << "'" << std::endl;
      str << "cast type: `" << to_string(type) << "'";
      throw 0;
    }
  }
  else if(id==ID_dynamic_cast)
  {
    if(!dynamic_typecast(cast_op, type, new_expr))
    {
      err_location(cast_op);
      str << "type mismatch on dynamic_cast:" << std::endl;
      str << "operand type: `" << to_string(cast_op.type()) << "'" << std::endl;
      str << "cast type: `" << to_string(type) << "'";
      throw 0;
    }
  }
  else if(id==ID_reinterpret_cast)
  {
    if(!reinterpret_typecast(cast_op, type, new_expr))
    {
      err_location(cast_op);
      str << "type mismatch on reinterpret_cast:" << std::endl;
      str << "operand type: `" << to_string(cast_op.type()) << "'" << std::endl;
      str << "cast type: `" << to_string(type) << "'";
      throw 0;
    }
  }
  else if(id==ID_static_cast)
  {
    if(!static_typecast(cast_op, type, new_expr))
    {
      err_location(cast_op);
      str << "type mismatch on static_cast:" << std::endl;
      str << "operand type: `" << to_string(cast_op.type()) << "'" << std::endl;
      str << "cast type: `" << to_string(type) << "'";
      throw 0;
    }
  }
  else
    assert(false);

  expr.swap(new_expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_cpp_name

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_cpp_name(
  exprt &expr,
  const cpp_typecheck_fargst &fargs)
{
  locationt location=
    to_cpp_name(expr).location();

  for(unsigned i=0; i<expr.get_sub().size(); i++)
  {
    if(expr.get_sub()[i].id()==ID_cpp_name)
    {
      typet &type=static_cast<typet &>(expr.get_sub()[i]);
      typecheck_type(type);

      std::string tmp="("+cpp_type2name(type)+")";

      typet name(ID_name);
      name.set(ID_identifier, tmp);
      name.location()=location;
      
      type=name;
    }
  }

  if(expr.get_sub().size()>=1 &&
     expr.get_sub().front().id()==ID_name)
  {
    const irep_idt &id=expr.get_sub().front().get(ID_identifier);

    if(id==ID_const_cast ||
       id==ID_dynamic_cast ||
       id==ID_reinterpret_cast ||
       id==ID_static_cast)
    {
      expr.id("cast_expression");
      return;
    }
  }

  exprt symbol_expr=
    resolve(
      to_cpp_name(expr),
      cpp_typecheck_resolvet::VAR,
      fargs);

  // we want VAR
  assert(symbol_expr.id()!=ID_type);

  if(symbol_expr.id()==ID_member)
  {
    if(symbol_expr.operands().empty() ||
       symbol_expr.op0().is_nil())
    {
      if(symbol_expr.type().get(ID_return_type)!=ID_constructor)
      {
        if(cpp_scopes.current_scope().this_expr.is_nil())
        {
          if(symbol_expr.type().id()!=ID_code)
          {
            err_location(location);
            str << "object missing";
            throw 0;
          }

          // may still be good for address of
        }
        else
        {
          // Try again
          exprt ptrmem(ID_ptrmember);
          ptrmem.operands().push_back(
            cpp_scopes.current_scope().this_expr);

          ptrmem.add("component_cpp_name") = expr;

          ptrmem.location()=location;
          typecheck_expr_ptrmember(ptrmem, fargs);
          symbol_expr.swap(ptrmem);
        }
      }
    }
  }

  symbol_expr.location()=location;
  expr=symbol_expr;

  if(expr.id()==ID_symbol)
    typecheck_expr_function_identifier(expr);

  add_implicit_dereference(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::add_implicit_dereference

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::add_implicit_dereference(exprt &expr)
{
  if(is_reference(expr.type()))
  {
    // add implicit dereference
    exprt tmp(ID_dereference, expr.type().subtype());
    tmp.set(ID_C_implicit, true);
    tmp.location()=expr.location();
    tmp.move_to_operands(expr);
    tmp.set(ID_C_lvalue, true);
    expr.swap(tmp);
  }
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_side_effect_function_call

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_side_effect_function_call(
  side_effect_expr_function_callt &expr)
{
  // For virtual functions, it is important to check whether
  // the function name is qualified. If it is qualified, then
  // the call is not virtual.
  bool is_qualified = false;

  if(expr.function().id()==ID_member ||
     expr.function().id()==ID_ptrmember)
  {
    if(expr.function().get("component_cpp_name")==ID_cpp_name)
    {
      const cpp_namet &cpp_name=
        to_cpp_name(expr.function().find("component_cpp_name"));
      is_qualified=cpp_name.is_qualified();
    }
  }
  else if(expr.function().id()==ID_cpp_name)
  {
    const cpp_namet &cpp_name=to_cpp_name(expr.function());
    is_qualified=cpp_name.is_qualified();
  }

  // Backup of the original operand
  exprt op0=expr.function();

  // now do the function -- this has been postponed
  typecheck_function_expr(expr.function(), cpp_typecheck_fargst(expr));
  
  if(expr.function().id()=="pod_constructor")
  {
    assert(expr.function().type().id()==ID_code);

    // This must be a POD.
    const typet &pod=to_code_type(expr.function().type()).return_type();
    assert(cpp_is_pod(pod));

    // These aren't really function calls, but either conversions or
    // initializations.    
    if(expr.arguments().size()==0)
    {
      // create temporary object
      exprt tmp_object_expr(ID_sideeffect, pod);
      tmp_object_expr.set(ID_statement, ID_temporary_object);
      tmp_object_expr.set(ID_C_lvalue, true);
      tmp_object_expr.set(ID_mode, current_mode);
      tmp_object_expr.location()=expr.location();
      expr.swap(tmp_object_expr);
    }
    else if(expr.arguments().size()==1)
    {
      exprt typecast("explicit-typecast");
      typecast.type()=pod;
      typecast.location() = expr.location();
      typecast.copy_to_operands(expr.arguments().front());
      typecheck_expr_explicit_typecast(typecast);
      expr.swap(typecast);
    }
    else
    {
      err_location(expr.location());
      str << "zero or one argument excpected\n";
      throw 0;
    }

    return;
  }

  #ifdef CPP_SYSTEMC_EXTENSION
  if(expr.function().id() == "sc_extension")
  {
    exprt tmp = expr.function().op0();
    expr.swap(tmp);
    return;
  }
  #endif
  
  if(expr.function().id()=="cast_expression")
  {
    // These are not really function calls,
    // but usually just type adjustments.
    typecheck_cast_expr(expr);
    add_implicit_dereference(expr);
    return;
  }

  follow_symbol(expr.function().type());
  
  if(expr.function().type().id()==ID_pointer)
  {
    if(expr.function().type().find("to-member").is_not_nil())
    {
      const exprt &bound=
        static_cast<const exprt &>(expr.function().type().find("#bound"));

      if(bound.is_nil())
      {
        err_location(expr.location());
        str << "pointer-to-member not bound";
        throw 0;
      }

      // add `this'
      assert(bound.type().id()==ID_pointer);
      expr.arguments().insert(expr.arguments().begin(), bound);

      // we don't need the object anymore
      expr.function().type().remove("#bound");
    }

    // do implicit dereference
    if((expr.function().id()=="implicit_address_of" ||
        expr.function().id()==ID_address_of) &&
      expr.function().operands().size()==1)
    {
      exprt tmp;
      tmp.swap(expr.function().op0());
      expr.function().swap(tmp);
    }
    else
    {
      assert(expr.function().type().id()==ID_pointer);
      exprt tmp(ID_dereference, expr.function().type().subtype());
      tmp.location()=expr.op0().location();
      tmp.move_to_operands(expr.function());
      expr.function().swap(tmp);
    }

    if(expr.function().type().id()!=ID_code)
    {
      err_location(expr.op0());
      throw "expecting code as argument";
    }
  }
  else if(expr.function().type().id()==ID_code)
  {
    if(expr.function().type().get_bool("#is_virtual") &&
       !is_qualified)
    {
      exprt vtptr_member;
      if(op0.id()==ID_member || op0.id()==ID_ptrmember)
      {
        vtptr_member.id(op0.id());
        vtptr_member.move_to_operands(op0.op0());
      }
      else
      {
        vtptr_member.id(ID_ptrmember);
        exprt this_expr("cpp-this");
        vtptr_member.move_to_operands(this_expr);
      }

      // get the virtual table
      typet this_type =  to_code_type(expr.function().type()).arguments().front().type();
      irep_idt vtable_name = this_type.subtype().get(ID_identifier).as_string() +"::@vtable_pointer";

      const struct_typet &vt_struct=
        to_struct_type(follow(this_type.subtype()));

      const struct_typet::componentt &vt_compo=
        vt_struct.get_component(vtable_name);

      assert(vt_compo.is_not_nil());

      vtptr_member.set(ID_component_name, vtable_name);

      // look for the right entry
      irep_idt vtentry_component_name = vt_compo.type().subtype().get(ID_identifier).as_string()
        + "::" + expr.function().type().get("#virtual_name").as_string();

      exprt vtentry_member(ID_ptrmember);
      vtentry_member.copy_to_operands(vtptr_member);
      vtentry_member.set(ID_component_name, vtentry_component_name );
      typecheck_expr(vtentry_member);

      assert(vtentry_member.type().id()==ID_pointer);

      {
        exprt tmp(ID_dereference, vtentry_member.type().subtype());
        tmp.location()=expr.op0().location();
        tmp.move_to_operands(vtentry_member);
        vtentry_member.swap(tmp);
      }

      // Typcheck the expresssion as if it was not virtual
      // (add the this pointer)

      expr.type()=
        to_code_type(expr.function().type()).return_type();

      typecheck_method_application(expr);

      // Let's make the call virtual
      expr.function().swap(vtentry_member);

      typecheck_function_call_arguments(expr);
      add_implicit_dereference(expr);
      return;
    }
  }
  else if(expr.function().type().id()==ID_struct)
  {
    irept name(ID_name);
    name.set(ID_identifier, "operator()");
    name.set(ID_C_location, expr.location());

    cpp_namet cppname;
    cppname.get_sub().push_back(name);

    exprt member(ID_member);
    member.add("component_cpp_name") = cppname;

    member.move_to_operands(op0);

    expr.function().swap(member);
    typecheck_side_effect_function_call(expr);

    return;
  }
  else
  {
    err_location(expr.function());
    str << "function call expects function/function "
    << "pointer as argument, but got `"
    << to_string(expr.op0().type()) << "'";
    throw 0;
  }

  expr.type()=
    to_code_type(expr.function().type()).return_type();

  if(expr.type().id()==ID_constructor)
  {
    assert(expr.function().id() == ID_symbol);

    const code_typet::argumentst &arguments=
      to_code_type(expr.function().type()).arguments();

    assert(arguments.size()>=1);

    const typet &this_type=arguments[0].type();

    // change type from 'constructor' to object type
    expr.type() = this_type.subtype();

    // create temporary object
    exprt tmp_object_expr(ID_sideeffect, this_type.subtype());
    tmp_object_expr.set(ID_statement, ID_temporary_object);
    tmp_object_expr.set(ID_C_lvalue, true);
    tmp_object_expr.set(ID_mode, current_mode);
    tmp_object_expr.location()=expr.location();

    exprt member;

    exprt new_object("new_object", tmp_object_expr.type());
    new_object.set(ID_C_lvalue, true);
    
    assert(follow(tmp_object_expr.type()).id()==ID_struct);

    get_component(expr.location(),
                  new_object,
                  expr.function().get(ID_identifier),
                  member);

    // special case for the initialization of parents
    if(member.get_bool("#not_accessible"))
    {
      assert(id2string(member.get(ID_C_access))!="");
      tmp_object_expr.set("#not_accessible", true);
      tmp_object_expr.set(ID_C_access, member.get(ID_C_access));
    }

    expr.function().swap(member);
    
    typecheck_method_application(expr);
    typecheck_function_call_arguments(expr);

    codet new_code(ID_expression);
    new_code.copy_to_operands(expr);

    tmp_object_expr.add(ID_initializer) = new_code;
    expr.swap(tmp_object_expr);
    return;
  }

  assert(expr.operands().size()==2);

  if(expr.function().id()==ID_member)
    typecheck_method_application(expr);
  else
  {
    // for the object of a method call,
    // we are willing to add an "address_of"
    // for the sake of operator overloading

    const irept::subt &arguments=
      expr.function().type().find(ID_arguments).get_sub();

    if(arguments.size()>=1 &&
       arguments.front().get(ID_C_base_name)==ID_this &&
       expr.arguments().size()>=1)
    {
      const exprt &argument=
      static_cast<const exprt &>(arguments.front());

      exprt &operand=expr.op1();
      assert(argument.type().id()==ID_pointer);

      if(operand.type().id()!=ID_pointer &&
         operand.type()==argument.type().subtype())
      {
        exprt tmp(ID_address_of, typet(ID_pointer));
        tmp.type().subtype()=operand.type();
        tmp.location()=operand.location();
        tmp.move_to_operands(operand);
        operand.swap(tmp);
      }
    }
  }

  assert(expr.operands().size()==2);

  typecheck_function_call_arguments(expr);

  assert(expr.operands().size()==2);
  
  add_implicit_dereference(expr);

  // we will deal with some 'special' functions here
  do_special_functions(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_function_call_arguments

  Inputs: type-checked arguments, type-checked function

 Outputs: type-adjusted function arguments

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_function_call_arguments(
  side_effect_expr_function_callt &expr)
{
  exprt &f_op=expr.function();
  const code_typet &code_type=to_code_type(f_op.type());
  const code_typet::argumentst &arguments=code_type.arguments();

  // do default arguments

  if(arguments.size()>expr.arguments().size())
  {
    unsigned i=expr.arguments().size();

    for(; i<arguments.size(); i++)
    {
      if(!arguments[i].has_default_value())
        break;

      const exprt &value=arguments[i].default_value();
      expr.arguments().push_back(value);
    }
  }

  for(unsigned i=0; i<arguments.size(); i++)
  {
    if(arguments[i].get_bool("#call_by_value"))
    {
      assert(is_reference(arguments[i].type()));

      if(expr.arguments()[i].id()!=ID_temporary_object)
      {
        // create a temporary for the parameter

        exprt arg("already_typechecked");
        arg.copy_to_operands(expr.arguments()[i]);

        exprt temporary;
        new_temporary(expr.arguments()[i].location(),
                      arguments[i].type().subtype(),
                      arg,
                      temporary);
        expr.arguments()[i].swap(temporary);
      }

    }
  }
  
  c_typecheck_baset::typecheck_function_call_arguments(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_side_effect

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_side_effect(
 side_effect_exprt &expr)
{
  const irep_idt &statement=expr.get(ID_statement);

  if(statement==ID_cpp_new ||
     statement==ID_cpp_new_array)
  {
    typecheck_expr_new(expr);
  }
  else if(statement==ID_cpp_delete ||
          statement==ID_cpp_delete_array)
  {
    typecheck_expr_delete(expr);
  }
  else if(statement==ID_preincrement ||
          statement==ID_predecrement ||
          statement==ID_postincrement ||
          statement==ID_postdecrement)
  {
    typecheck_side_effect_inc_dec(expr);
  }
  else if(statement==ID_throw)
  {
    typecheck_expr_throw(expr);
  }
  else
    c_typecheck_baset::typecheck_expr_side_effect(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_method_application

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_method_application(
  side_effect_expr_function_callt &expr)
{
  assert(expr.operands().size()==2);

  assert(expr.function().id()==ID_member);
  assert(expr.function().operands().size()==1);

  // turn e.f(...) into xx::f(e, ...)

  exprt member_expr;
  member_expr.swap(expr.function());

  const symbolt &symbol=lookup(member_expr.get(ID_component_name));

  // build new function expression
  exprt new_function(cpp_symbol_expr(symbol));
  new_function.location()=member_expr.location();
  expr.function().swap(new_function);

  if(!expr.function().type().get_bool("#is_static"))
  {
    const code_typet &func_type=to_code_type(symbol.type);
    typet this_type=func_type.arguments().front().type();

    // Special case. Make it a reference.
    assert(this_type.id()==ID_pointer);
    this_type.set(ID_C_reference, true);
    this_type.set("#this", true);
    
    if(expr.arguments().size()==func_type.arguments().size())
    {
      implicit_typecast(expr.arguments().front(), this_type);
      assert(is_reference(expr.arguments().front().type()));
      expr.arguments().front().type().remove(ID_C_reference);
    }
    else
    {
      exprt this_arg = member_expr.op0();
      implicit_typecast(this_arg, this_type);
      assert(is_reference(this_arg.type()));
      this_arg.type().remove(ID_C_reference);
      expr.arguments().insert(expr.arguments().begin(), this_arg);
    }
  }

  if(symbol.value.id()=="cpp_not_typechecked" &&
      !symbol.value.get_bool("is_used"))
  {
    context.symbols[symbol.name].value.set("is_used", true);
  }
}

/*******************************************************************\

Function: cpp_typecheckt::function_call_add_this

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::function_call_add_this(
  side_effect_expr_function_callt &expr)
{
  /*  // add "this" to argument list
  assert(expr.operands().size()==2);

  typet this_type = to_code_type(lookup(expr.function().type())
                                 .arguments().front().type();

  // special case. make it reference
  assert(this_type.id()=="pointer");
  this_type.set("#refrence",true);
  this_type.set("#this",true);

  exprt new_expr
  implicit_typecast()

  // insert operand
  expr.arguments().insert(expr.arguments().begin(), exprt());

  exprt &member_expr=expr.function();
  exprt &this_argument_expr=expr.arguments().front();

  assert(member_expr.operands().size()==1);

  exprt &struct_expr=member_expr.op0();

  if(struct_expr.id()==ID_dereference &&
     struct_expr.operands().size()==1 &&
   struct_expr.op0().type().id()==ID_pointer)
  {
    this_argument_expr=struct_expr.op0();
    if(this_argument_expr.type().get_bool("#reference"))
      this_argument_expr.type().remove("#reference");
  }
  else
  {
    this_argument_expr=exprt(ID_address_of, typet(ID_pointer));
    this_argument_expr.location()=expr.location();
    this_argument_expr.type().subtype()=member_expr.op0().type();
    this_argument_expr.copy_to_operands(member_expr.op0());
  }*/
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_side_effect_assignment

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_side_effect_assignment(exprt &expr)
{
  if(expr.operands().size()!=2)
    throw "assignment side-effect expected to have two operands";
    
  typet type0=expr.op0().type();

  if(is_reference(type0))
    type0=type0.subtype();

  if(cpp_is_pod(type0))
  {
    #ifdef CPP_SYSTEMC_EXTENSION
    if(expr.op0().id()==ID_extractbits)
    {
       if(!expr.op0().op0().get_bool(ID_C_lvalue))
       {
         err_location(expr.op0().op0());
         str << "assignment error: `" << to_string(expr.op0().op0())
             << "' not an lvalue";
         throw 0;
       }

      expr.op0().set(ID_C_lvalue, true);
      c_typecheck_baset::typecheck_side_effect_assignment(expr);
      expr.op0().remove(ID_C_lvalue);

      const exprt& extractbits = expr.op0();

      int width = atoi(extractbits.op0().type().get(ID_width).c_str());
      int left  = atoi(extractbits.op1().get(ID_C_cformat).c_str());
      int right = atoi(extractbits.op2().get(ID_C_cformat).c_str());

      std::string mask;

      for(int i = 0; i < right; i++)
        mask = "1" + mask;

      for(int i = right; i <= left ; i++)
        mask = "0" + mask;

      for(int i = left+1; i < width; i++)
        mask = "1" + mask;

       // `a.range(left,right) = b' is converted to
       //  a =  (mask & a) |  ((typecast)b) << right

      exprt expr_mask(ID_constant, extractbits.op0().type());
      expr_mask.set(ID_value, mask);

      exprt and_expr(ID_bitand, expr_mask.type());
      and_expr.copy_to_operands(expr_mask, extractbits.op0());

      exprt expr_typecast(expr.op1());
      expr_typecast.make_typecast(extractbits.op0().type());

      exprt shl(ID_shl, expr_typecast.type());
      shl.copy_to_operands(expr_typecast, extractbits.op2());

      exprt or_expr(ID_bitor, and_expr.type());
      or_expr.copy_to_operands(and_expr, shl);

      simplify(or_expr, *this);

      codet assign(expr.get(ID_statement));
      assign.type() = extractbits.op0().type();
      assign.copy_to_operands(extractbits.op0(), or_expr);
      assign.location() = expr.location();

      code_expressiont code_expr;
      code_expr.type() = extractbits.type();
      code_expr.expression()=extractbits;

      code_blockt block;
      block.location() = expr.location();
      block.copy_to_operands(assign, code_expr);

      side_effect_exprt statement_expr(ID_statement_expression);
      statement_expr.type() = code_expr.type();
      statement_expr.copy_to_operands(block);

      expr = statement_expr;
      return;
    }
    else if(expr.op0().id()==ID_extractbit)
    {
      if(!expr.op0().op0().get_bool(ID_C_lvalue))
      {
        err_location(expr.op0().op0());
        str << "assignment error: `" << to_string(expr.op0().op0())
            << "' not an lvalue";
        throw 0;
      }

      expr.op0().set(ID_C_lvalue, true);
      c_typecheck_baset::typecheck_side_effect_assignment(expr);
      expr.op0().remove(ID_C_lvalue);

      const exprt& extractbit = expr.op0();

      int width = atoi(extractbit.op0().type().get(ID_width).c_str());
      int index = atoi(extractbit.op1().get(ID_C_cformat).c_str());

      std::string mask;

      for(int i = 0; i < index; i++)
        mask = "1" + mask;

      mask = "0" + mask;

      for(int i = index+1; i < width; i++)
        mask = "1" + mask;

       // `a.range(left,right) = b' is converted to
       //  a =  (mask & a) |  ((typecast)b) << right

      exprt expr_mask(ID_constant, extractbit.op0().type());
      expr_mask.set(ID_value, mask);

      exprt and_expr(ID_bitand, extractbit.op0().type());
      and_expr.copy_to_operands(expr_mask, extractbit.op0());

      exprt expr_typecast(expr.op1());
      expr_typecast.make_typecast(extractbit.op0().type());

      exprt shl(ID_shl, expr_typecast.type());
      shl.copy_to_operands(expr_typecast, extractbit.op1());

      exprt or_expr(ID_bitor, and_expr.type());
      or_expr.copy_to_operands(and_expr, shl);

      simplify(or_expr, *this);

      codet assign(expr.get(ID_statement));
      assign.type() = extractbit.op0().type();
      assign.copy_to_operands(extractbit.op0(), or_expr);
      assign.location() = expr.location();

      code_expressiont code_expr;
      code_expr.type() = extractbit.type();
      code_expr.expression()=extractbit;

      code_blockt block;
      block.location() = expr.location();
      block.copy_to_operands(assign, code_expr);

      side_effect_exprt statement_expr(ID_statement_expression);
      statement_expr.type() = code_expr.type();
      statement_expr.copy_to_operands(block);

      expr = statement_expr;
      return;
    }

    if(expr.op0().type().id() == ID_verilogbv)
    {
      if(expr.id()==ID_assign_plus ||
         expr.id()==ID_assign_minus ||
         expr.id()==ID_assign_mult ||
         expr.id()==ID_assign_div)
      {
        err_location(expr);
        str << "assignment operator '" << id2string(expr.get(ID_statement))
            << "' not defined for types `" << type2cpp(expr.op0().type(),*this)
            << "' and `" << type2cpp(expr.op1().type(),*this) <<"'";
      }
      implicit_typecast(expr.op1(),expr.op0().type());
      expr.type() = expr.op0().type();
      return;
    }
    #endif

    // for structs we use the 'implicit assignment operator',
    // and therefore, it is allowed to assign to a rvalue struct.
    if(follow(type0).id()==ID_struct)
      expr.op0().set(ID_C_lvalue, true);

    c_typecheck_baset::typecheck_side_effect_assignment(expr);
    return;
  }

  // It's a non-POD.
  // Turn into an operator call

  std::string strop="operator";
  
  const irep_idt statement=expr.get(ID_statement);

  if(statement==ID_assign)
    strop += "=";
  else if(statement==ID_assign_shl)
    strop += "<<=";
  else if(statement==ID_assign_shr)
    strop += ">>=";
  else if(statement==ID_assign_plus)
    strop += "+=";
  else if(statement==ID_assign_minus)
    strop += "-=";
  else if(statement==ID_assign_mult)
    strop += "*=";
  else if(statement==ID_assign_div)
    strop += "/=";
  else if(statement==ID_assign_bitand)
    strop += "&=";
  else if(statement==ID_assign_bitor)
    strop += "|=";
  else if(statement==ID_assign_bitxor)
    strop += "^=";
  else
  {
    err_location(expr);
    str << "bad assignment operator `" << statement << "'";
    throw 0;
  }

  cpp_namet cpp_name;
  cpp_name.get_sub().push_back(irept(ID_name));
  cpp_name.get_sub().front().set(ID_identifier, strop);
  cpp_name.get_sub().front().set(ID_C_location, expr.location());

  // expr.op0() is already typechecked
  exprt already_typechecked("already_typechecked");
  already_typechecked.move_to_operands(expr.op0());

  exprt member(ID_member);
  member.set("component_cpp_name", cpp_name);
  member.move_to_operands(already_typechecked);

  side_effect_expr_function_callt new_expr;
  new_expr.function().swap(member);
  new_expr.arguments().push_back(expr.op1());
  new_expr.location()=expr.location();

  typecheck_side_effect_function_call(new_expr);

  expr.swap(new_expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_side_effect_inc_dec

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_side_effect_inc_dec(
  side_effect_exprt &expr)
{
  if(expr.operands().size()!=1)
    throw std::string("statement ")+
          id2string(expr.get_statement())+
          " expected to have one operand";

  add_implicit_dereference(expr.op0());

  typet tmp_type=follow(expr.op0().type());

  if(is_number(tmp_type) ||
     tmp_type.id()==ID_pointer)
  {
    // standard stuff
    c_typecheck_baset::typecheck_expr_side_effect(expr);
    return;
  }

  // Turn into an operator call

  std::string str_op = "operator";
  bool post = false;

  if(expr.get(ID_statement)==ID_preincrement)
    str_op += "++";
  else if(expr.get(ID_statement)==ID_predecrement)
    str_op += "--";
  else if(expr.get(ID_statement)==ID_postincrement)
  {
    str_op += "++";
    post = true;
  }
  else if(expr.get(ID_statement)==ID_postdecrement)
  {
    str_op += "--";
    post = true;
  }
  else
  {
    err_location(expr);
    str << "bad assignment operator `"
        << expr.get_statement()
        << "'";
    throw 0;
  }

  cpp_namet cpp_name;
  cpp_name.get_sub().push_back(irept(ID_name));
  cpp_name.get_sub().front().set(ID_identifier, str_op);
  cpp_name.get_sub().front().set(ID_C_location, expr.location());

  exprt already_typechecked("already_typechecked");
  already_typechecked.move_to_operands(expr.op0());

  exprt member(ID_member);
  member.set("component_cpp_name", cpp_name);
  member.move_to_operands(already_typechecked);

  side_effect_expr_function_callt new_expr;
  new_expr.function().swap(member);
  new_expr.location()=expr.location();

  // the odd C++ way to denote the post-inc/dec operator
  if(post)
    new_expr.arguments().push_back(
      from_integer(mp_integer(0), int_type()));
      
  typecheck_side_effect_function_call(new_expr);
  expr.swap(new_expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_dereference

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_dereference(exprt &expr)
{
  if(expr.operands().size()!=1)
  {
    err_location(expr);
    str << "unary operator * expects one operand";
    throw 0;
  }

  exprt &op=expr.op0();
  const typet op_type=follow(op.type());

  if(op_type.id()==ID_pointer &&
     op_type.find("to-member").is_not_nil())
  {
    err_location(expr);
    str << "pointer-to-member must use "
        << "the .* or ->* operators";
    throw 0;
  }

  c_typecheck_baset::typecheck_expr_dereference(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::convert_pmop

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::convert_pmop(exprt& expr)
{
  assert(expr.id()=="pointer-to-member");
  assert(expr.operands().size() == 2);

  if(expr.op1().type().id()!=ID_pointer
     || expr.op1().type().find("to-member").is_nil())
  {
    err_location(expr.location());
    str << "pointer-to-member expected\n";
    throw 0;
  }

  typet t0 = expr.op0().type().id()==ID_pointer ?
  expr.op0().type().subtype():  expr.op0().type();

  typet t1((const typet&)expr.op1().type().find("to-member"));

  t0 = follow(t0);
  t1 = follow(t1);

  if(t0.id()!=ID_struct)
  {
    err_location(expr.location());
    str << "pointer-to-member type error";
    throw 0;
  }

  const struct_typet &from_struct = to_struct_type(t0);
  const struct_typet &to_struct = to_struct_type(t1);

  if(!subtype_typecast(from_struct, to_struct))
  {
    err_location(expr.location());
    str << "pointer-to-member type error";
    throw 0;
  }

  if(expr.op1().type().subtype().id()!=ID_code)
  {
    err_location(expr);
    str << "pointers to data member are not supported";
    throw 0;
  }

  typecheck_expr_main(expr.op1());

  if(expr.op0().type().id()!=ID_pointer)
  {
    if(expr.op0().id()==ID_dereference)
    {
      exprt tmp = expr.op0().op0();
      expr.op0().swap(tmp);
    }
    else
    {
      assert(expr.op0().get_bool(ID_C_lvalue));
      exprt address_of(ID_address_of, typet(ID_pointer));
      address_of.copy_to_operands(expr.op0());
      address_of.type().subtype() = address_of.op0().type();
      expr.op0().swap(address_of);
    }
  }

  exprt tmp(expr.op1());
  tmp.type().set("#bound", expr.op0());
  expr.swap(tmp);
  return;
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_function_identifier

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_function_identifier(exprt &expr)
{
  if(expr.id()==ID_symbol)
  {
    // Check if the function body has to be typechecked
    contextt::symbolst::iterator it=
      context.symbols.find(expr.get(ID_identifier));

    assert(it != context.symbols.end());

    symbolt &func_symb = it->second;

    if(func_symb.value.id()=="cpp_not_typechecked")
      func_symb.value.set("is_used", true);
  }

  c_typecheck_baset::typecheck_expr_function_identifier(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr(exprt &expr)
{
  bool override_constantness=
    expr.get_bool("#override_constantness");
    
  // we take care of an ambiguity in the C++ grammar
  // needs to be done before the operands!
  explicit_typecast_ambiguity(expr);
  
  c_typecheck_baset::typecheck_expr(expr);

  if(override_constantness)
    expr.type().set(ID_C_constant, false);
}

/*******************************************************************\

Function: cpp_typecheckt::explict_typecast_ambiguity

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::explicit_typecast_ambiguity(exprt &expr)
{
  // There is an ambiguity in the C++ grammar as follows:
  // (TYPENAME) + expr   (typecast of unary plus)  vs.
  // (expr) + expr       (sum of two expressions)
  // Same issue with the operators & and - and *

  // We figure this out by resolving the type argument
  // and re-writing if needed

  if(expr.id()!="explicit-typecast")
    return;
    
  assert(expr.operands().size()==1);
  
  irep_idt op0_id=expr.op0().id();
  
  if(expr.type().id()==ID_cpp_name &&
     expr.op0().operands().size()==1 &&
     (op0_id==ID_unary_plus ||
      op0_id==ID_unary_minus ||
      op0_id==ID_address_of ||
      op0_id==ID_dereference))
  {
    exprt resolve_result=
      resolve(
        to_cpp_name(expr.type()),
        cpp_typecheck_resolvet::BOTH,
        cpp_typecheck_fargst());

    if(resolve_result.id()!=ID_type)
    {
      // need to re-write the expression
      // e.g., (ID) +expr  ->  ID+expr
      exprt new_binary_expr;
      
      new_binary_expr.operands().resize(2);
      new_binary_expr.op0().swap(expr.type());
      new_binary_expr.op1().swap(expr.op0().op0());
      
      if(op0_id==ID_unary_plus)
        new_binary_expr.id(ID_plus);
      else if(op0_id==ID_unary_minus)
        new_binary_expr.id(ID_minus);
      else if(op0_id==ID_address_of)
        new_binary_expr.id(ID_bitand);
      else if(op0_id==ID_dereference)
        new_binary_expr.id(ID_mult);
        
      new_binary_expr.location()=expr.op0().location();
      expr.swap(new_binary_expr);
    }
  }
  
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_binary_arithmetic

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_binary_arithmetic(exprt &expr)
{
  if(expr.operands().size()!=2)
  {
    err_location(expr);
    str << "operator `" << expr.id() << "' expects two operands";
    throw 0;
  }

  add_implicit_dereference(expr.op0());
  add_implicit_dereference(expr.op1());

  #ifdef CPP_SYSTEMC_EXTENSION
  if(expr.id()==ID_bitnot || expr.id()==ID_bitand ||
     expr.id()==ID_bitor  || expr.id()==ID_bitxor)
  {
    if(expr.op0().type().id()=="verilogbv")
    {
      implicit_typecast(expr.op1(), expr.op0().type());
      expr.type() = expr.op0().type();
      return;
    }
    else if(expr.op1().type().id()=="verilogbv")
    {
      implicit_typecast(expr.op0(), expr.op1().type());
      expr.type() = expr.op1().type();
      return;
    }
  }
  #endif

  c_typecheck_baset::typecheck_expr_binary_arithmetic(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_index

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_index(exprt &expr)
{
  #ifdef CPP_SYSTEMC_EXTENSION
  if(expr.operands().size()!=2)
  {
    err_location(expr);
    str << "operator `" << expr.id_string()
        << "' expects two operands";
    throw 0;
  }

  if(expr.op0().type().id()==ID_signedbv ||
     expr.op0().type().id()==ID_unsignedbv)
  {
    typecheck_expr_sc_index(expr);
    return;
  }
  #endif

  c_typecheck_baset::typecheck_expr_index(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_rel

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_comma(exprt &expr)
{
  if(expr.operands().size()!=2)
  {
    err_location(expr);
    str << "comma operator expects two operands";
    throw 0;
  }

  if(follow(expr.op0().type()).id()==ID_struct)
  {
    // TODO: check if the comma operator has been overloaded!
  }

  #if 0
  #ifdef CPP_SYSTEMC_EXTENSION
  if(expr.op0().type().id()==ID_verilogbv ||
     expr.op0().type().id()==ID_signedbv ||
     expr.op0().type().id()==ID_unsignedbv ||
     expr.op0().type().id()==ID_bool)
  {
    // do concatenation

    int width0 = expr.op0().type().id() ==ID_bool ? 1 :
              atoi(expr.op0().type().get(ID_width).c_str());

    int width1 = expr.op1().type().id() == ID_bool ? 1 :
              atoi(expr.op1().type().get(ID_width).c_str());

    irep_idt type_id=
      (expr.op0().type().id()==ID_verilogbv ||
       expr.op1().type().id()==ID_verilogbv)? ID_verilogbv : ID_unsignedbv;

    typet new_type0(type_id);
    new_type0.set(ID_width, width0);
    implicit_typecast(expr.op0(), new_type0);

    typet new_type1(type_id);
    new_type1.set(ID_width, width1);
    implicit_typecast(expr.op1(), new_type1);

    expr.id(ID_concatenation);

    typet new_type(type_id);
    new_type.set(ID_width, width0 + width1);
    expr.type()=new_type;

    return;
  }
  #endif
  #endif

  c_typecheck_baset::typecheck_expr_comma(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_rel

Inputs:

Outputs:

Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_expr_rel(exprt &expr)
{
  if(expr.operands().size() != 2)
  {
    err_location(expr);
    str << "operator `" << expr.id()
        << "' expects two operands";
    throw 0;
  }

  #ifdef CPP_SYSTEMC_EXTENSION
  if(expr.op0().type().id()==ID_verilogbv)
  {
    if(expr.id()==ID_equal || expr.id()==ID_notequal)
    {
      implicit_typecast(expr.op1(), expr.op0().type());
      expr.type()=typet(ID_bool);
      return;
    }
  }
  #endif

  c_typecheck_baset::typecheck_expr_rel(expr);
}

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_sc_member

Inputs:

Outputs:

Purpose:

\*******************************************************************/

#ifdef CPP_SYSTEMC_EXTENSION
void cpp_typecheckt::typecheck_expr_sc_member(
  exprt &expr,
  const cpp_typecheck_fargst &fargs)
{
  cpp_namet cpp_name=
    to_cpp_name(expr.find("component_cpp_name"));

  if(cpp_name.get_sub().size()!=1 &&
     cpp_name.get_sub()[0].id()!=ID_name)
  {
    err_location(expr);
    str << "error: bad SystemC member expression";
    throw 0;
  }

  irep_idt name=cpp_name.get_sub()[0].get(ID_identifier);

  if(name=="range")
  {
    if(fargs.operands.size()!=2)
    {
      err_location(expr);
      str << "error: `range' expects two arguments";
      throw 0;
    }

    exprt arg0=fargs.operands[0];
    exprt arg1=fargs.operands[1];

    make_constant_index(arg0);
    make_constant_index(arg1);

    mp_integer o0, o1;

    if(to_integer(arg0, o0))
    {
      err_location(arg0);
      str << "failed to convert index 0";
      throw 0;
    }

    if(o0<0)
    {
      err_location(arg1);
      str << "index 1 out of range";
      throw 0;
    }

    if(to_integer(arg1, o1))
    {
      err_location(arg1);
      str << "failed to convert index 2";
      throw 0;
    }

    if(o1<0)
    {
      err_location(arg1);
      str << "index 2 out of range";
      throw 0;
    }

    if(o1>o0)
    {
      err_location(expr);
      str << "index 2 greater than index 1";
    }

    exprt extractbits(ID_extractbits, expr.op0().type());
    extractbits.type().set(ID_width, integer2string(o0-o1+1));
    extractbits.copy_to_operands(expr.op0(), arg0, arg1);

    expr=exprt("sc_extension");
    expr.copy_to_operands(extractbits);
    return;
  }
  else
  {
//     ps_irep("sc_expr",expr);
    std::cout << "MEMBER: " << expr.pretty() << std::endl;
    assert(0);
  }
}
#endif

/*******************************************************************\

Function: cpp_typecheckt::typecheck_expr_sc_index

Inputs:

Outputs:

Purpose:

\*******************************************************************/

#ifdef CPP_SYSTEMC_EXTENSION
void cpp_typecheckt::typecheck_expr_sc_index(exprt &expr)
{
  exprt &index_expr=expr.op1();

  make_index_type(index_expr);

  expr.id(ID_extractbit);
  expr.type()=typet(ID_bool);
}
#endif

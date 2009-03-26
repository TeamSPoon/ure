/*
 * opencog/comboreduct/type_tree.cc
 *
 * Copyright (C) 2002-2008 Novamente LLC
 * All Rights Reserved
 *
 * Written by Nil Geisweiller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "util/Logger.h"

#include "comboreduct/combo/type_tree.h"
#include "comboreduct/combo/descriptions.h"

namespace combo
{

// ------------ support for builtins --------------------------

char get_arity(builtin b)
{
    using namespace builtin_properties;
    return builtins_properties::instance().builtin_arity(b);
}

type_tree get_type_tree(builtin b)
{
    using namespace builtin_properties;
    return builtins_properties::instance().type_tree_of_builtin(b);
}

type_tree get_output_type_tree(builtin b)
{
    using namespace builtin_properties;
    return type_tree(builtins_properties::instance().output_type_of_builtin(b));
}

type_tree get_input_type_tree(builtin b, arity_t i)
{
    using namespace builtin_properties;
    if (builtins_properties::instance().builtin_arity(b) == -1)
        return builtins_properties::instance().builtin_argument(b, 0);
    if (i < builtins_properties::instance().builtin_arity(b))
        return builtins_properties::instance().builtin_argument(b, i);
    else
        return type_tree(); //return empty tree
}


// ------------ support for actions --------------------------

type_tree get_type_tree(action a)
{
    using namespace action_properties;
    return actions_properties::instance().type_tree_of_action(a);
}

type_tree get_output_type_tree(action a)
{
    using namespace action_properties;
    return type_tree(actions_properties::instance().output_type_of_action(a));
}

type_tree get_input_type_tree(action a, arity_t i)
{
    using namespace action_properties;
    if (actions_properties::instance().action_arity(a) == -1)
        return actions_properties::instance().action_argument(a, 0);
    if (i < actions_properties::instance().action_arity(a))
        return actions_properties::instance().action_argument(a, i);
    else
        return type_tree();
}


// ------------ support for builtin actions --------------------------

type_tree get_type_tree(builtin_action a)
{
    return a->get_type_tree();
}

type_tree get_output_type_tree(builtin_action a)
{
    return a->get_output_type_tree();
}

type_tree get_input_type_tree(builtin_action a, arity_t i)
{
    return a->get_input_type_tree(i);
}

// ------------ support for perceptions --------------------------

type_tree get_type_tree(perception p)
{
    return p->get_type_tree();
}

type_tree get_output_type_tree(perception p)
{
    return p->get_output_type_tree();
}

type_tree get_input_type_tree(perception p, arity_t i)
{
    return p->get_input_type_tree(i);
}

// ------------ support for procedure_call --------------------------

type_tree get_type_tree(procedure_call pc)
{
    return pc->get_type_tree();
}

type_tree get_output_type_tree(procedure_call pc)
{
    return pc->get_output_type_tree();
}

type_tree get_input_type_tree(procedure_call pc, arity_t i)
{
    return pc->get_input_type_tree(i);
}

// -----------------------------------------------------------------


//all other get_type_tree functions
type_tree get_type_tree(const argument& a)
{
    return type_tree((type_node)((int)id::argument_type + a.abs_idx_from_zero()));
}
type_tree get_type_tree(contin_t t)
{
    return type_tree(id::contin_type);
}
type_tree get_type_tree(const definite_object& d)
{
    if (is_action_definite_object(d))
        return type_tree(id::action_definite_object_type);
    else
        return type_tree(id::definite_object_type);
}
type_tree get_type_tree(indefinite_object i)
{
    return type_tree(id::indefinite_object_type);
}
type_tree get_type_tree(const message& m)
{
    return type_tree(id::message_type);
}
type_tree get_type_tree(action_symbol as)
{
    return type_tree(id::action_symbol_type);
}
type_tree get_type_tree(wild_card wc)
{
    return type_tree(id::wild_card_type);
}


type_tree get_type_tree(const vertex& v)
{
    //builtin
    if (is_builtin(v))
        return get_type_tree(get_builtin(v));
    //arguments
    else if (is_argument(v))
        return get_type_tree(get_argument(v));
    //contin_t
    else if (is_contin(v))
        return get_type_tree(get_contin(v));
    //action
    else if (is_action(v))
        return get_type_tree(get_action(v));
    //builtin_action
    else if (is_builtin_action(v))
        return get_type_tree(get_builtin_action(v));
    //perception
    else if (is_perception(v))
        return get_type_tree(get_perception(v));
    //definite_object
    else if (is_definite_object(v))
        return get_type_tree(get_definite_object(v));
    //indefinite_object
    else if (is_indefinite_object(v))
        return get_type_tree(get_indefinite_object(v));
    //message
    else if (is_message(v))
        return get_type_tree(get_message(v));
    //procedure_call
    else if (is_procedure_call(v))
        return get_type_tree(get_procedure_call(v));
    //action_symbol
    else if (is_action_symbol(v))
        return get_type_tree(get_action_symbol(v));
    //wild_card
    else if (is_wild_card(v))
        return get_type_tree(get_wild_card(v));
    //other non handled cases
    else {
        opencog::cassert(TRACE_INFO, false, "this case isn't handled yet");
        type_tree tmp;
        return tmp;
    }
}

type_tree get_output_type_tree(const vertex& v)
{
    if (is_builtin(v)) {
        return get_output_type_tree(get_builtin(v));
    } else if (is_argument(v)) {
        return type_tree(id::unknown_type);
    } else if (is_contin(v)) {
        return type_tree(id::contin_type);
    } else if (is_action(v)) {
        return get_output_type_tree(get_action(v));
    } else if (is_builtin_action(v)) {
        return get_output_type_tree(get_builtin_action(v));
    } else if (is_perception(v)) {
        return get_output_type_tree(get_perception(v));
    } else if (is_definite_object(v)) {
        return get_type_tree(v);
    } else if (is_indefinite_object(v)) {
        return type_tree(id::indefinite_object_type);
    } else if (is_wild_card(v)) {
        return type_tree(id::wild_card_type);
    } else if (is_message(v)) {
        return type_tree(id::message_type);
    } else if (is_procedure_call(v)) {
        return get_output_type_tree(get_procedure_call(v));
    } else {
        std::stringstream out;
        out << v;
        opencog::cassert(TRACE_INFO, false, "Unhandled case '%s'",
                          out.str().c_str());
        return type_tree(id::ill_formed_type);
    }
}

type_tree get_input_type_tree(const vertex& v, arity_t i)
{
    if (is_builtin(v)) {
        return get_input_type_tree(get_builtin(v), i);
    } else if (is_argument(v)) {
        return type_tree(id::unknown_type);
    } else if (is_contin(v)) {
        return type_tree();
    } else if (is_action(v)) {
        return get_input_type_tree(get_action(v), i);
    } else if (is_builtin_action(v)) {
        return get_input_type_tree(get_builtin_action(v), i);
    } else if (is_perception(v)) {
        return get_input_type_tree(get_perception(v), i);
    } else if (is_definite_object(v)) {
        return type_tree();
    } else if (is_indefinite_object(v)) {
        return type_tree();
    } else if (is_message(v)) {
        return type_tree();
    } else if (is_wild_card(v)) {
        return type_tree();
    } else if (is_procedure_call(v)) {
        return get_input_type_tree(get_procedure_call(v), i);
    } else {
        std::stringstream out;
        out << v;
        opencog::cassert(TRACE_INFO, false,
                          "Unhandled case '%s'", out.str().c_str());
        return type_tree(id::ill_formed_type);
    }
}

arity_t contin_arity(const type_tree& ty)
{
    typedef type_tree::sibling_iterator sib_it;
    int res = 0;
    type_tree::iterator ty_it = ty.begin();
    if (*ty_it == id::lambda_type)
        for (sib_it sib = ty_it.begin();sib != sib_it(ty.last_child(ty_it));++sib)
            if (*sib == id::contin_type)
                res++;
    return res;
}

arity_t boolean_arity(const type_tree& ty)
{
    typedef type_tree::sibling_iterator sib_it;
    int res = 0;
    type_tree::iterator ty_it = ty.begin();
    if (*ty_it == id::lambda_type)
        for (sib_it sib = ty_it.begin();sib != sib_it(ty.last_child(ty_it)); ++sib)
            if (*sib == id::boolean_type)
                res++;
    return res;
}

arity_t action_result_arity(const type_tree& ty)
{
    typedef type_tree::sibling_iterator sib_it;
    int res = 0;
    type_tree::iterator ty_it = ty.begin();
    if (*ty_it == id::lambda_type)
        for (sib_it sib = ty_it.begin();sib != sib_it(ty.last_child(ty_it)); ++sib)
            if (*sib == id::action_result_type)
                res++;
    return res;
}

arity_t type_tree_arity(const type_tree& ty)
{
    opencog::cassert(TRACE_INFO, !ty.empty(),
                      "Not sure this assert should not be replaced by a conditional");
    type_tree::iterator ty_it = ty.begin();
    if (*ty_it == id::lambda_type) {
        unsigned int noc = ty_it.number_of_children();
        opencog::cassert(TRACE_INFO, noc > 0, "Lambda must not be childless");
        type_tree_pre_it ty_last_child = ty_it.last_child();
        if (noc == 1)
            return 0;
        else {
            arity_t a = noc - 1;
            type_tree_pre_it last_arg = ty.previous_sibling(ty_last_child);
            return (*last_arg == id::arg_list_type ? -a : a);
        }
    }
    return 0;
}

argument_type_list type_tree_input_arg_types(const type_tree& ty)
{
    opencog::cassert(TRACE_INFO, !ty.empty(),
                      "Not sure this assert should not be replaced by a conditional");
    argument_type_list res;
    type_tree_pre_it ty_it = ty.begin();
    if (*ty_it == id::lambda_type) {
        opencog::cassert(TRACE_INFO, !ty_it.is_childless(),
                          "Lambda must not be childless");
        type_tree_sib_it sib = ty_it.begin();
        //setting input argument type trees
        for (; sib != type_tree_sib_it(ty.last_child(ty_it)); ++sib) {
            res.push_back(type_tree(*sib == id::arg_list_type ? sib.begin() : sib));
        }
    }
    return res;
}

arity_t convert_index(arity_t arity, arity_t index)
{
    arity_t ap = -arity;
    if (ap > 0 && index >= ap) {
        return ap -1;
    }
    return index;
}

arity_t abs_min_arity(arity_t arity)
{
    if (arity < 0)
        return -arity - 1;
    else return arity;
}

const type_tree& argument_type_list_input_type(const argument_type_list& atl,
                                               arity_t arity,
                                               arity_t index)
{
    return atl[convert_index(arity, index)];
}

type_tree type_tree_output_type_tree(const type_tree& ty)
{
    opencog::cassert(TRACE_INFO, !ty.empty(), "ty must not be empty");
    type_tree_pre_it ty_it = ty.begin();
    if (*ty_it == id::lambda_type)
        return type_tree(ty_it.last_child());
    else
        return type_tree(ty_it);
}

arity_t get_arity(const vertex& v)
{
    if (is_builtin(v))
        return get_arity(get_builtin(v));
    else if (is_argument(v))
        return 1;
    else if (is_contin(v))
        return 0;
    else if (is_action(v))
        return get_arity(get_action(v));
    else if (is_builtin_action(v))
        return get_builtin_action(v)->arity();
    else if (is_perception(v))
        return get_perception(v)->arity();
    else if (is_definite_object(v))
        return 0;
    else if (is_indefinite_object(v))
        return 0;
    else if (is_message(v))
        return 0;
    else if (is_wild_card(v))
        return 0;
    else if (is_procedure_call(v))
        return get_procedure_call(v)->arity();
    else if (is_action_symbol(v))
        return 0;
    else {
        opencog::cassert(TRACE_INFO, false, "Unhandled case");
        return 0;
    }
}

bool equal_type_tree(const type_tree& ty1, const type_tree& ty2)
{
    return inherit_type_tree(ty1, ty2) && inherit_type_tree(ty2, ty1);
}

bool inherit_type_tree(const type_tree& ty1, const type_tree& ty2)
{
    return inherit_type_tree(ty1, type_tree_pre_it(ty1.begin()),
                             ty2, type_tree_pre_it(ty2.begin()));
}

bool inherit_type_tree(const type_tree& ty1, type_tree_pre_it it1,
                       const type_tree& ty2, type_tree_pre_it it2)
{
    //--------------
    //recursive case
    //--------------

    //union case
    if (*it1 == id::union_type) {
        //all type in the union must inherit from ty2 at it2
        for (type_tree_sib_it sib = it1.begin(); sib != it1.end(); ++sib) {
            if (!inherit_type_tree(ty1, type_tree_pre_it(sib), ty2, it2))
                return false;
        }
        return true;
    } else if (*it2 == id::union_type) {
        opencog::cassert(TRACE_INFO, *it1 != id::union_type,
                          "Due to the recursive structure of that function it is impossible that *it1 is id::union_type");
        //the type of it1 must inherit at least one type of the union of it2
        for (type_tree_sib_it sib = it2.begin(); sib != it2.end(); ++sib) {
            if (inherit_type_tree(ty1, it1, ty2, type_tree_pre_it(sib)))
                return true;
        }
        return false;
    }
    //lambda case
    else if (*it1 == id::lambda_type && *it2 == id::lambda_type) {
        if (it1.number_of_children() == it2.number_of_children()) {
            //all input arguments sib2 must inherit sib1
            //or sib2 must be unknown_type
            //and output of ty1 must inherit from ty2
            type_tree_sib_it sib1 = it1.begin();
            type_tree_sib_it sib2 = it2.begin();
            for (; sib2 != it2.last_child(); ++sib1, ++sib2) {
                if (!inherit_type_tree(ty2, type_tree_pre_it(sib2),
                                       ty1, type_tree_pre_it(sib1))
                        && *sib2 != id::unknown_type)
                    return false;
            }
            if (inherit_type_tree(ty1, type_tree_pre_it(sib1),
                                  ty2, type_tree_pre_it(sib2)))
                return true;
            else return false;
        } else return false;
    }
    //application case
    else if (*it1 == id::application_type) {
        return false; //from the assumption that type_tree must be first reduced
    }
    //arg_list case
    else if (*it1 == id::arg_list_type && *it2 == id::arg_list_type) {
        opencog::cassert(TRACE_INFO, it1.has_one_child() && it2.has_one_child(),
                          "arg_list_type takes only one argument");
        return inherit_type_tree(ty1, type_tree_pre_it(it1.begin()),
                                 ty2, type_tree_pre_it(it2.begin()));
    }
    //----------
    //base cases
    //----------
    else if (*it1 == id::ill_formed_type || *it2 == id::ill_formed_type)
        return false;//nothing can inherit or be inherited from a ill formed type
    else if (*it2 == id::unknown_type) //everything but ill_formed_type inherits from unknown_type
        return true;
    else return *it1 == *it2; //all other cases
}

void reduce_type_tree(type_tree& tt,
                      const argument_type_list& arg_types,
                      const combo_tree& tr,
                      const std::string& proc_name)
{
    if (!tt.empty()) {
        reduce_type_tree(tt, tt.begin(), arg_types, tr, tr.begin(), proc_name);
    } else std::cout << "Warning : you're trying to reduce an empty type_tree"
                     << std::endl;
}

void reduce_type_tree(type_tree& tt,
                      const combo_tree& tr,
                      const std::string& proc_name)
{
    argument_type_list empty_arg_types;
    reduce_type_tree(tt, empty_arg_types, tr, proc_name);
}

void reduce_type_tree(type_tree& tt, type_tree_pre_it it,
                      const argument_type_list& arg_types,
                      const combo_tree& tr, combo_tree::iterator ct_it,
                      const std::string& proc_name)
{

    //---------------------
    //   recursive cases
    //---------------------

    //-----------
    //lambda case
    //-----------
    if (*it == id::lambda_type) {
        opencog::cassert(TRACE_INFO, !it.is_childless(),
                          "lambda_type must have at least a child");
        //if lambda has only one child
        //then reduce lambda(X) -> X
        if (it.has_one_child()) {
            type_tree_pre_it it_child = it.begin();
            *it = *it_child;
            tt.erase(tt.flatten(it_child));
            reduce_type_tree(tt, it, arg_types, tr, ct_it, proc_name);
        } else {
            for (type_tree_sib_it sib = it.begin(); sib != it.end(); ++sib)
                reduce_type_tree(tt, type_tree_pre_it(sib), arg_types,
                                 tr, ct_it, proc_name);
        }
    }
    //----------------
    //application case
    //----------------
    else if (*it == id::application_type) {
        opencog::cassert(TRACE_INFO, !it.is_childless(),
                          "application_type must have at least a child");
        type_tree_pre_it it_child = it.begin();
        //if application has only one child
        //them reduce application(X) -> X
        if (it.has_one_child()) {
            *it = *it_child;
            tt.erase(tt.flatten(it_child));
            reduce_type_tree(tt, it, arg_types, tr, ct_it, proc_name);
        } else if (*it_child == id::lambda_type) {

            //check if the output type of all siblings of it_child
            //inherit from all children but the last of it_child
            //or, if the previous last child of it_child is arg_list,
            //inherit from the children before that arg_list and
            //all next inherit from the child of arg_list
            //that is for instance in
            //application(lambda(T1 arg_list(T2) T3) a1 a2 a3 a4)
            //check that a1 inherits from T1, a2 a3 and a4 inherit from T2

            opencog::cassert(TRACE_INFO, !it_child.is_childless(),
                              "it_child must have at least one child");
            //cia stands for current input argument
            type_tree_pre_it cia_it = it_child.begin();

            //number of input arguments the function takes
            int arg_count = it_child.number_of_children() - 1;
            //number of arguments applied to that function
            int arg_count_app = tt.number_of_siblings(it_child);
            //check whether the last input argument under the lambda
            //is arg_list
            type_tree_sib_it last_arg_sib = it_child.last_child();
            --last_arg_sib;
            //ila is true if the last input argument under lambda is arg_list
            bool ila = tt.is_valid(last_arg_sib)
                && *last_arg_sib == id::arg_list_type;
            //check if the number of applied arguments are incorrect
            if ((ila && arg_count_app < arg_count - 1)
                    || (!ila && arg_count_app != arg_count)) {

                //log message
                std::stringstream message;
                message << "combo::type_tree - Type reduction error:"
                        << " the number of arguments, which is "
                        << arg_count_app;

                if(!tr.empty()) { //trace info debug is available
                    message << ", involved in the"
                            << " application '"
                            << combo_tree(ct_it)
                            << "' located at pre-order index "
                            << opencog::pre_order_index(tr, ct_it)
                            << " of procedure '"
                            << proc_name << "'";
                }

                message << ", is not right as the operator must take ";
                if (ila)
                    message << arg_count - 1 << " arguments or more.";
                else
                    message << arg_count << " arguments.";

                opencog::logger().log(opencog::Logger::ERROR,
                                message.str().c_str());
                //~log message

                *it = id::ill_formed_type;
                tt.erase_children(it);
                return;
            } else { //the number of applied arguments is correct
                type_tree_pre_it output_it = it_child.last_child();

                //will be set to true if at some point arg_list is reached
                bool is_arg_list_reached = false;

                // this is done so that ct_it can move over its arguments
                // along the arg_app (see below)
                combo_tree::iterator ct_it_child;
                if(!tr.empty()) 
                    ct_it_child = ct_it.begin();

                //iterate over the applied arguments
                //and possibly over the operand of tr in case tr is not empty
                for (type_tree_sib_it arg_app = tt.next_sibling(it_child);
                     arg_app != it.end(); ++arg_app) {
                    //check if cia_it is arg_list(T)
                    if (*cia_it == id::arg_list_type)
                        is_arg_list_reached = true;
                    //reduce both input and applied arguments
                    reduce_type_tree(tt, arg_app, arg_types,
                                     tr, ct_it_child, proc_name);
                    reduce_type_tree(tt, cia_it, arg_types,
                                     tr, ct_it, proc_name);
                    //input_arg_it is either cia_it
                    //or cia_it.begin() if cia_it is arg_list
                    type_tree_pre_it input_arg_it = cia_it;
                    if (is_arg_list_reached)
                        input_arg_it = input_arg_it.begin();
                    if (!inherit_type_tree(tt, arg_app, tt, input_arg_it)) {
                        //then check if the ouput argument of arg_app inherits
                        //from cia_it (or cia_it child if cia_it is arg_list)
                        if (*arg_app == id::lambda_type) {
                            opencog::cassert(TRACE_INFO,
                                              !arg_app.is_childless(),
                                              "lambda must have at least one child");
                            type_tree_pre_it output_it = arg_app.last_child();
                            if (inherit_type_tree(tt, output_it,
                                                  tt, input_arg_it)) {
                                //insert all inputs before cia_it
                                for (type_tree_sib_it input_sib = arg_app.begin();
                                     input_sib != arg_app.last_child(); ++input_sib)
                                    tt.insert_subtree(cia_it, input_sib);
                            }
                            //if it does not inherit then there is type checker
                            //error and the resulting type is ill_formed
                            else {
                                
                                //log message
                                std::stringstream message;
                                message << "combo::type_tree - Type reduction error:"
                                        << " the output of the "
                                        << tt.sibling_index(arg_app)
                                        << "th applied argument ";
                                if(!tr.empty()) { //trace debug info available
                                    message << "'" << combo_tree(ct_it_child)
                                            << "' (located at pre-order index "
                                            << opencog::pre_order_index(tr,
                                                                         ct_it_child)
                                            << " of procedure '"
                                            << proc_name << "')"
                                            << " of operator '"
                                            << *ct_it << "', ";
                                }
                                message << "has type "
                                        << type_tree(output_it)
                                        << " which does not inherits from "
                                        << type_tree(input_arg_it);
                                opencog::logger().log(opencog::Logger::ERROR,
                                                message.str().c_str());
                                //~log message

                                *it = id::ill_formed_type;
                                tt.erase_children(it);
                                return;
                            }
                        }
                        //if it's not a lambda then it is ill formed
                        //because arg_app does not inherits input_arg_it
                        else {
                            
                            //log message
                            std::stringstream message;
                            message << "combo::type_tree - Type reduction error:"
                                    << " the "
                                    << tt.sibling_index(arg_app)
                                    << "th applied argument ";
                            if(!tr.empty()) { //trace debug info available
                                message << "'" << combo_tree(ct_it_child)
                                        << "' (located at pre-order index "
                                        << opencog::pre_order_index(tr,
                                                                     ct_it_child)
                                        << " of procedure '"
                                        << proc_name << "')"
                                        << " of operator '"
                                        << *ct_it << "', ";
                            }
                            message << "has type "
                                    << type_tree(arg_app)
                                    << " which does not inherits from "
                                    << type_tree(input_arg_it);
                            opencog::logger().log(opencog::Logger::ERROR,
                                            message.str().c_str());
                            //~log message

                            *it = id::ill_formed_type;
                            tt.erase_children(it);
                            return;
                        }
                    }
                    //if cia_it is not an arg_list
                    //then erase it and its children
                    //because this input has been substituted by the inputs
                    //of the application (if there are some)
                    //and cia_it will now points to the next input argument
                    if (!is_arg_list_reached) {
                        cia_it = tt.erase(cia_it);
                    }
                    
                    if(!tr.empty()) //to move the operands of tr to the next
                        ct_it_child = tr.next_sibling(ct_it_child);
                }
                //remove a possibly remaining arg_list(T) in the input arguments
                if (is_arg_list_reached || ila) {
                    tt.erase(cia_it);
                }
                //remove application_type and its argument now that the
                //substitution has been done
                for (type_tree_sib_it sib = tt.next_sibling(it_child);
                     sib != it.end();)
                    sib = tt.erase(sib);
                *it = *it_child;
                tt.erase(tt.flatten(it_child));
                //check if it is in the form lambda(X) and remove lambda if so
                if (it.has_one_child()) {
                    type_tree_pre_it it_child = it.begin();
                    *it = *it_child;
                    tt.erase(tt.flatten(it_child));
                    reduce_type_tree(tt, it, arg_types, tr, ct_it, proc_name);
                }
            }
        }
        // that is the first of the application is not lambda
        // then there is a type checker error because the so called operator
        // is not a function
        else {
            //log message
            std::stringstream message;
            message << "combo::type_tree - Type reduction error:"
                    << " the supposely operator ";

            if(!tr.empty()) { // trace debug info available
                message << "'" << *ct_it
                        << "', located at pre-order index "
                        << opencog::pre_order_index(tr, ct_it)
                        << " of procedure '"
                        << proc_name << "', ";
            }
            message << "is not typed as a function (that is lambda)"
                    << " but is typed "
                    << type_tree(it_child);
            opencog::logger().log(opencog::Logger::ERROR, message.str().c_str());
            //~log message

            *it = id::ill_formed_type;
            tt.erase_children(it);
            return;
        }
    }
    //----------
    //union case
    //----------
    else if (*it == id::union_type) {
        opencog::cassert(TRACE_INFO, !it.is_childless(),
                          "union_type must have at least a child");
        std::vector<type_tree> utt;
        //apply reduce recursively to the children of the union
        for (type_tree_sib_it sib = it.begin(); sib != it.end(); ++sib) {
            reduce_type_tree(tt, type_tree_pre_it(sib), arg_types,
                             tr, ct_it, proc_name);
        }
        //remove sib if it inherits from other siblings
        for (type_tree_sib_it sib = it.begin(); sib != it.end();) {
            bool found_inherit = false;
            for (type_tree_sib_it sib2 = it.begin();
                    sib2 != it.end() && !found_inherit; ++sib2) {
                found_inherit =
                    sib != sib2
                    && inherit_type_tree(tt, type_tree_pre_it(sib),
                                         tt, type_tree_pre_it(sib2));
            }
            if (found_inherit) {
                sib = tt.erase(sib);
            } else ++sib;
        }
        //if union has only one child then reduce union(X) -> X
        if (it.has_one_child()) {
            type_tree_pre_it it_child = it.begin();
            *it = *it_child;
            tt.erase(tt.flatten(it_child));
        }
    }
    //-------------
    //arg_list case
    //-------------
    else if (*it == id::arg_list_type) {
        opencog::cassert(TRACE_INFO, it.has_one_child(),
                          "arg_list_type must have exactly one child");
        reduce_type_tree(tt, type_tree_pre_it(it.begin()), arg_types,
                         tr, ct_it, proc_name);
    }

    //----------------
    //   base case
    //----------------

    //at this stage an argument is considered as constant, so it is simply
    //replaced by its type
    //The binding between considering argument as input variable in the
    //signature of the function is made at the end in
    //infer_type_tree(const type_tree* tt)
    else if (is_argument_type(*it)) {
        type_tree tmp = get_arg_type(*it, arg_types);
        type_tree_pre_it tmp_head = tmp.begin();
        *it = *tmp_head;
        tt.reparent(it, tmp_head);
    }

}

type_tree get_intersection(const type_tree& tt1, const type_tree& tt2)
{
    opencog::cassert(TRACE_INFO, !tt1.empty() && !tt2.empty(),
                      "neither tt1 nor tt2 must be empty");
    type_tree_pre_it head1 = tt1.begin();
    type_tree_pre_it head2 = tt2.begin();
    type_tree res = get_intersection(tt1, head1, tt2, head2);
    return res;
}

type_tree get_intersection(const type_tree& tt1, type_tree_pre_it it1,
                           const type_tree& tt2, type_tree_pre_it it2)
{
    //check whether one inherit from the other
    if (inherit_type_tree(tt1, it1, tt2, it2))
        return type_tree(it1);
    else if (inherit_type_tree(tt2, it2, tt1, it1))
        return type_tree(it2);
    //check if tt1 and tt2 are unions, if so the intersection of the unions
    //is the union of the interections
    else if (*it1 == id::union_type && *it2 == id::union_type) {
        std::set<type_tree, opencog::size_tree_order<type_node> > union_of_inter;
        for (type_tree_sib_it sib1 = it1.begin(); sib1 != it1.end(); ++sib1) {
            for (type_tree_sib_it sib2 = it2.begin(); sib2 != it2.end(); ++sib2) {
                type_tree inter_tt = get_intersection(tt1,
                                                      type_tree_pre_it(sib1),
                                                      tt2,
                                                      type_tree_pre_it(sib2));
                if (is_well_formed(inter_tt))
                    union_of_inter.insert(inter_tt);
            }
        }
        if (union_of_inter.empty()) {
            return type_tree(id::ill_formed_type);
        } else if (union_of_inter.size() == 1) {
            return *union_of_inter.begin();
        } else {
            type_tree tmp;
            tmp.set_head(id::union_type);
            type_tree_pre_it head = tmp.begin();
            for (std::set<type_tree>::const_iterator its = union_of_inter.begin();
                    its != union_of_inter.end(); ++its) {
                tmp.replace(tmp.append_child(head), its->begin());
            }
            return tmp;
        }
    }
    //if both are lambda, then calculate the intersection of
    //their outputs and the union of there inputs
    else if (*it1 == id::lambda_type && *it2 == id::lambda_type) {
        unsigned int n1 = it1.number_of_children();
        unsigned int n2 = it2.number_of_children();
        opencog::cassert(TRACE_INFO, n1 > 0 && n2 > 0,
                          "lambda must not be childless");
        if (n1 != n2) {
            return type_tree(id::ill_formed_type);
        } else {
            type_tree tmp(id::lambda_type);
            type_tree_pre_it tmp_head = tmp.begin();
            type_tree_sib_it sib1 = it1.begin();
            type_tree_sib_it sib2 = it2.begin();
            //perform union of arguments
            for (; sib1 != it1.last_child(); ++sib1, ++sib2) {
                type_tree_pre_it union_it =
                    tmp.append_child(tmp_head, id::union_type);
                tmp.replace(tmp.append_child(union_it), sib1);
                tmp.replace(tmp.append_child(union_it), sib2);
            }
            type_tree output_tt = get_intersection(tt1, type_tree_pre_it(sib1),
                                                   tt2, type_tree_pre_it(sib2));
            type_tree_pre_it output_tt_it = output_tt.begin();
            tmp.replace(tmp.append_child(tmp_head), output_tt_it);
            reduce_type_tree(tmp);
            return tmp;
        }
    }
    //check if one is unknown_type
    else if (*it1 == id::unknown_type)
        return type_tree(it2);
    else if (*it2 == id::unknown_type)
        return type_tree(it1);
    //check if one is ill_formed_type
    else if (*it1 == id::ill_formed_type || *it2 == id::ill_formed_type)
        return type_tree(id::ill_formed_type);
    //otherwise there is no detected intersection and it returns ill_formed
    else return type_tree(id::ill_formed_type);
}

//function to fill a vector of type_tree corresponding to a given
//arguments
void set_arg_type(const type_tree& tt, const argument& arg,
                  argument_type_list& arg_types)
{
    set_arg_type(tt, arg.abs_idx(), arg_types);
}
//like above but takes a type_node corresponding to an argument
//it is assumed that the given type_node does correspond to an argument
void set_arg_type(const type_tree& tt, type_node arg,
                  argument_type_list& arg_types)
{
    set_arg_type(tt, arg_to_idx(arg), arg_types);
}
//like above but uses the index of the argument
//(counted from 1, like in class argument of vertex.h)
void set_arg_type(const type_tree& tt, unsigned int idx,
                  argument_type_list& arg_types)
{
    opencog::cassert(TRACE_INFO, idx > 0);
    unsigned int idxfz = idx - 1;
    unsigned int s = arg_types.size();
    if (s < idx)
        for (unsigned int i = s; i < idx; i++)
            arg_types.push_back(type_tree(id::unknown_type));
    arg_types[idxfz] = tt;
}

//return the type_tree stored in the vector arg_types
//if no type_tree are stored then return the type_tree id::unknown_type
const type_tree& get_arg_type(const argument& arg,
                              const argument_type_list& arg_types)
{
    return get_arg_type(arg.abs_idx(), arg_types);
}
//like above but takes a type_node corresponding to an argument
//it is assumed that the given type_node does correspond to an argument
const type_tree& get_arg_type(type_node arg,
                              const argument_type_list& arg_types)
{
    return get_arg_type(arg_to_idx(arg), arg_types);
}
//like above but uses the index of the argument
//(counted from 1, like in class argument of vertex.h)
const type_tree& get_arg_type(unsigned int idx,
                              const argument_type_list& arg_types)
{
    static const type_tree unknown_type_tree = type_tree(id::unknown_type);
    if (idx <= arg_types.size())
        return arg_types[idx-1];
    else return unknown_type_tree;
}

type_tree infer_vertex_type(const combo_tree& tr, combo_tree::iterator it,
                            const argument_type_list& atl)
{
    typedef combo_tree::iterator pre_it;
    typedef combo_tree::sibling_iterator sib_it;
    pre_it it_parent = tr.parent(it);
    type_tree res = type_tree(id::unknown_type);
    //set output type
    if (tr.is_valid(it_parent)) {
        unsigned int si = tr.sibling_index(it);
        arity_t a = get_arity(*it_parent);
        if (a < 0 || (a > 0 && (arity_t)si < a))
            res = get_input_type_tree(*it_parent, tr.sibling_index(it));
        else return type_tree(id::ill_formed_type);
    }
    //set input types
    if (!it.is_childless()) {
        //wrap lambda over res
        type_tree_pre_it head = res.begin();
        type_tree_pre_it new_head = res.wrap(head, id::lambda_type);
        for (sib_it sib = it.begin(); sib != it.end(); ++sib) {
            //if argument then uses atl to get the input type
            type_tree child_type;
            if (is_argument(*sib)) {
                child_type = get_arg_type(get_argument(*sib), atl);
            } else child_type = get_output_type_tree(*sib);
            type_tree_pre_it child_type_head = child_type.begin();
            res.insert_subtree(head, child_type_head);
        }
    }
    return res;
}

//that function uses a trace of procedure_calls already in progress
//in order to avoid infinite loop due to circularity references
void infer_arg_type_tree(const combo_tree& tr, argument_type_list& arg_types)
{
    typedef combo_tree::leaf_iterator leaf_it;
    typedef combo_tree::iterator pre_it;
    opencog::cassert(TRACE_INFO, !tr.empty(),
                      "cannot infer arg types on an empty combo_tree");
    for (leaf_it lit = tr.begin_leaf(); lit != tr.end_leaf(); ++lit) {
        if (is_argument(*lit)) {
            argument a = get_argument(*lit);
            type_tree att = get_arg_type(a, arg_types);
            type_tree itt = get_intersection(att,
                                             infer_vertex_type(tr, lit,
                                                               arg_types));
            set_arg_type(itt, a, arg_types);
        }
    }
}

void insert_arg_type_tree(const argument_type_list& arg_types,
                          type_tree& tt2)
{
    opencog::cassert(TRACE_INFO, !tt2.empty(),
                      "tt2 is supposed to contain a type");
    if (!arg_types.empty()) {
        type_tree_pre_it head = tt2.begin();
        if (*head != id::lambda_type)
            head = tt2.wrap(head, id::lambda_type);
        type_tree_pre_it first_var_arg = head.begin();
        for (argument_type_list_const_it itt = arg_types.begin();
                itt != arg_types.end(); ++itt) {
            tt2.insert_subtree(first_var_arg, itt->begin());
        }
    }
}

type_tree get_type_tree(const combo_tree& tr)
{
    type_tree tmp;
    if (!tr.empty())
        tmp = get_type_tree(tr, combo_tree::iterator(tr.begin()));
    return tmp;
}

type_tree get_type_tree(const combo_tree& tr, combo_tree::iterator it)
{
    opencog::cassert(TRACE_INFO, !tr.empty(), "tr cannot be empty");
    opencog::cassert(TRACE_INFO, tr.is_valid(it), "it must be valid");
    type_tree tmp = get_type_tree(*it);
    opencog::cassert(TRACE_INFO, !tmp.empty(), "tmp cannot be empty");
    if (it.is_childless()) {
        return tmp;
    } else {
        type_tree_pre_it head = tmp.begin();
        head = tmp.wrap(head, id::application_type);
        for (combo_tree::sibling_iterator sib = it.begin();
                sib != it.end(); ++sib) {
            type_tree arg_app = get_type_tree(tr,
                                              combo_tree::iterator(sib));
            type_tree_pre_it arg_app_it = arg_app.begin();
            type_tree_pre_it arg_it = tmp.append_child(head);
            tmp.replace(arg_it, arg_app_it);
        }
        return tmp;
    }
}

type_tree infer_type_tree(const combo_tree& tr)
{
    type_tree tt = get_type_tree(tr);
    argument_type_list arg_types;
    infer_arg_type_tree(tr, arg_types);
    reduce_type_tree(tt, arg_types, tr);
    insert_arg_type_tree(arg_types, tt);
    return tt;
}

bool is_well_formed(const type_tree& tt)
{
    if (tt.empty())
        return false;
    else for (type_tree_pre_it it = tt.begin(); it != tt.end(); ++it)
            if (*it == id::ill_formed_type)
                return false;
    return true;
}

bool does_contain_all_arg_up_to(const combo_tree& tr, arity_t n)
{
    typedef combo_tree::leaf_iterator leaf_it;
    typedef combo_tree::iterator pre_it;
    opencog::cassert(TRACE_INFO, !tr.empty(),
                      "cannot infer arg types on an empty combo_tree");
    opencog::cassert(TRACE_INFO, n >= 0, "Must be positive or null");
    std::vector<bool> bv(n, false);
    if (n > 0) {
        for (leaf_it lit = tr.begin_leaf(); lit != tr.end_leaf(); ++lit) {
            if (is_argument(*lit)) {
                int idx = get_argument(*lit).abs_idx_from_zero();

                if (idx < n)
                    bv[idx] = true;
                else return false;
            }
        }
        std::vector<bool> tbv(n, true);
        return tbv == bv;
    } else return true;
}

arity_t infer_arity(const combo_tree& tr)
{
    typedef combo_tree::leaf_iterator leaf_it;

    arity_t a = 0;

    for (leaf_it l_it = tr.begin_leaf(); l_it != tr.end_leaf(); ++l_it) {
        arity_t va = get_arity(*l_it);
        if (va > 0)
            a += va;
        else if (va < 0) {
            return va -a;
        }
    }
    return a;
}

arity_t explicit_arity(const combo_tree& tr)
{
    typedef combo_tree::leaf_iterator leaf_it;
    arity_t res = 0;
    for (leaf_it l_it = tr.begin_leaf(); l_it != tr.end_leaf(); ++l_it) {
        if (is_argument(*l_it))
            res = std::max(res,
                           static_cast<arity_t>(get_argument(*l_it).abs_idx()));
    }
    return res;
}

}//~namespace combo


std::ostream& operator<<(std::ostream& out, const combo::type_node& n)
{
    using namespace combo;
    switch (n) {
        //type operators
    case id::lambda_type:
        return out << "->";
    case id::application_type:
        return out << "application";
    case id::union_type:
        return out << "union";
    case id::arg_list_type:
        return out << "arg_list";
        //elementary types
    case id::boolean_type:
        return out << "boolean";
    case id::contin_type:
        return out << "contin";
    case id::action_result_type:
        return out << "action_result";
    case id::definite_object_type:
        return out << "definite_object";
    case id::action_definite_object_type:
        return out << "action_definite_object";
    case id::indefinite_object_type:
        return out << "indefinite_object";
    case id::message_type:
        return out << "message";
    case id::action_symbol_type:
        return out << "action_symbol";
    case id::wild_card_type:
        return out << "wild_card";
    case id::unknown_type:
        return out << "unknown";
    case id::ill_formed_type:
        return out << "ill_formed";
    default:
        if (n >= id::argument_type) {
            return out << "#" << n - (int)id::argument_type + 1;
        } else return out << "UNKNOWN_HANDLE";
    }
}

std::istream& operator>>(std::istream& in, combo::type_node& n)
{
    using namespace combo;
    std::string str;
    in >> str;
    opencog::cassert(TRACE_INFO, !str.empty(),
                      "str representation must not be empty.");
    //type operators
    if (str == "->" || str == "lambda" || str == "lambda_type")
        n = id::lambda_type;
    else if (str == "application" || str == "application_type")
        n = id::application_type;
    else if (str == "union" || str == "union_type")
        n = id::union_type;
    else if (str == "arg_list" || str == "arg_list_type")
        n = id::arg_list_type;
    //elementary types
    else if (str == "boolean" || str == "boolean_type")
        n = id::boolean_type;
    else if (str == "contin" || str == "contin_t" || str == "contin_type")
        n = id::contin_type;
    else if (str == "action_result" || str == "action_result_type")
        n = id::action_result_type;
    else if (str == "definite_object" || str == "definite_object_type")
        n = id::definite_object_type;
    else if (str == "action_definite_object" || str == "action_definite_object_type")
        n = id::action_definite_object_type;
    else if (str == "indefinite_object" || str == "indefinite_object_type")
        n = id::indefinite_object_type;
    else if (str == "message" || str == "message_type")
        n = id::message_type;
    else if (str == "action_symbol" || str == "action_symbol_type")
        n = id::action_symbol_type;
    else if (str == "wild_card" || str == "wild_card_type")
        n = id::wild_card_type;
    else if (str == "unknown" || str == "unknown_type")
        n = id::unknown_type;
    else if (str == "ill_formed" || str == "ill_formed_type")
        n = id::ill_formed_type;
    else if (str[0] == '#') {
        int arg = boost::lexical_cast<int>(str.substr(1));
        opencog::cassert(TRACE_INFO, arg != 0,
                          "arg value should be different from zero.");
        n = (type_node)((int)id::argument_type + arg - 1);
    } else {
        opencog::cassert(TRACE_INFO, false,
                          "type tree input stream fatal error : %s no such case",
                          str.c_str());
    }
    return in;
}

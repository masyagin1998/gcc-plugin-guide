// Copyright (C) 2019 Mikhail Masyagin

#include <iostream>
#include <sstream>
#include <string>

#include <gcc-plugin.h>
#include <plugin-version.h>

#include <coretypes.h>

#include <tree-pass.h>
#include <context.h>
#include <basic-block.h>

#include <tree.h>
#include <tree-ssa-alias.h>
#include <gimple-expr.h>
#include <gimple.h>
#include <gimple-ssa.h>
#include <tree-phinodes.h>
#include <tree-ssa-operands.h>

#include <ssa-iterators.h>
#include <gimple-iterator.h>

#define PREFIX_UNUSED(variable) ((void)variable)

int plugin_is_GPL_compatible = 1;

#define PLUGIN_NAME    "phi-debug"
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_HELP    "this plugin shows:\n"\
                       "* basic blocks;\n"\
                       "* GIMPLE instructions:\n"\
                       "  - arithmetic operations;\n"\
                       "  - phi-functions;\n"\
                       "  - branches;\n"\
                       "  - memory operations;"

static struct plugin_info phi_debug_plugin_info = {
    .version = PLUGIN_VERSION,
    .help =    PLUGIN_HELP,
};

static const struct pass_data phi_debug_pass_data = {
    .type =          GIMPLE_PASS,
    .name =          PLUGIN_NAME,
    .optinfo_flags = OPTGROUP_NONE,
    .tv_id =         TV_NONE,
    
    .properties_required =  PROP_gimple_any,
    .properties_provided =  0,
    .properties_destroyed = 0,
    
    .todo_flags_start =  0,
    .todo_flags_finish = 0,
};

struct phi_debug_pass : gimple_opt_pass {
    phi_debug_pass(gcc::context*ctx) : gimple_opt_pass(phi_debug_pass_data, ctx) {}
    virtual unsigned int execute(function*fun) override;
    virtual phi_debug_pass *clone() override { return this; }
};

static unsigned int phi_debug_bb_id(basic_block bb)
{
    std::cout << "\t" << "bb: ";
    edge e;
    edge_iterator it;
    std::cout << "(";
    std::stringstream src_stream;
    FOR_EACH_EDGE(e, it, bb->preds) {
        src_stream << e->src->index << ", ";
    }
    std::string src = src_stream.str();
    std::cout << src.substr(0, src.size() - 2);
    std::cout << ")";
    std::cout << " -> " <<
        "(" << bb->index << ")"
              << " -> ";
    std::cout << "(";
    std::stringstream dst_stream;
    FOR_EACH_EDGE(e, it, bb->succs) {
        dst_stream << e->dest->index << ", ";
    }
    std::string dst = dst_stream.str();
    std::cout << dst.substr(0, dst.size() - 2);
    std::cout << ")";
    
    return 0;
}

static unsigned int phi_debug_tree(tree t)
{   
    switch (TREE_CODE(t)) {
        // Constants.
    case INTEGER_CST:
        std::cout << TREE_INT_CST_LOW(t);
        break;
    case REAL_CST: {
        std::cout << "REAL_CST";
        break;
    }
    case FIXED_CST: {
        std::cout << "FIXED_CST";
        break;
    }
    case COMPLEX_CST:
        std::cout << "COMPLEX_CST";
        break;
    case VECTOR_CST:
        std::cout << "VECTOR_CST";
        break;
    case STRING_CST:
        std::cout << "\"" << TREE_STRING_POINTER(t) << "\"";
        break;
        // Declarations.
    case LABEL_DECL: 
        std::cout << (DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "unk_label_decl") << ":";
        break;
    case FIELD_DECL:
        std::cout << (DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "unk_field_decl");
        break;
    case VAR_DECL:
        std::cout << (DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "unk_var_decl");
        break;
    case CONST_DECL:
        std::cout << (DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "unk_const_decl");
        break;
        // Memory references.
    case COMPONENT_REF:
        phi_debug_tree(TREE_OPERAND(t, 0));
        std::cout << "->";
        phi_debug_tree(TREE_OPERAND(t, 1));
        break;
    case BIT_FIELD_REF:
        phi_debug_tree(TREE_OPERAND(t, 0));
        std::cout << "->";
        std::cout << "(";
        phi_debug_tree(TREE_OPERAND(t, 1));
        std::cout << " : ";
        phi_debug_tree(TREE_OPERAND(t, 2));
        std::cout << ")";
        break;
    case ARRAY_REF:
        phi_debug_tree(TREE_OPERAND(t, 0));
        std::cout << "[";
        phi_debug_tree(TREE_OPERAND(t, 1));
        std::cout << "]";
        break;
    case ARRAY_RANGE_REF:
        phi_debug_tree(TREE_OPERAND(t, 0));
        std::cout  << "[";
        phi_debug_tree(TREE_OPERAND(t, 1));
        std::cout << ":";
        // MAYBE RUNTIME EXCEPTION ?!!!
        phi_debug_tree(TREE_OPERAND(t, 2));
        std::cout << "]";
        break;
    case INDIRECT_REF:
        std::cout << "*";
        phi_debug_tree(TREE_OPERAND(t, 0));
        break;
    case CONSTRUCTOR:
        std::cout << "constructor";
        break;
    case ADDR_EXPR:
        std::cout << "&";
        phi_debug_tree(TREE_OPERAND(t, 0));
        break;
    case TARGET_MEM_REF:
        std::cout << "TMR(";
        std::cout << "BASE: ";
        phi_debug_tree(TREE_OPERAND(t, 0));
        std::cout << ", ";
        std::cout << "OFFSET: ";
        phi_debug_tree(TREE_OPERAND(t, 1));
        std::cout << ", ";
        std::cout << "STEP: ";
        phi_debug_tree(TREE_OPERAND(t, 2));
        std::cout << ", ";
        std::cout << "INDEX1: ";
        phi_debug_tree(TREE_OPERAND(t, 3));
        std::cout << ", ";
        std::cout << "INDEX2: ";
        phi_debug_tree(TREE_OPERAND(t, 4));
        std::cout << " )";
        break;
    case MEM_REF:
        std::cout << "((typeof(";
        phi_debug_tree(TREE_OPERAND(t, 1));
        std::cout << "))";
        phi_debug_tree(TREE_OPERAND(t, 0));
        std::cout << ")";
        break;
        // SSA-name.
    case SSA_NAME: {
        gimple stmt = SSA_NAME_DEF_STMT(t);
        if (gimple_code(stmt) == GIMPLE_PHI) {
            std::cout << "(" << (SSA_NAME_IDENTIFIER(t) ? IDENTIFIER_POINTER(SSA_NAME_IDENTIFIER(t)) : "unk_ssa_name") <<
            "__v" << SSA_NAME_VERSION(t);
            std::cout << " = GIMPLE_PHI(";
            for (unsigned int i = 0; i < gimple_phi_num_args(stmt); i++) {
                phi_debug_tree(gimple_phi_arg(stmt, i)->def);
                if (i != gimple_phi_num_args(stmt) - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << "))";
        } else {
            std::cout << (SSA_NAME_IDENTIFIER(t) ? IDENTIFIER_POINTER(SSA_NAME_IDENTIFIER(t)) : "unk_ssa_name") <<
                "__v" << SSA_NAME_VERSION(t);
        }
        
        break;
    }
    default:
        std::cout << "unk_tree_code(" << TREE_CODE(t) << ")";
        break;
    }

    return 0;
}

static unsigned int phi_debug_op(enum tree_code code)
{
    switch (code) {
        // Arithmetical operators.
    case POINTER_PLUS_EXPR:
    case PLUS_EXPR:
        std::cout << "+";
        break;
    case NEGATE_EXPR:
    case MINUS_EXPR:
        std::cout << "-";
        break;
    case MULT_EXPR:
        std::cout << "*";
        break;
    case TRUNC_DIV_EXPR:
    case CEIL_DIV_EXPR:
    case FLOOR_DIV_EXPR:
    case ROUND_DIV_EXPR:
    case EXACT_DIV_EXPR:
    case RDIV_EXPR:
        std::cout << "/";
        break;
        // Bit-shift operators.
    case LSHIFT_EXPR:
        std::cout << "<<";
        break;
    case RSHIFT_EXPR:
        std::cout << ">>";
        break;
        // Bit-logical operators.
    case BIT_IOR_EXPR:
        std::cout << "|";
        break;
    case BIT_XOR_EXPR:
        std::cout << "^";
        break;
    case BIT_AND_EXPR:
        std::cout << "&";
        break;
    case BIT_NOT_EXPR:
        std::cout << "!";
        break;
        // Truth-logical operators.
    case TRUTH_ANDIF_EXPR:
    case TRUTH_AND_EXPR:
        std::cout << "&&";
        break;
    case TRUTH_ORIF_EXPR:
    case TRUTH_OR_EXPR:
        std::cout << "||";
        break;
    case TRUTH_XOR_EXPR:
        std::cout << "^^";
        break;
    case TRUTH_NOT_EXPR:
        std::cout << "!";
        // Relational operators.
    case LT_EXPR:
    case UNLT_EXPR:
        std::cout << "<";
        break;
    case LE_EXPR:
    case UNLE_EXPR:
        std::cout << "<=";
        break;
    case GT_EXPR:
    case UNGT_EXPR:
        std::cout << ">";
        break;
    case GE_EXPR:
    case UNGE_EXPR:
        std::cout << ">=";
        break;
    case EQ_EXPR:
    case UNEQ_EXPR:
        std::cout << "==";
        break;
    case NE_EXPR:
    case LTGT_EXPR:
        std::cout << "!=";
        break;
    case UNORDERED_EXPR:
        std::cout << "unord";
        break;
    case ORDERED_EXPR:
        std::cout << "ord";
        break;
        // Unknown operators.
    default:
        std::cout << "?(" << code << ")?";
        break;
    }
    return 0;
}

static unsigned int phi_debug_on_gimple_assign(gimple stmt)
{
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_ASSIGN\"" << " " << "(" << GIMPLE_ASSIGN << ")" << " { ";
    switch (gimple_num_ops(stmt)) {
    case 2:
        phi_debug_tree(gimple_assign_lhs(stmt));
        std::cout << " = ";
        phi_debug_tree(gimple_assign_rhs1(stmt));
        break;
    case 3:
        phi_debug_tree(gimple_assign_lhs(stmt));
        std::cout << " = ";
        phi_debug_tree(gimple_assign_rhs1(stmt));
        std::cout << " ";
        phi_debug_op(gimple_assign_rhs_code(stmt));
        std::cout << " ";
        phi_debug_tree(gimple_assign_rhs2(stmt));
        break;
    }
    std::cout << " }" << std::endl;
    
    return 0;
}

static unsigned int phi_debug_on_gimple_call(gimple stmt)
{
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_CALL\"" << " " << "(" << GIMPLE_CALL << ")" << " { ";
    tree lhs = gimple_call_lhs (stmt);
    if (lhs) {
        phi_debug_tree(lhs);
        printf(" = ");
    }
    std::cout << fndecl_name(gimple_call_fndecl(stmt)) << "(";
    for (unsigned int i = 0; i < gimple_call_num_args(stmt); i++) {
        phi_debug_tree(gimple_call_arg(stmt, i));
        if (i != gimple_call_num_args(stmt) - 1) {
            std::cout << ", ";
        }
    }
    std::cout << ")";
    std::cout << " }" << std::endl;
    
    return 0;
}

/*
static unsigned int phi_debug_on_gimple_phi(gimple stmt)
{
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_PHI\"" << " "<< "(" << GIMPLE_PHI << ")" << " {";
    for (unsigned int i = 0; i < gimple_phi_num_args(stmt); i++) {
        phi_debug_tree(gimple_phi_arg(stmt, i)->def);
        if (i != gimple_phi_num_args(stmt) - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "}" << std::endl;
    
    return 0;
}
*/

static unsigned int phi_debug_on_gimple_cond(gimple stmt)
{
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_COND\"" << " " << "(" << GIMPLE_COND << ")" << " { ";
    phi_debug_tree(gimple_cond_lhs(stmt));
    std::cout << " ";
    phi_debug_op(gimple_assign_rhs_code(stmt));
    std::cout << " ";
    phi_debug_tree(gimple_cond_rhs(stmt));
    std::cout << " }" << std::endl;
    
    return 0;
}

static unsigned int phi_debug_on_gimple_label(gimple stmt) {
    PREFIX_UNUSED(stmt);
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_LABEL\"" << " " << "(" << GIMPLE_LABEL << ")" << " {";
    std::cout << "}" << std::endl;
    
    return 0;
}

static unsigned int phi_debug_on_gimple_return(gimple stmt)
{
    PREFIX_UNUSED(stmt);
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_RETURN\"" << " " << "(" << GIMPLE_RETURN << ")" << " {";
    std::cout << "}" << std::endl;
        
    return 0;
}

static unsigned int phi_debug_on_unknown_stmt(gimple stmt)
{
    std::cout << "\t\t" << "stmt: " << "\"GIMPLE_UNKNOWN\"" << " " << "(" << gimple_code(stmt) << ")" << " {}" << std::endl;
    
    return 0;
}


static unsigned int phi_debug_statements(basic_block bb)
{
    for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
        gimple stmt = gsi_stmt(gsi);

        switch (gimple_code(stmt)) {
        case GIMPLE_ASSIGN:
            phi_debug_on_gimple_assign(stmt);
            break;
        case GIMPLE_CALL:
            phi_debug_on_gimple_call(stmt);
            break;
            /*
        case GIMPLE_PHI:
            phi_debug_on_gimple_phi(stmt);
            break;
            */
        case GIMPLE_COND:
            phi_debug_on_gimple_cond(stmt);
            break;
        case GIMPLE_LABEL:
            phi_debug_on_gimple_label(stmt);
            break;
        case GIMPLE_RETURN:
            phi_debug_on_gimple_return(stmt);
            break;
        default:
            phi_debug_on_unknown_stmt(stmt);
            break;
        }
    }
             
    return 0;
}

static unsigned int phi_debug_function(function*fn)
{
    std::cout << "func: " << "\"" << function_name(fn) << "\"" << " {" << std::endl;
    basic_block bb;
    FOR_EACH_BB_FN(bb, fn) {
        phi_debug_bb_id(bb);
        std::cout << " {" <<  std::endl;
        phi_debug_statements(bb);
        std::cout << "\t" << "}" << std::endl;
    }
    std::cout << "}" << std::endl;
    std::cout << std::endl;
    return 0;
}

unsigned int phi_debug_pass::execute(function*fn) { return phi_debug_function(fn); }

static struct register_pass_info phi_debug_pass_info = {
    .pass =                     new phi_debug_pass(g),
    .reference_pass_name =      "ssa",
    .ref_pass_instance_number = 1,
    .pos_op =                   PASS_POS_INSERT_AFTER,
};

int plugin_init(struct plugin_name_args *args, struct plugin_gcc_version *version)
{
    if(!plugin_default_version_check(version, &gcc_version)) {
        return 1;
    }
    
    register_callback(args->base_name, PLUGIN_INFO, NULL, &phi_debug_plugin_info);
    register_callback(args->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &phi_debug_pass_info);
    
    return 0;
}

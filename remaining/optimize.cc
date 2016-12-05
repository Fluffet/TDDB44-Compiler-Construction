#include "optimize.hh"

/*** This file contains all code pertaining to AST optimisation. It currently
     implements a simple optimisation called "constant folding". Most of the
     methods in this file are empty, or just relay optimize calls downward
     in the AST. If a more powerful AST optimization scheme were to be
     implemented, only methods in this file should need to be changed. ***/


ast_optimizer *optimizer = new ast_optimizer();


/* The optimizer's interface method. Starts a recursive optimize call down
   the AST nodes, searching for binary operators with constant children. */
void ast_optimizer::do_optimize(ast_stmt_list *body)
{
    if (body != NULL) {
        body->optimize();
    }
}


/* Returns 1 if an AST expression is a subclass of ast_binaryoperation,
   ie, eligible for constant folding. */
bool ast_optimizer::is_binop(ast_expression *node)
{
    switch (node->tag) {
    case AST_ADD:
    case AST_SUB:
    case AST_OR:
    case AST_AND:
    case AST_MULT:
    case AST_DIVIDE:
    case AST_IDIV:
    case AST_MOD:
        return true;
    default:
        return false;
    }
}



/* We overload this method for the various ast_node subclasses that can
   appear in the AST. By use of virtual (dynamic) methods, we ensure that
   the correct method is invoked even if the pointers in the AST refer to
   one of the abstract classes such as ast_expression or ast_statement. */
void ast_node::optimize()
{
    fatal("Trying to optimize abstract class ast_node.");
}

void ast_statement::optimize()
{
    fatal("Trying to optimize abstract class ast_statement.");
}

void ast_expression::optimize()
{
    fatal("Trying to optimize abstract class ast_expression.");
}

void ast_lvalue::optimize()
{
    fatal("Trying to optimize abstract class ast_lvalue.");
}

void ast_binaryoperation::optimize()
{
    fatal("Trying to optimize abstract class ast_binaryoperation.");
}

void ast_binaryrelation::optimize()
{
    fatal("Trying to optimize abstract class ast_binaryrelation.");
}



/*** The optimize methods for the concrete AST classes. ***/

/* Optimize a statement list. */
void ast_stmt_list::optimize()
{
    if (preceding != NULL) {
        preceding->optimize();
    }
    if (last_stmt != NULL) {
        last_stmt->optimize();
    }
}


/* Optimize a list of expressions. */
void ast_expr_list::optimize()
{
    /* Your code here */
    if (preceding != NULL) {
        preceding->optimize();
    }
    if (last_expr != NULL) {
        last_expr->optimize();
    }

}


/* Optimize an elsif list. */
void ast_elsif_list::optimize()
{
    /* Your code here */
    if (preceding != NULL) {
        preceding->optimize();
    }
    if (last_elsif != NULL) {
        last_elsif->optimize();
    }
}


/* An identifier's value can change at run-time, so we can't perform
   constant folding optimization on it unless it is a constant.
   Thus we just do nothing here. It can be treated in the fold_constants()
   method, however. */
void ast_id::optimize()
{
}

void ast_indexed::optimize()
{
    /* Your code here */
    index->optimize();
}

// Our own implementation
void ast_optimizer::ghett0_optimize_binop(ast_binaryoperation *node)
{
    node->left->optimize();
    node->right->optimize();
    ast_expression *temp = node->left;
    node->left = optimizer->fold_constants(node->left);
    delete temp;
    temp = node->right;
    node->right = optimizer->fold_constants(node->right);
    delete temp;
}

/* This convenience method is used to apply constant folding to all
   binary operations. It returns either the resulting optimized node or the
   original node if no optimization could be performed. */
ast_expression *ast_optimizer::fold_constants(ast_expression *node)
{
    /* Your code here */
    if (node->tag == AST_ID)
    {
        ast_id *id = node->get_ast_id();
        if (sym_tab->get_symbol_tag(id->sym_p) == SYM_CONST)
        {
            constant_symbol *sym = sym_tab->get_symbol(id->sym_p)->get_constant_symbol();
            if (sym->type == integer_type)
            {
                return new ast_integer(id->pos, sym->const_value.ival);
            }
            else
            {
                return new ast_real(id->pos, sym->const_value.rval);
            }
        }
    }

    if ( is_binop(node) )
    {
        ast_binaryoperation *binop = node->get_ast_binaryoperation();
        ast_expression *l = binop->left;
        ast_expression *r = binop->right;


        switch (node->tag) {
        case AST_ADD:
            if (l->tag == AST_INTEGER && r->tag == AST_INTEGER) {
                return new ast_integer(l->pos, l->get_ast_integer()->value + r->get_ast_integer()->value);
            }
            else if (l->tag == AST_INTEGER && r->tag == AST_REAL) {
                return new ast_real(l->pos, l->get_ast_integer()->value + r->get_ast_real()->value);
            }
            else if (l->tag == AST_REAL && r->tag == AST_INTEGER) {
                return new ast_real(l->pos, l->get_ast_real()->value + r->get_ast_integer()->value);
            }
            else if (l->tag == AST_REAL && r->tag == AST_REAL) {
                return new ast_real(l->pos, l->get_ast_real()->value + r->get_ast_real()->value);
            }
            break;
        case AST_SUB:
            if (l->tag == AST_INTEGER && r->tag == AST_INTEGER) {
                return new ast_integer(l->pos, l->get_ast_integer()->value - r->get_ast_integer()->value);
            }
            else if (l->tag == AST_INTEGER && r->tag == AST_REAL) {
                return new ast_real(l->pos, l->get_ast_integer()->value - r->get_ast_real()->value);
            }
            else if (l->tag == AST_REAL && r->tag == AST_INTEGER) {
                return new ast_real(l->pos, l->get_ast_real()->value - r->get_ast_integer()->value);
            }
            else if (l->tag == AST_REAL && r->tag == AST_REAL) {
                return new ast_real(l->pos, l->get_ast_real()->value - r->get_ast_real()->value);
            }
            break;
        case AST_OR:
            if (l->tag == AST_INTEGER && r->tag == AST_INTEGER) {
                return new ast_integer(l->pos, l->get_ast_integer()->value or r->get_ast_integer()->value);
            }
            else if (l->tag == AST_INTEGER && r->tag == AST_REAL) {
                return new ast_real(l->pos, l->get_ast_integer()->value or r->get_ast_real()->value);
            }
            else if (l->tag == AST_REAL && r->tag == AST_INTEGER) {
                return new ast_real(l->pos, l->get_ast_real()->value or r->get_ast_integer()->value);
            }
            else if (l->tag == AST_REAL && r->tag == AST_REAL) {
                return new ast_real(l->pos, l->get_ast_real()->value or r->get_ast_real()->value);
            }
            break;
        case AST_AND:
            if (l->tag == AST_INTEGER && r->tag == AST_INTEGER) {
                return new ast_integer(l->pos, l->get_ast_integer()->value and r->get_ast_integer()->value);
            }
            else if (l->tag == AST_INTEGER && r->tag == AST_REAL) {
                return new ast_real(l->pos, l->get_ast_integer()->value and r->get_ast_real()->value);
            }
            else if (l->tag == AST_REAL && r->tag == AST_INTEGER) {
                return new ast_real(l->pos, l->get_ast_real()->value and r->get_ast_integer()->value);
            }
            else if (l->tag == AST_REAL && r->tag == AST_REAL) {
                return new ast_real(l->pos, l->get_ast_real()->value and r->get_ast_real()->value);
            }
            break;
        case AST_MULT:
            if (l->tag == AST_INTEGER && r->tag == AST_INTEGER) {
                return new ast_integer(l->pos, l->get_ast_integer()->value * r->get_ast_integer()->value);
            }
            else if (l->tag == AST_INTEGER && r->tag == AST_REAL) {
                return new ast_real(l->pos, l->get_ast_integer()->value * r->get_ast_real()->value);
            }
            else if (l->tag == AST_REAL && r->tag == AST_INTEGER) {
                return new ast_real(l->pos, l->get_ast_real()->value * r->get_ast_integer()->value);
            }
            else if (l->tag == AST_REAL && r->tag == AST_REAL) {
                return new ast_real(l->pos, l->get_ast_real()->value * r->get_ast_real()->value);
            }
            break;
        case AST_DIVIDE:
            if (l->tag == AST_REAL && r->tag == AST_REAL) {
                return new ast_real(l->pos, l->get_ast_real()->value / r->get_ast_real()->value);
            }
            break;
        case AST_IDIV:
            if (l->tag == AST_INTEGER && r->tag == AST_INTEGER) {
                return new ast_integer(l->pos, l->get_ast_integer()->value / r->get_ast_integer()->value);
            }
            break;
        case AST_MOD:
            if (l->tag == AST_INTEGER && r->tag == AST_INTEGER) {
                return new ast_integer(l->pos, l->get_ast_integer()->value % r->get_ast_integer()->value);
            }
            break;
        default:
            break;
        }
        
    }


    return node;
}

/* All the binary operations should already have been detected in their parent
   nodes, so we don't need to do anything at all here. */
void ast_add::optimize()
{
    /* Your code here */
    optimizer->ghett0_optimize_binop(this);
}

void ast_sub::optimize()
{
    /* Your code here */
    optimizer->ghett0_optimize_binop(this);
}

void ast_mult::optimize()
{
    /* Your code here */
    optimizer->ghett0_optimize_binop(this);
}

void ast_divide::optimize()
{
    /* Your code here */
    optimizer->ghett0_optimize_binop(this);
}

void ast_or::optimize()
{
    /* Your code here */
    optimizer->ghett0_optimize_binop(this);
}

void ast_and::optimize()
{
    /* Your code here */
    optimizer->ghett0_optimize_binop(this);
}

void ast_idiv::optimize()
{
    /* Your code here */
    optimizer->ghett0_optimize_binop(this);
}

void ast_mod::optimize()
{
    /* Your code here */
    optimizer->ghett0_optimize_binop(this);
}



/* We can apply constant folding to binary relations as well. */
void ast_equal::optimize()
{
    /* Your code here */
    left->optimize();
    left = optimizer->fold_constants(left);
    right->optimize();
    right = optimizer->fold_constants(right);
}

void ast_notequal::optimize()
{
    /* Your code here */
    left->optimize();
    left = optimizer->fold_constants(left);
    right->optimize();
    right = optimizer->fold_constants(right);
}

void ast_lessthan::optimize()
{
    /* Your code here */
    left->optimize();
    left = optimizer->fold_constants(left);
    right->optimize();
    right = optimizer->fold_constants(right);

}

void ast_greaterthan::optimize()
{
    /* Your code here */
    left->optimize();
    left = optimizer->fold_constants(left);
    right->optimize();
    right = optimizer->fold_constants(right);
}



/*** The various classes derived from ast_statement. ***/

void ast_procedurecall::optimize()
{
    /* Your code here */
    parameter_list->optimize();
}


void ast_assign::optimize()
{
    /* Your code here */
    rhs->optimize();
    rhs = optimizer->fold_constants(rhs);

}


void ast_while::optimize()
{
    /* Your code here */
    condition->optimize();
    condition = optimizer->fold_constants(condition);
    body->optimize();
}


void ast_if::optimize()
{
    /* Your code here */
    condition->optimize();
    condition = optimizer->fold_constants(condition);
    body->optimize();
    if (elsif_list != NULL)
    {
        elsif_list->optimize();
    }
    if (else_body != NULL)
    {
        else_body->optimize();
    }
}


void ast_return::optimize()
{
    /* Your code here */
    value->optimize();
    value = optimizer->fold_constants(value);
}


void ast_functioncall::optimize()
{
    /* Your code here */
    parameter_list->optimize();
}

void ast_uminus::optimize()
{
    /* Your code here */
    expr->optimize();
    expr = optimizer->fold_constants(expr);
}

void ast_not::optimize()
{
    /* Your code here */
    expr->optimize();
    expr = optimizer->fold_constants(expr);
}


void ast_elsif::optimize()
{
    /* Your code here */
    condition->optimize();
    condition = optimizer->fold_constants(condition);
    body->optimize();
}



void ast_integer::optimize()
{
    /* Your code here */
}

void ast_real::optimize()
{
    /* Your code here */
}

/* Note: See the comment in fold_constants() about casts and folding. */
void ast_cast::optimize()
{
    /* Your code here */
}



void ast_procedurehead::optimize()
{
    fatal("Trying to call ast_procedurehead::optimize()");
}


void ast_functionhead::optimize()
{
    fatal("Trying to call ast_functionhead::optimize()");
}

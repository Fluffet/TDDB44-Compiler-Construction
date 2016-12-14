#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "symtab.hh"
#include "quads.hh"
#include "codegen.hh"

using namespace std;

// Defined in main.cc.
extern bool assembler_trace;

#define STREAM out

// Used in parser.y. Ideally the filename should be parametrized, but it's not
// _that_ important...
code_generator *code_gen = new code_generator("d.out");

// Constructor.
code_generator::code_generator(const string object_file_name)
{
    out.open(object_file_name);

    reg[RAX] = "rax";
    reg[RCX] = "rcx";
    reg[RDX] = "rdx";
}


/* Destructor. */
code_generator::~code_generator()
{
    // Make sure we close the outfile before exiting the compiler.
    out << flush;
    out.close();
}



/* This method is called from parser.y when code generation is to start.
   The argument is a quad_list representing the body of the procedure, and
   the symbol for the environment for which code is being generated. */
void code_generator::generate_assembler(quad_list *q, symbol *env)
{
    prologue(env);
    expand(q);
    epilogue(env);
}



/* This method aligns a frame size on an 8-byte boundary. Used by prologue().
 */
int code_generator::align(int frame_size)
{
    return ((frame_size + 7) / 8) * 8;
}



/* This method generates assembler code for initialisating a procedure or
   function. */
void code_generator::prologue(symbol *new_env)
{
    int ar_size;
    int label_nr;
    // Used to count parameters.
    //parameter_symbol *last_arg;

    block_level level;

    // Again, we need a safe downcast for a procedure/function.
    // Note that since we have already generated quads for the entire block
    // before we expand it to assembler, the size of the activation record
    // is known here (ar_size).
    if (new_env->tag == SYM_PROC) {
        procedure_symbol *proc = new_env->get_procedure_symbol();
        ar_size = align(proc->ar_size);
        label_nr = proc->label_nr;
        //last_arg = proc->last_parameter;
        level = proc->level;
    } else if (new_env->tag == SYM_FUNC) {
        function_symbol *func = new_env->get_function_symbol();
        /* Make sure ar_size is a multiple of eight */
        ar_size = align(func->ar_size);
        label_nr = func->label_nr;
        //last_arg = func->last_parameter;
        level = func->level;
    } else {
        fatal("code_generator::prologue() called for non-proc/func");
        return;
    }

    /* Print out the label number (a SYM_PROC/ SYM_FUNC attribute) */
    STREAM << "L" << label_nr << ":" << "\t\t\t" << "# " <<
        /* Print out the function/procedure name */
        sym_tab->pool_lookup(new_env->id) << endl;

    if (assembler_trace) {
        STREAM << "\t" << "# PROLOGUE (" << short_symbols << new_env
            << long_symbols << ")" << endl;
    }

    /* Your code here */

    // store previous rbp, save previous rsp
    STREAM << "\t\t" << "push" << "\t" << "rbp" << endl;
    STREAM << "\t\t" << "mov" << "\t" << "rcx, rsp" << endl;

    // copy display values
    for (int i = 1; i <= level; i++){
        STREAM << "\t\t" << "push" << "\t" << "[rbp-" << i*STACK_WIDTH << "]" << endl;
    }


    //push previous rsp on stack
    STREAM << "\t\t" << "push" << "\t" << "rcx" << endl;
    STREAM << "\t\t" << "mov" << "\t" << "rbp, rcx" << endl;
    STREAM << "\t\t" << "sub" << "\t" << "rsp, " << ar_size << endl;
    

    STREAM << flush;
}



/* This method generates assembler code for leaving a procedure or function. */
void code_generator::epilogue(symbol *old_env)
{
    if (assembler_trace) {
        STREAM << "\t" << "# EPILOGUE (" << short_symbols << old_env
            << long_symbols << ")" << endl;
    }

    /* Your code here */

    STREAM << "\t\t" << "leave" << "\t" << endl;
    STREAM << "\t\t" << "ret" << "\t" << endl;
    
    STREAM << flush;
}



/* This function finds the display register level and offset for a variable,
   array or a parameter. Note the pass-by-pointer arguments. */
void code_generator::find(sym_index sym_p, int *level, int *offset)
{
    /* Your code here */
    symbol *sym = sym_tab->get_symbol(sym_p);
    *level = sym->level;
    sym_type tag = sym->tag;
    if (tag == SYM_VAR || tag == SYM_ARRAY)
    {
        //printf("Variable: \"%s\". Symbol offset: %d\n", sym_tab->pool_lookup(sym->id), sym->offset);
    
        //Offset for local variable's are the display area plus it's internal offset
        *offset = -((*level+1)*STACK_WIDTH + sym->offset);

    }
    else if (tag == SYM_PARAM)
    {   
        // Jump over previous return address + the offset of the param
        // in the param list. Then get to the start of the param b its size
        //printf("Parameter: \"%s\". Symbol offset: %d\n", sym_tab->pool_lookup(sym->id), sym->offset);
        *offset = STACK_WIDTH + sym->offset + sym->get_parameter_symbol()->size;
    }
}

/*
 * Generates code for getting the address of a frame for the specified scope level.
 */
void code_generator::frame_address(int level, const register_type dest)
{
    /* Your code here */
    STREAM << "\t\t" << "mov" << "\t" << reg[dest] << ", [rbp-" << (level)*STACK_WIDTH << "]" << endl;
}

/* This function fetches the value of a variable or a constant into a
   register. */
void code_generator::fetch(sym_index sym_p, register_type dest)
{
    /* Your code here */
    symbol *sym = sym_tab->get_symbol(sym_p);
    sym_type tag = sym->tag;
    if (tag == SYM_CONST) 
    {
        constant_symbol *cs = sym->get_constant_symbol();
        long value;
        if (cs->type == real_type)
        {
            value = sym_tab->ieee(cs->const_value.rval);
        }
        else
        {
            value = cs->const_value.ival;
        }
        STREAM << "\t\t" << "mov " << reg[dest] << ", " << value << endl;
    }
    else if (tag == SYM_VAR)
    {
        int level, offset;
        find(sym_p, &level, &offset);
        frame_address(level, RCX);
        STREAM << "\t\t" << "mov" << "\t" << reg[dest] << ", [rcx";
        if (offset > 0)
        {
            STREAM << "+" << offset;
        }
        else if (offset < 0)
        {
            STREAM << offset;
        }
        STREAM << "]" << endl;
    }
    else if (tag == SYM_PARAM)
    {
        int level, offset;
        find(sym_p, &level, &offset);
        frame_address(level, RCX);
        STREAM << "\t\t" << "mov" << "\t" << reg[dest] << ", [rcx";
        if (offset > 0)
        {
            STREAM << "+" << offset;
        }
        else if (offset < 0)
        {
            STREAM << offset;
        }
        STREAM << "]" << endl;
    }

}

void code_generator::fetch_float(sym_index sym_p)
{
    /* Your code here */
    symbol *sym = sym_tab->get_symbol(sym_p);
    sym_type tag = sym->tag;
    if (tag == SYM_CONST) 
    {
        constant_symbol *cs = sym->get_constant_symbol();
        long value = sym_tab->ieee(cs->const_value.rval);
        STREAM << "\t\t" << "sub" << "\t" << "rsp, " << STACK_WIDTH << endl;
        STREAM << "\t\t" << "push" << "\t" << value << endl;
        STREAM << "\t\t" << "fld qword ptr" << "\t" << "[rsp]" << endl;
        STREAM << "\t\t" << "add" << "\t" << "rsp, " << STACK_WIDTH << endl;
    }
    else if (tag == SYM_VAR)
    {
        int level, offset;
        find(sym_p, &level, &offset);
        frame_address(level, RCX);
        
        STREAM << "\t\t" << "fld qword ptr" << "\t" << "[rcx";
        if (offset > 0)
        {
            STREAM << "+" << offset;
        }
        else if (offset < 0)
        {
            STREAM << offset;
        }
        STREAM << "]" << endl;

        //STREAM << "\t\t" << "fld" << "\t" << "rsp" << endl;
        

    }
}



/* This function stores the value of a register into a variable. */
void code_generator::store(register_type src, sym_index sym_p)
{
    /* Your code here */

    int level, offset;
    find(sym_p, &level, &offset);
    frame_address(level, RCX);
    STREAM << "\t\t" << "mov" << "\t"  << "[rcx";
    if (offset > 0)
    {
        STREAM << "+" << offset;
    }
    else if (offset < 0)
    {
        STREAM << offset;
    }
    STREAM << "], " << reg[src] << endl ;

}

void code_generator::store_float(sym_index sym_p)
{
    /* Your code here */
    int level, offset;
    find(sym_p, &level, &offset);
    frame_address(level, RCX);
    STREAM << "\t\t" << "fstp qword ptr" << "\t"  << "[rbp";
    if (offset > 0)
    {
        STREAM << "+" << offset;
    }
    else if (offset < 0)
    {
        STREAM << offset;
    }
    STREAM << "]" << endl ;
}


/* This function fetches the base address of an array. */
void code_generator::array_address(sym_index sym_p, register_type dest)
{
    /* Your code here */
    //array_symbol *arr_s = sym_tab->get_symbol(sym_p)->get_array_symbol();
    int level, offset;
    find(sym_p, &level, &offset);
    frame_address(level, RCX);
    //STREAM << "\t\t" << "mov" << "\t" << "rcx, rbp" << endl;
    if (offset > 0)
    {
        STREAM << "\t\t" << "add" << "\t" << "rcx, " << offset << endl; //TODO: Dafuq. add by negative value?
    }
    else
    {
        STREAM << "\t\t" << "sub" << "\t" << "rcx, " << -offset << endl; //TODO: Dafuq. add by negative value?
    }
    STREAM << "\t\t" << "mov" << "\t" << reg[dest] << ", rcx" << endl;

}

/* This method expands a quad_list into assembler code, quad for quad. */
void code_generator::expand(quad_list *q_list)
{
    long quad_nr = 0;       // Just to make debug STREAMput easier to read.
    

    // We use this iterator to loop through the quad list.
    quad_list_iterator *ql_iterator = new quad_list_iterator(q_list);

    quadruple *q = ql_iterator->get_current(); // This is the head of the list.

    while (q != NULL) {
        quad_nr++;

        // We always do labels here so that a branch doesn't miss the
        // trace code.
        if (q->op_code == q_labl) {
            STREAM<< "L" << q->int1 << ":" << endl;
        }

        // Debug output.
        if (assembler_trace) {
            STREAM<< "\t" << "# QUAD " << quad_nr << ": "
                << short_symbols << q << long_symbols << endl;
        }

        // The main switch on quad type. This is where code is actually
        // generated.
        switch (q->op_code) {
        case q_rload:
        case q_iload:
            STREAM<< "\t\t" << "mov" << "\t" << "rax, " << q->int1 << endl;
            store(RAX, q->sym3);
            break;

        case q_inot: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            STREAM<< "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "je" << "\t" << "L" << label << endl;
            // Not equal branch
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // Equal branch
            STREAM<< "\t\t" << "L" << label << ":" << endl;
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            STREAM<< "\t\t" << "L" << label2 << ":" <<  endl;
            store(RAX, q->sym3);
            break;
        }
        case q_ruminus:
            fetch_float(q->sym1);
            STREAM<< "\t\t" << "fchs" << endl;
            store_float(q->sym3);
            break;

        case q_iuminus:
            fetch(q->sym1, RAX);
            STREAM<< "\t\t" << "neg" << "\t" << "rax" << endl;
            store(RAX, q->sym3);
            break;

        case q_rplus:
            fetch_float(q->sym1);
            fetch_float(q->sym2);
            STREAM<< "\t\t" << "faddp" << endl;
            store_float(q->sym3);
            break;

        case q_iplus:
            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            STREAM<< "\t\t" << "add" << "\t" << "rax, rcx" << endl;
            store(RAX, q->sym3);
            break;

        case q_rminus:
            fetch_float(q->sym1);
            fetch_float(q->sym2);
            STREAM<< "\t\t" << "fsubp" << endl;
            store_float(q->sym3);
            break;

        case q_iminus:
            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            STREAM<< "\t\t" << "sub" << "\t" << "rax, rcx" << endl;
            store(RAX, q->sym3);
            break;

        case q_ior: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            STREAM<< "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "jne" << "\t" << "L" << label << endl;
            fetch(q->sym2, RAX);
            STREAM<< "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "jne" << "\t" << "L" << label << endl;
            // False branch
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            STREAM<< "\t\t" << "L" << label << ":" << endl;
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            STREAM<< "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_iand: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            STREAM<< "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "je" << "\t" << "L" << label << endl;
            fetch(q->sym2, RAX);
            STREAM<< "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "je" << "\t" << "L" << label << endl;
            // True branch
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 1" << endl;
            STREAM<< "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // False branch
            STREAM<< "\t\t" << "L" << label << ":" << endl;
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 0" << endl;

            STREAM<< "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_rmult:
            fetch_float(q->sym1);
            fetch_float(q->sym2);
            STREAM<< "\t\t" << "fmulp" << endl;
            store_float(q->sym3);
            break;

        case q_imult:
            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            STREAM<< "\t\t" << "imul" << "\t" << "rax, rcx" << endl;
            store(RAX, q->sym3);
            break;

        case q_rdivide:
            fetch_float(q->sym1);
            fetch_float(q->sym2);
            STREAM<< "\t\t" << "fdivp" << endl;
            store_float(q->sym3);
            break;

        case q_idivide:
            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            STREAM<< "\t\t" << "cqo" << endl;
            STREAM<< "\t\t" << "idiv" << "\t" << "rax, rcx" << endl;
            store(RAX, q->sym3);
            break;

        case q_imod:
            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            STREAM<< "\t\t" << "cqo" << endl;
            STREAM<< "\t\t" << "idiv" << "\t" << "rax, rcx" << endl;
            store(RDX, q->sym3);
            break;

        case q_req: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch_float(q->sym1);
            fetch_float(q->sym2);
            STREAM<< "\t\t" << "fcomip" << "\t" << "ST(0), ST(1)" << endl;
            // Clear the stack
            STREAM<< "\t\t" << "fstp" << "\t" << "ST(0)" << endl;
            STREAM<< "\t\t" << "je" << "\t" << "L" << label << endl;
            // False branch
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            STREAM<< "\t\t" << "L" << label << ":" << endl;
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            STREAM<< "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_ieq: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            STREAM<< "\t\t" << "cmp" << "\t" << "rax, rcx" << endl;
            STREAM<< "\t\t" << "je" << "\t" << "L" << label << endl;
            // False branch
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            STREAM<< "\t\t" << "L" << label << ":" << endl;
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            STREAM<< "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_rne: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch_float(q->sym1);
            fetch_float(q->sym2);
            STREAM<< "\t\t" << "fcomip" << "\t" << "ST(0), ST(1)" << endl;
            // Clear the stack
            STREAM<< "\t\t" << "fstp" << "\t" << "ST(0)" << endl;
            STREAM<< "\t\t" << "jne" << "\t" << "L" << label << endl;
            // False branch
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            STREAM<< "\t\t" << "L" << label << ":" << endl;
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            STREAM<< "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_ine: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            STREAM<< "\t\t" << "cmp" << "\t" << "rax, rcx" << endl;
            STREAM<< "\t\t" << "jne" << "\t" << "L" << label << endl;
            // False branch
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            STREAM<< "\t\t" << "L" << label << ":" << endl;
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            STREAM<< "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_rlt: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            // We need to push in reverse order for this to work
            fetch_float(q->sym2);
            fetch_float(q->sym1);
            STREAM<< "\t\t" << "fcomip" << "\t" << "ST(0), ST(1)" << endl;
            // Clear the stack
            STREAM<< "\t\t" << "fstp" << "\t" << "ST(0)" << endl;
            STREAM<< "\t\t" << "jb" << "\t" << "L" << label << endl;
            // False branch
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            STREAM<< "\t\t" << "L" << label << ":" << endl;
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            STREAM<< "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_ilt: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            STREAM<< "\t\t" << "cmp" << "\t" << "rax, rcx" << endl;
            STREAM<< "\t\t" << "jl" << "\t" << "L" << label << endl;
            // False branch
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            STREAM<< "\t\t" << "L" << label << ":" << endl;
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            STREAM<< "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_rgt: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            // We need to push in reverse order for this to work
            fetch_float(q->sym2);
            fetch_float(q->sym1);
            STREAM<< "\t\t" << "fcomip" << "\t" << "ST(0), ST(1)" << endl;
            // Clear the stack
            STREAM<< "\t\t" << "fstp" << "\t" << "ST(0)" << endl;
            STREAM<< "\t\t" << "ja" << "\t" << "L" << label << endl;
            // False branch
            STREAM<< "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            STREAM<< "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            STREAM<< "\t\t" << "L" << label << ":" << endl;
            STREAM << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            STREAM << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_igt: {
            int label = sym_tab->get_next_label();
            int label2 = sym_tab->get_next_label();

            fetch(q->sym1, RAX);
            fetch(q->sym2, RCX);
            STREAM << "\t\t" << "cmp" << "\t" << "rax, rcx" << endl;
            STREAM << "\t\t" << "jg" << "\t" << "L" << label << endl;
            // False branch
            STREAM << "\t\t" << "mov" << "\t" << "rax, 0" << endl;
            STREAM << "\t\t" << "jmp" << "\t" << "L" << label2 << endl;
            // True branch
            STREAM << "\t\t" << "L" << label << ":" << endl;
            STREAM << "\t\t" << "mov" << "\t" << "rax, 1" << endl;

            STREAM << "\t\t" << "L" << label2 << ":" << endl;
            store(RAX, q->sym3);
            break;
        }
        case q_rstore:
        case q_istore:
            fetch(q->sym1, RAX);
            fetch(q->sym3, RCX);
            STREAM << "\t\t" << "mov" << "\t" << "[rcx], rax" << endl;
            break;

        case q_rassign:
        case q_iassign:
            fetch(q->sym1, RAX);
            store(RAX, q->sym3);
            break;

        case q_param: {
            /* Your code here */
            sym_type tag = sym_tab->get_symbol_tag(q->sym1);
            if (tag == SYM_VAR)
            {
                int level, offset;
                find(q->sym1, &level, &offset);
                frame_address(level, RCX);
                STREAM << "\t\t" << "mov" << "\t" << "rax, [rcx";
                if (offset > 0)
                {
                    STREAM << "+" << offset;
                }
                else if (offset < 0)
                {
                    STREAM << offset;
                }
                STREAM << "]" << endl;
                STREAM << "\t\t" << "push" << "\t" << "rax" << endl;
            }
            else if (tag == SYM_CONST)
            {
                constant_symbol *con_s = sym_tab->get_symbol(q->sym1)->get_constant_symbol();
                sym_index type = con_s->type;
                STREAM << "\t\t" << "mov" << "\t" << "rax, ";
                STREAM << (type == integer_type ? con_s->const_value.ival : con_s->const_value.rval) << endl;
                STREAM << "\t\t" << "push" << "\t" << "rax" << endl;
            }
            break;
        }

        case q_call: {
            /* Your code here */
            sym_type tag = sym_tab->get_symbol_tag(q->sym1);
            if (tag == SYM_FUNC)
            {
                function_symbol *fun_s = sym_tab->get_symbol(q->sym1)->get_function_symbol();
                STREAM << "\t\t" << "call" << "\t" << "L" << fun_s->label_nr << endl;
            }
            else if (tag == SYM_PROC)
            {
                procedure_symbol *para_s = sym_tab->get_symbol(q->sym1)->get_procedure_symbol();
                STREAM << "\t\t" << "call" << "\t" << "L" << para_s->label_nr << endl;
            }
            if (q->int2 > 0)
            {
                STREAM << "\t\t" << "add" << "\t" << "rsp, " << q->int2*STACK_WIDTH << endl; //TODO: If parameter bigger than 8?
            }
            break;
        }
        case q_rreturn:
        case q_ireturn:
            fetch(q->sym2, RAX);
            STREAM << "\t\t" << "jmp" << "\t" << "L" << q->int1 << endl;
            break;

        case q_lindex:
            array_address(q->sym1, RAX);
            fetch(q->sym2, RCX);
            STREAM << "\t\t" << "imul" << "\t" << "rcx, " << STACK_WIDTH << endl;
            STREAM << "\t\t" << "sub" << "\t" << "rax, rcx" << endl;
            store(RAX, q->sym3);
            break;

        case q_rrindex:
        case q_irindex:
            array_address(q->sym1, RAX);
            fetch(q->sym2, RCX);
            STREAM << "\t\t" << "imul" << "\t" << "rcx, " << STACK_WIDTH << endl;
            STREAM << "\t\t" << "sub" << "\t" << "rax, rcx" << endl;
            STREAM << "\t\t" << "mov" << "\t" << "rax, [rax]" << endl;
            store(RAX, q->sym3);
            break;

        case q_itor: {
            block_level level;      // Current scope level.
            int offset;             // Offset within current activation record.

            find(q->sym1, &level, &offset);
            frame_address(level, RCX);
            STREAM << "\t\t" << "fild" << "\t" << "qword ptr [rcx";
            if (offset >= 0) {
                STREAM << "+" << offset;
            } else {
                STREAM << offset; // Implicit "-"
            }
            STREAM << "]" << endl;
            store_float(q->sym3);
        }
        break;

        case q_jmp:
            STREAM << "\t\t" << "jmp" << "\t" << "L" << q->int1 << endl;
            break;

        case q_jmpf:
            fetch(q->sym2, RAX);
            STREAM << "\t\t" << "cmp" << "\t" << "rax, 0" << endl;
            STREAM << "\t\t" << "je" << "\t" << "L" << q->int1 << endl;
            break;

        case q_labl:
            // We handled this one above already.
            break;

        case q_nop:
            // q_nop quads should never be generated.
            fatal("code_generator::expand(): q_nop quadruple produced.");
            return;
        }

        // Get the next quad from the list.
        q = ql_iterator->get_next();
    }

    // Flush the generated code to file.
    STREAM << flush;
}

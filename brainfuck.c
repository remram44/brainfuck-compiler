/*            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                    Version 2, December 2004
 *
 * Copyright (C) 2004 Sam Hocevar
 *  14 rue de Plaisance, 75014 Paris, France
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FUCK YOU WANT TO.
 */

#include <stdio.h>
#include <string.h>

#define NAME "bfc"
#define VERSION "0.1"
#define AUTHOR "Remram <remirampin@gmail.com>"
#define COPYRIGHT "Released under the terms of the WTFPL license."

int compile(FILE *in, const char *outfile);

void help(FILE *out)
{
    fprintf(out, "HELP\n");
}

int strict = 0;

int main(int argc, char **argv)
{
    const char *output = NULL;
    const char *input = NULL;
    const char std[1];
    for(argv++; *argv != NULL; argv++)
    {
        if( (strcmp(*argv, "-o") == 0) || (strcmp(*argv, "--output") == 0) )
        {
            argv++;
            if(*argv && !output)
                output = *argv;
            else if(*argv)
            {
                fprintf(stderr, "Multiple outputs specified\n");
                return 1;
            }
            else
            {
                help(stderr);
                return 1;
            }
        }
        else if(strcmp(*argv, "--stdout") == 0)
        {
            if(output)
            {
                fprintf(stderr, "Multiple outputs specified\n");
                return 1;
            }
            output = std;
        }
        else if( (strcmp(*argv, "-h") == 0) || (strcmp(*argv, "--help") == 0) )
        {
            help(stdout);
            return 0;
        }
        else if((strcmp(*argv, "-v") == 0) || (strcmp(*argv, "--version") == 0))
        {
            fprintf(stdout, NAME ", " VERSION ", by " AUTHOR ".\n"
                COPYRIGHT "\n");
            return 0;
        }
        else if(strcmp(*argv, "--strict") == 0)
            strict = 1;
        else
        {
            if(*argv)
            {
                if(input)
                {
                    fprintf(stderr, "Multiple inputs specified\n");
                    return 1;
                }
                else
                    input = *argv;
            }
            else
            {
                help(stderr);
                return 1;
            }
        }
    }

    if(!output)
        output = std;
    if(!input)
        input = std;

    {
        FILE *in;
        if(input)
            in = fopen(input, "r");
        else
            in = stdin;
        if(!in)
        {
            fprintf(stderr, "Couldn't open %s for reading\n", input);
            return 2;
        }

        return compile(in, output);
    }
}

enum Mode {
    UNKNOWN,
    DATA,
    MOVE,
    LEFT_LOOP,
    RIGHT_LOOP,
    OUTPUT,
    INPUT
};

int write_op(FILE *out, enum Mode mode, int data);
void begin_code(FILE *out);
int end_code(FILE *out);

int compile(FILE *in, const char *outfile)
{
    int c;
    enum Mode mode = UNKNOWN;
    int data = 0;
    unsigned int line = 1;

    /* TODO : temporary file */
    FILE *tmp = fopen("tmp.s", "w");
    begin_code(tmp);

    while((c = fgetc(in)) != EOF)
    {
#ifdef _DEBUG
        fprintf(stderr, "%c ", c);
#endif
        switch(c)
        {
        case '+':
        case '-':
            if(mode == DATA)
                data += (c == '+')?1:-1;
            else
            {
                write_op(tmp, mode, data);
                mode = DATA;
                data = (c == '+')?1:-1;
            }
            break;
        case '>':
        case '<':
            if(mode == MOVE)
                data += (c == '>')?1:-1;
            else
            {
                write_op(tmp, mode, data);
                mode = MOVE;
                data = (c == '>')?1:-1;
            }
            break;
        case '[':
        case ']':
            write_op(tmp, mode, data);
            mode = UNKNOWN;
            if(write_op(tmp, (c == '[')?LEFT_LOOP:RIGHT_LOOP, 0) != 0)
            {
                fprintf(stderr, "line %u: unbalanced ]\n", line);
                return 3;
            }
            break;
        case '.':
        case ',':
            write_op(tmp, mode, data);
            mode = UNKNOWN;
            write_op(tmp, (c == '.')?OUTPUT:INPUT, 0);
            break;
        case '\r':
            break;
        case '\n':
            line++;
            break;
        default:
            if(strict)
            {
                fprintf(stderr, "line %u: unknown command '%c'\n", line, c);
                return 3;
            }
            break;
        }
    }

    if(end_code(tmp) != 0)
    {
        fprintf(stderr, "line %u: at end of file: unbalanced [ ]\n", line);
        return 4;
    }
    fclose(tmp);

    /* TODO : assemble */

    return 0;
}

void begin_code(FILE *out)
{
    fprintf(out,
        "    .section .rdata,\"dr\"\n"
        "FORMATC:\n"
        "    .ascii \"%%c\\0\"\n"
        "    .text\n"
        ".globl _main\n"
        "    .def _main; .scl 2; .type 32; .endef\n"
        "_main:\n"
        "    pushl %%ebp\n"
        "    movl %%esp, %%ebp\n"
        "    andl $-16, %%esp\n"
        "    subl $16, %%esp\n"
        "    movl $_tab+131068, 12(%%esp)\n\n");
}

static size_t idx = 0;

int end_code(FILE *out)
{
    if(idx != 0)
        return 1;
    fprintf(out, "\n"
        "    movl $0, %%eax\n"
        "    leave\n"
        "    ret\n\n"
        ".lcomm _tab,262144\n");
    return 0;
}

int write_op(FILE *out, enum Mode mode, int data)
{
    static unsigned int labels[2048];
    static unsigned int gen = 0;
#ifdef _DEBUG
#define DEBUG_OUT(s) fprintf(stderr, s)
#else
#define DEBUG_OUT(s)
#endif
    switch(mode)
    {
    case UNKNOWN:
        break;
    case DATA: DEBUG_OUT("DATA\n");
        fprintf(out,
            "    movl 12(%%esp), %%eax\n"
            "    movl (%%eax), %%eax\n"
            "    leal %d(%%eax), %%edx\n"
            "    movl 12(%%esp), %%eax\n"
            "    movl %%edx, (%%eax)\n",
            data);
        break;
    case MOVE: DEBUG_OUT("MOVE\n");
        fprintf(out,
            "    addl $%d, 12(%%esp)\n",
            4*data);
        break;
    case LEFT_LOOP: DEBUG_OUT("LEFT_LOOP\n");
        labels[idx] = gen;
        gen++;
        idx++;
        fprintf(out,
            "LOOP%u:\n",
            labels[idx-1]);
        break;
    case RIGHT_LOOP: DEBUG_OUT("RIGHT_LOOP\n");
        if(idx == 0)
            return 1;
        idx--;
        fprintf(out,
            "    movl 12(%%esp), %%eax\n"
            "    movl (%%eax), %%eax\n"
            "    testl %%eax, %%eax\n"
            "    jne LOOP%u\n",
            labels[idx]);
        break;
    case OUTPUT: DEBUG_OUT("OUTPUT\n");
        fprintf(out,
            "    movl 12(%%esp), %%eax\n"
            "    movl (%%eax), %%eax\n"
            "    movl %%eax, 4(%%esp)\n"
            "    movl $FORMATC, %%eax\n"
            "    movl %%eax, (%%esp)\n"
            "    call _printf\n");
        break;
    case INPUT: DEBUG_OUT("INPUT\n");
        fprintf(out,
            "    movl 12(%%esp), %%eax\n"
            "    movl %%eax, 4(%%esp)\n"
            "    movl $FORMATC, %%eax\n"
            "    movl %%eax, (%%esp)\n"
            "    call _scanf\n");
        break;
    }
#undef DEBUG_OUT

    return 0;
}

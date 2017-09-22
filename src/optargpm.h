/***********************************************************************

          OPTARGPM - a header-only library for decoding 
                PROJ.4 style command line options

                   Thomas Knudsen, 2017-09-10

************************************************************************

For PROJ.4 command line programs, we have a somewhat complex option
decoding situation, since we have to navigate in a cocktail of classic
single letter style options, prefixed by "-", GNU style long options
prefixwd by "--", transformation specification elements prefixed by "+",
and input file names prefixed by "" nothing.

Hence, classic getopt.h style decoding does not cut the mustard, so
this is an attempt to catch up and chop the ketchup.

Since optargpm (for "optarg plus minus") does not belong, in any
obvious way, in any systems development library, it is provided as
a "header only" library. 

While this is conventional in C++, it is frowned at in plain C.
But frown away - "header only" has its places, and this is one of
them.

By convention, we expect a command line to consist of the following
elements:

        <operator/program name>
        [short ("-")/long ("--") options}
        [operator ("+") specs]
        [operands/input files]

or less verbose:

        <operator>   [options]   [operator specs]   [operands]

or less abstract:

   proj  -I --output=foo  +proj=utm +zone=32 +ellps=GRS80   bar baz...

Where

Operator is              proj
Options are             -I --output=foo
Operator specs are      +proj=utm +zone=32 +ellps=GRS80
Operands are             bar baz


While claiming neither to save the world, nor to hint at the "shape of
jazz to come",  at least optargpm has shown useful in constructing cs2cs
style transformation filters.

Supporting a wide range of option syntax, the getoptpm API is somewhat
quirky, but also compact, consisting of one data type, 3(+2) functions,
and one enumeration:

OPTARGS 
        Housekeeping data type. An instance of OPTARGS is conventionally
        called o or opt
opt_parse (opt, argc, argv ...):
        The work horse: Define supported options; Split (argc, argv)
        into groups (options, op specs, operands); Parse option
        arguments.
opt_given (o, option):
        The number of times <option> was given on the command line.
        (i.e. 0 if not given or option unsupported)
opt_arg (o, option):
        A char pointer to the argument for <option>

The 2 additional functions (of which, one is really a macro) implements
a "read all operands sequentially" functionality, eliminating the need to
handle open/close of a sequence of input files:

enum OPTARGS_FILE_MODE:
        indicates whether to read operands in text (0) or binary (1) mode
opt_input_loop (o, mode):
        When used as condition in a while loop, traverses all operands,
        giving the impression of reading just a single input file.
opt_eof_handler (o):
        Auxiliary macro, to be called inside the input loop after each
        read operation

Usage is probably easiest understood by a brief textbook style example:

Consider a simple program taking the conventinal "-v, -h, -o" options
indicating "verbose output", "help please", and "output file specification",
respectively.

The "-v" and "-h" options are *flags*, taking no arguments, while the
"-o" option is a *key*, taking a *value* argument, representing the
output file name.

The short options have long aliases: "--verbose", "--help" and "--output".
Additionally, the long key "--hello", without any short counterpart, is
supported.

-------------------------------------------------------------------------------


int main(int argc, char **argv) {
    PJ *P;
    OPTARGS *o;
    FILE *out = stdout;
    char *longflags[]  = {"v=verbose", "h=help", 0};
    char *longkeys[]   = {"o=output", "hello", 0};
    
    o = opt_parse (argc, argv, "hv", "o", longflags, longkeys);
    if (0==o)
        return 0;


    if (opt_given (o, "h")) {
        printf ("Usage: %s [-v|--verbose] [-h|--help] [-o|--output <filename>] [--hello=<name>] infile...", o->progname);
        exit (0);
    }

    if (opt_given (o, "v")) 
        puts ("Feeling chatty today?");

    if (opt_given (o, "hello")) {
        printf ("Hello, %s!\n", opt_arg(o, "hello"));
        exit (0);
    }

    if (opt_given (o, "o"))
        out = fopen (opt_arg (o, "output"), "rt"); // Note: "output" translates to "o" internally
    
    // Setup transformation 
    P = proj_create_argv (0, o->pargc, o->pargv);

    // Loop over all lines of all input files
    while (opt_input_loop (o, optargs_file_format_text)) {
        char buf[1000];
        int ret = fgets (buf, 1000, o->input);
        opt_eof_handler (o);
        if (0==ret) {
            fprintf (stderr, "Read error in record %d\n", (int) o->record_index);
            continue;
        }
        do_what_needs_to_be_done (buf);
    }

    return 0;
}


-------------------------------------------------------------------------------

Note how short aliases for longflags and longkeys are defined by prefixing
an "o=", "h=" or "v=", respectively. This also means that it is possible to
have more than one alias for each short option, e.g.

               longkeys = {"o=output", "o=banana", 0}

would define both "--output" and "--banana" to be aliases for "-o".

************************************************************************

Thomas Knudsen, thokn@sdfe.dk, 2016-05-25/2017-09-10

************************************************************************

* Copyright (c) 2016, 2017 Thomas Knudsen
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.

***********************************************************************/

#define PJ_LIB__
#include <proj.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>


typedef struct {
    int    argc,   margc,   pargc,   fargc;
    char **argv, **margv, **pargv, **fargv;
    FILE *input;
    int   input_index;
    int   record_index;
    char  *progname;         /* argv[0], stripped from /path/to, if present */
    char   flaglevel[21];    /* if flag -f is specified n times, its optarg pointer is set to flaglevel + n */
    char  *optarg[256];      /* optarg[(int) 'f'] holds a pointer to the argument of option "-f" */
    char  *flags;            /* a list of flag style options supported, e.g. "hv" (help and verbose) */
    char  *keys;             /* a list of key/value style options supported, e.g. "o" (output) */
    const char **longflags;  /* long flags, {"help", "verbose"}, or {"h=help", "v=verbose"}, to indicate homologous short options */ 
    const char **longkeys;   /* e.g. {"output"} or {o=output"} to support --output=/path/to/output-file. In the latter case, */
                             /* all operations on "--output" gets redirected to "-o", so user code need handle arguments to "-o" only */
} OPTARGS;

enum OPTARGS_FILE_FORMAT {optargs_file_format_text = 0, optargs_file_format_binary = 1};


#define opt_eof_handler(opt) if (opt_eof (opt)) {continue;} else {;}




/* name of file currently read from */
char *opt_filename (OPTARGS *opt) {
    if (0==opt)
        return 0;
    if (0==opt->fargc)
        return opt->flaglevel;
    return opt->fargv[opt->input_index];
}

static int opt_eof (OPTARGS *opt) {
    if (0==opt)
        return 1;
    return feof (opt->input);
}

/* record number of most recently read record */
int opt_record (OPTARGS *opt) {
    if (0==opt)
        return 0;
    return opt->record_index + 1;
}


/* handle closing/opening of a "stream-of-streams" */
int opt_input_loop (OPTARGS *opt, int binary) {
    if (0==opt)
        return 0;

    /* most common case: increment record index and read on */
    if ( (opt->input!=0) && !feof (opt->input) ) {
        opt->record_index++;
        return 1;
    }

    opt->record_index = 0;
        
    /* no input files specified - read from stdin */
    if ((0==opt->fargc) && (0==opt->input)) {
        opt->input = stdin;
        return 1;
    }

    /* if we're here, we have either reached eof on current input file.   */
    /*  or not yet opened a file. If eof on stdin, we're done             */
    if (opt->input==stdin)
        return 0;

    /* end if no more input */
    if (0!=opt->input)
        fclose (opt->input);
    if (opt->input_index >= opt->fargc)
        return 0;

    /* otherwise, open next input file */
    opt->input = fopen (opt->fargv[opt->input_index++], binary? "rb": "rt");

    /* ignore non-existing files - go on! */
    if (0==opt->input)
        return opt_input_loop (opt, binary);
    return 0;
}


/* return true if option with given ordinal is a flag, false if undefined or key=value */
static int opt_is_flag (OPTARGS *opt, int ordinal) {
    if (opt->optarg[ordinal] < opt->flaglevel)
        return 0;
    if (opt->optarg[ordinal] > opt->flaglevel + 20)
        return 0;
    return 1;
}

static int opt_raise_flag (OPTARGS *opt, int ordinal) {
    if (opt->optarg[ordinal] < opt->flaglevel)
        return 1;
    if (opt->optarg[ordinal] > opt->flaglevel + 20)
        return 1;

    /* Max out at 20 */
    if (opt->optarg[ordinal]==opt->flaglevel + 20)
        return 0;
    opt->optarg[ordinal]++;
    return 0;
}

/* Find the ordinal value of any (short or long) option */
static int opt_ordinal (OPTARGS *opt, char *option) {
    int i;
    if (0==opt)
        return 0;
    if (0==option)
        return 0;
    if (0==option[0])
        return 0;
    /* An ordinary -o style short option */
    if (strlen (option)==1) {
        /* Undefined option? */
        if (0==opt->optarg[(int) option[0]])
            return 0;
        return (int) option[0];
    }

    /* --longname style long options are slightly harder */
    for (i = 0; i < 64; i++) {
        const char **f = opt->longflags;
        if (0==f)
            break;
        if (0==f[i])
            break;
        if (0==strcmp(f[i], "END"))
            break;
        if (0==strcmp(f[i], option))
            return 128 + i;

        /* long alias? - return ordinal for corresponding short */
        if ((strlen(f[i]) > 2) && (f[i][1]=='=') && (0==strcmp(f[i]+2, option))) {
                /* Undefined option? */
                if (0==opt->optarg[(int) f[i][0]])
                    return 0;
                return (int) f[i][0];
        }
    }

    for (i = 0; i < 64; i++) {
        const char **v = opt->longkeys;
        if (0==v)
            return 0;
        if (0==v[i])
            return 0;
        if (0==strcmp (v[i], "END"))
            return 0;
        if (0==strcmp(v[i], option))
            return 192 + i;

        /* long alias? - return ordinal for corresponding short */
        if ((strlen(v[i]) > 2) && (v[i][1]=='=') && (0==strcmp(v[i]+2, option))) {
            /* Undefined option? */
            if (0==opt->optarg[(int) v[i][0]])
                return 0;
            return (int) v[i][0];
        }

    }

    return 0;
}


/* Returns 0 if option was not given on command line, non-0 otherwise */
int opt_given (OPTARGS *opt, char *option) {
    int ordinal = opt_ordinal (opt, option);
    if (0==ordinal)
        return 0;
    /* For flags we return the number of times the flag was specified (mostly for repeated -v(erbose) flags) */
    if (opt_is_flag (opt, ordinal))
        return opt->optarg[ordinal] - opt->flaglevel;
    return opt->argv[0] != opt->optarg[ordinal];
}


/* Returns the argument to a given option */
char *opt_arg (OPTARGS *opt, char *option) {
    int ordinal = opt_ordinal (opt, option);
    if (0==ordinal)
        return 0;
    return opt->optarg[ordinal];
}


/* split command line options into options/flags ("-" style), projdefs ("+" style) and input file args */
OPTARGS *opt_parse (int argc, char **argv, const char *flags, const char *keys, const char **longflags, const char **longkeys) {
    int i, j;
    OPTARGS *o;
    char *c;
    
    o = (OPTARGS *) calloc (1, sizeof(OPTARGS));
    if (0==o)
        return 0;

    o->argc = argc;
    o->argv = argv;
    o->progname = argv[0];
    c = strrchr (argv[0], '\\');
    if (c > o->progname)
        o->progname = c;
    c = strrchr (argv[0], '/');
    if (c > o->progname)
        o->progname = c;
        

    /* Reset all flags */
    for (i = 0;  i < (int) strlen (flags);  i++)
        o->optarg[(int) flags[i]] = o->flaglevel;

     /* Flag args for all argument taking options as "unset" */
    for (i = 0; i < (int) strlen (keys); i++)
        o->optarg[(int) keys[i]] = argv[0];
    
    /* Hence, undefined/illegal options have an argument of 0 */

    /* long opts are handled similarly, but are mapped to the high bit character range (above 128) */
    o->longflags  =  longflags;
    o->longkeys   =  longkeys;


    /* check aliases, An end user should never experience this, but the developer should make sure that aliases are valid */
    for (i = 0;  longflags && longflags[i]; i++) {
        /* Go on if it does not look like an alias */
        if (strlen (longflags[i]) < 3)
            continue;
        if ('='!=longflags[i][1])
            continue;
        if (0==strchr (flags, longflags[i][0])) {
            fprintf (stderr, "%s: Invalid alias - '%s'. Valid short flags are '%s'\n", o->progname, longflags[i], flags);
            free (o);
            return 0;            
        }
    }
    for (i = 0;  longkeys && longkeys[i]; i++) {
        /* Go on if it does not look like an alias */
        if (strlen (longkeys[i]) < 3)
            continue;
        if ('='!=longkeys[i][1])
            continue;
        if (0==strchr (keys, longkeys[i][0])) {
            fprintf (stderr, "%s: Invalid alias - '%s'. Valid short flags are '%s'\n", o->progname, longkeys[i], keys);
            free (o);
            return 0;            
        }
    }

    /* aside from counting the number of times a flag has been specified, we also abuse the */
    /* flaglevel array to provide a pseudo-filename for the case of reading from stdin      */
    strcpy (o->flaglevel, "<stdin>");

    for (i = 128; (longflags != 0) && (longflags[i - 128] != 0); i++) {
        if (i==192) {
            free (o);
            fprintf (stderr, "Too many flag style long options\n");
            return 0;
        }
        o->optarg[i] = o->flaglevel;
    }

    for (i = 192;  (longkeys != 0) && (longkeys[i - 192] != 0);  i++) {
        if (i==256) {
            free (o);
            fprintf (stderr, "Too many value style long options\n");
            return 0;
        }
        o->optarg[i] = argv[0];
    }

    /* Now, set up the agrc/argv pairs, and interpret args */
    o->argc = argc;
    o->argv = argv;

    /* Process all '-' and '--'-style options */
    for (i = 1;  i < argc;  i++) {
        int arg_group_size = strlen (argv[i]);

        if ('-' != argv[i][0])
            break;
        if (0==o->margv)
            o->margv = argv + i;
        o->margc++;

        for (j = 1;  j < arg_group_size;  j++) {
            int c =  argv[i][j];
            char cstring[2], *crepr = cstring;
            cstring[0] = (char) c;
            cstring[1] = 0;
            

            /* Long style flags and options (--long_opt_name, --long_opt_namr arg, --long_opt_name=arg) */
            if (c== (int)'-') {
                char *equals;
                crepr = argv[i] + 2;

                /* need to maniplulate a bit to support gnu style --pap=pop syntax */
                equals = strchr (crepr, '=');
                if (equals)
                    *equals = 0;
                c = opt_ordinal (o, crepr);
                if (0==c)
                    return fprintf (stderr, "Invalid option \"%s\"\n", crepr), (OPTARGS *) 0;

                /* inline (gnu) --foo=bar style arg */
                if (equals) {
                    *equals = '=';
                    if (opt_is_flag (o, c))
                        return fprintf (stderr, "Option \"%s\" takes no arguments\n", crepr), (OPTARGS *) 0;
                    o->optarg[c] = equals + 1;
                    break;
                }
                
                /* "outline" --foo bar style arg */
                if (!opt_is_flag (o, c)) {
                    if ((argc==i + 1) || ('+'==argv[i+1][0]) || ('-'==argv[i+1][0]))
                        return fprintf (stderr, "Missing argument for option \"%s\"\n", crepr), (OPTARGS *) 0;
                    o->optarg[c] = argv[i + 1];
                    i++; /* eat the arg */
                    break;
                }

                if (!opt_is_flag (o, c))
                    return fprintf (stderr, "Expected flag style long option here, but got \"%s\"\n", crepr), (OPTARGS *) 0;

                /* Flag style option, i.e. taking no arguments */
                opt_raise_flag (o, c);
                break;
            }

            /* classic short options */
            if (0==o->optarg[c])
                return fprintf (stderr, "Invalid option \"%s\"\n", crepr), (OPTARGS *) 0;

            /* Flag style option, i.e. taking no arguments */
            if (opt_is_flag (o, c)) {
                opt_raise_flag (o, c);
                continue;
            }

            /* options taking argumants */

            /* argument separate (i.e. "-i 10") */
            if (j + 1==arg_group_size) {
                if ((argc==i + 1) || ('+'==argv[i+1][0]) || ('-'==argv[i+1][0]))
                    return fprintf (stderr, "Bad or missing arg for option \"%s\"\n", crepr), (OPTARGS *) 0;
                o->optarg[(int) c] = argv[i + 1];
                i++;
                break;
            }

            /* Option arg inline (i.e. "-i10") */
            o->optarg[c] = argv[i] + j + 1;
            break;
        }
    }

    /* Process all '+'-style options, starting from where '-'-style processing ended */
    o->pargv = argv + i;
    for (/* empty */; i < argc; i++) {
        if ('-' == argv[i][0]) {
            free (o);
            fprintf (stderr, "+ and - style options must not be mixed\n");
            return 0;
        }

        if ('+' != argv[i][0])
            break;
        o->pargc++;
    }

    /* Handle input file names */
    o->fargc = argc - i;
    if (0!=o->fargc)
        o->fargv = argv + i;

    return o;

}

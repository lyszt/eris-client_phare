#include "alias_add.h"
#include "commands/alias/alias_ops.h"
#include "term/term.h"
#include "utils/eris_template_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "utils.h"

/* Macros live in the binary file .eris/.eris.macros, the same file init creates. */

#define MAX_CMD_LINE 4096

/* Appends one command line carrying the operator that precedes it.
 * Receives the destination array with its count and returns nothing. */
static void add_command_line(char **lines, int *nlines, int max_lines, const char *op, const char *cmd) {
    if (!cmd[0] || *nlines >= max_lines)
        return;
    char line[MAX_CMD_LINE + 8];
    if (snprintf(line, sizeof(line), "%s %s", op, cmd) < 0)
        return;
    lines[*nlines] = strdup(line);
    if (lines[*nlines])
        (*nlines)++;
}

void alias_add(int argc, char **argv) {
    char eris_location[PATH_MAX];

    if (!find_eris_root(eris_location, PATH_MAX)) {
        eris_printf(ERIS_LOG_ERROR, "Fatal: Not in an Eris project (or any of the parent directories).\n");
        return;
    }
    int is_global = 0;
    if (argc >= 1 && argv[0] && strcmp(argv[0], "--global") == 0) {
        is_global = 1;
        argv++; argc--;
    }

    if (argc < 1) {
        eris_printf(ERIS_LOG_ERROR, "Usage: eris alias add [--global] <name> <cmd1> and <cmd2> ...\n");
        eris_printf(ERIS_LOG_ERROR, "Separators: do runs always, and runs on success, or runs on failure.\n");
        return;
    }
    const char *name = argv[0];
    if (!name || !name[0]) {
        eris_printf(ERIS_LOG_ERROR, "Macro name cannot be empty.\n");
        return;
    }

    /* Each separator ends a command line, tokens in between are joined with spaces. */
    char *lines[256];
    const int max_lines = (int)(sizeof(lines) / sizeof(lines[0]));
    int nlines = 0;
    char seg[MAX_CMD_LINE];
    size_t seglen = 0;
    const char *op = "do";
    for (int i = 1; i < argc; i++) {
        if (!argv[i] || !argv[i][0])
            continue;
        const char *sep = find_alias_op(argv[i]);
        if (sep) {
            seg[seglen] = '\0';
            add_command_line(lines, &nlines, max_lines, op, seg);
            seglen = 0;
            op = sep;
            continue;
        }
        size_t len = strlen(argv[i]);
        if (seglen + (seglen ? 1 : 0) + len >= sizeof(seg))
            continue;
        if (seglen > 0)
            seg[seglen++] = ' ';
        memcpy(seg + seglen, argv[i], len + 1);
        seglen += len;
    }
    seg[seglen] = '\0';
    add_command_line(lines, &nlines, max_lines, op, seg);

    char macro_path[PATH_MAX];
    if (is_global)
        snprintf(macro_path, sizeof(macro_path), "%s/.eris/.eris.macros", eris_location);
    else if (!eris_macros_path(eris_location, macro_path, sizeof(macro_path))) {
        for (int i = 0; i < nlines; i++) free(lines[i]);
        eris_printf(ERIS_LOG_ERROR, "Fatal: path too long.\n");
        return;
    }

    if (append_macro(macro_path, name, lines, (size_t)nlines))
        eris_printf(ERIS_LOG_INFO, "Macro '%s' added (%d command(s)).\n", name, nlines);
    else
        eris_printf(ERIS_LOG_ERROR, "Cannot write to %s\n", macro_path);
    for (int i = 0; i < nlines; i++)
        free(lines[i]);
}
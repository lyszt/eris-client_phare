#include "alias_run.h"
#include "commands/alias/alias_ops.h"
#include "term/term.h"
#include "utils.h"
#include "utils/eris_template_utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/wait.h>
#include <unistd.h>

/* Macros are in the binary file .eris/.eris.macros (same file init creates). */

#define MAX_CMD_LINE 4096

static void err(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
}

void alias_run(int argc, char **argv) {
    char root_path[PATH_MAX];
    if (!find_eris_root(root_path, sizeof(root_path))) {
        err("eris: Not in an Eris project (run 'eris init' first).\n");
        return;
    }
    if (argc < 1 || !argv[0] || !argv[0][0]) {
        err("eris: Usage: eris alias run <name>\n");
        return;
    }
    const char *name = argv[0];

    char **lines = NULL;
    size_t n = 0;

    /* try branch-scoped macros first, then fall back to global */
    char macro_path[PATH_MAX];
    if (eris_macros_path(root_path, macro_path, sizeof(macro_path)))
        get_macro_commands(macro_path, name, &lines, &n);

    if (!lines) {
        char global_path[PATH_MAX];
        snprintf(global_path, sizeof(global_path), "%s/.eris/.eris.macros", root_path);
        get_macro_commands(global_path, name, &lines, &n);
    }

    if (!lines) {
        err("eris: Macro '%s' not found (add with 'eris alias add %s ...').\n", name, name);
        return;
    }

    char exec_path[PATH_MAX];
    exec_path[0] = '\0';
#if defined(__linux__)
    {
        ssize_t len = readlink("/proc/self/exe", exec_path, sizeof(exec_path) - 1);
        if (len > 0) {
            exec_path[len] = '\0';
        }
    }
#endif

    int last_status = 0;
    for (size_t i = 0; i < n; i++) {
        if (!lines[i] || !lines[i][0]) continue;
        const char *cmd = NULL;
        const char *op = split_alias_op(lines[i], &cmd);
        if (!cmd[0]) continue;
        if (strcmp(op, "and") == 0 && last_status != 0) continue;
        if (strcmp(op, "or") == 0 && last_status == 0) continue;

        const char *run = cmd;
        char buf[MAX_CMD_LINE];
        if (exec_path[0]) {
            if (strcmp(cmd, "eris") == 0) {
                run = exec_path;
            } else if (strncmp(cmd, "eris ", 5) == 0) {
                int need = snprintf(buf, sizeof(buf), "%s %s", exec_path, cmd + 5);
                if (need > 0 && (size_t)need < sizeof(buf))
                    run = buf;
            }
        }
        fflush(stdout);
        int ret = system(run);
        fflush(stdout);
        last_status = WIFEXITED(ret) ? WEXITSTATUS(ret) : 1;
        if (last_status != 0)
            err("eris: command exited %d: %s\n", last_status, cmd);
    }

    for (size_t i = 0; i < n; i++) free(lines[i]);
    free(lines);
}

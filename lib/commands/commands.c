#include "commands.h"
#include "term/term.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CLR_RESET "\x1b[0m"
#define CLR_BOLD "\x1b[1m"
#define CLR_GREEN "\x1b[32m"
#define CLR_YELLOW "\x1b[33m"
#define CLR_CYAN "\x1b[36m"

void help_commands(void) {
    eris_printf(ERIS_LOG_CMD, "usage: eris <command> [arguments]\n\n");
    eris_printf(ERIS_LOG_CMD, "Commands:\n");
    eris_printf(ERIS_LOG_CMD, "  init [PROJECT_NAME]     Initialize an Eris project (default: current folder name)\n");
    eris_printf(ERIS_LOG_CMD, "  commit, copush          Stage, commit (prompt for message), push\n");
    eris_printf(ERIS_LOG_CMD, "  rebuild                 Rebuild eris binary\n");
    eris_printf(ERIS_LOG_CMD, "  alias add <name> do <cmd> do <cmd> ...  Add macro (do between commands)\n");
    eris_printf(ERIS_LOG_CMD, "  alias add <name> \"cmd1\" \"cmd2\"        Or quote each command\n");
    eris_printf(ERIS_LOG_CMD, "  alias run <name>        Run a macro (shorthand: eris run <name>)\n");
    eris_printf(ERIS_LOG_CMD, "  ignore <file> [file]    Exclude file(s) from copush staging (stored in .eris)\n");
    eris_printf(ERIS_LOG_CMD, "  branch <name>           Create a branch and push it upstream\n");
    eris_printf(ERIS_LOG_CMD, "  rebase [branch]         Rebase branch (default: current) onto updated base\n");
    eris_printf(ERIS_LOG_CMD, "  merge [branch]          Merge updated base into branch (default: current)\n\n");
    eris_printf(ERIS_LOG_CMD, "Chaining (Prolog router): and (then)  or (else)  not <cmd> (invert)\n");
    eris_printf(ERIS_LOG_CMD, "Example: eris init myapp and commit\n");
}
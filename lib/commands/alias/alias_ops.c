#include "alias_ops.h"
#include <string.h>

static const char *const ALIAS_OPS[] = { "do", "and", "or" };
#define ALIAS_OP_COUNT (sizeof(ALIAS_OPS) / sizeof(ALIAS_OPS[0]))

const char *find_alias_op(const char *token) {
    if (!token) return NULL;
    for (size_t i = 0; i < ALIAS_OP_COUNT; i++)
        if (strcmp(token, ALIAS_OPS[i]) == 0)
            return ALIAS_OPS[i];
    return NULL;
}

const char *split_alias_op(const char *line, const char **cmd_out) {
    for (size_t i = 0; i < ALIAS_OP_COUNT; i++) {
        size_t len = strlen(ALIAS_OPS[i]);
        if (strncmp(line, ALIAS_OPS[i], len) == 0 && line[len] == ' ') {
            *cmd_out = line + len + 1;
            return ALIAS_OPS[i];
        }
    }
    *cmd_out = line;
    return "do";
}

#ifndef ERIS_ALIAS_OPS_H
#define ERIS_ALIAS_OPS_H

/* Macro command lines are stored as "<op> <command>", op being do, and or or. */

/* Recognizes a macro separator token.
 * Receives a token and returns the operator word, or NULL when it is not a separator. */
const char *find_alias_op(const char *token);

/* Splits a stored command line into operator and command.
 * Receives a line and returns the operator, writing the command part into cmd_out. */
const char *split_alias_op(const char *line, const char **cmd_out);

#endif

#include "commit.h"
#include "term/term.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "git2_utils/git2_utils.h"
#include "components/select.h"
#include "utils.h"

#define MAX_MSG    4096

/* readline prompt sequences — \001/\002 hide invisible chars from width calc */
#define RL_CYAN  "\001\x1b[36m\002"
#define RL_BOLD  "\001\x1b[1m\002"
#define RL_DIM   "\001\x1b[2m\002"
#define RL_RESET "\001\x1b[0m\002"

static void save_cache(const char *eris_root, const char *branch, const char *msg) {
	char dir[PATH_MAX], path[PATH_MAX];
	if (branch[0] && eris_branch_dir(eris_root, branch, dir, sizeof(dir)))
		snprintf(path, sizeof(path), "%s/commit_cache", dir);
	else
		snprintf(path, sizeof(path), "%s/.eris/commit_cache", eris_root);
	FILE *f = fopen(path, "w");
	if (f) { fputs(msg, f); fclose(f); }
}

static int load_cache(const char *eris_root, const char *branch, char *buf, size_t sz) {
	char dir[PATH_MAX], path[PATH_MAX];
	if (branch[0] && eris_branch_dir(eris_root, branch, dir, sizeof(dir)))
		snprintf(path, sizeof(path), "%s/commit_cache", dir);
	else
		snprintf(path, sizeof(path), "%s/.eris/commit_cache", eris_root);
	FILE *f = fopen(path, "r");
	if (!f) return 0;
	size_t n = fread(buf, 1, sz - 1, f);
	buf[n] = '\0';
	fclose(f);
	return n > 0;
}


static char *prompt_required(const char *label) {
	char *s;
	while (1) {
		s = readline(label);
		if (s && s[0]) return s;
		eris_printf(ERIS_LOG_ERROR, "This field is required.\n");
		free(s);
	}
}

void commit_push(int argc, char **argv) {
	int retry = 0;
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--retry") == 0 || strcmp(argv[i], "-r") == 0)
			retry = 1;
	}

	char eris_root[PATH_MAX] = {0};
	int has_root = find_eris_root(eris_root, sizeof(eris_root));
	char branch[256] = {0};
	get_current_branch(branch, sizeof(branch));
	char final_msg[MAX_MSG] = {0};

	if (retry && has_root) {
		if (!load_cache(eris_root, branch, final_msg, sizeof(final_msg)) || !final_msg[0]) {
			eris_printf(ERIS_LOG_ERROR, "No cached commit found. Run eris commit normally first.\n");
			return;
		}
		eris_printf(ERIS_LOG_INFO, "Retrying with cached commit message.\n");
	} else {
		if (!eris_git_has_changes(".")) {
			eris_printf(ERIS_LOG_WARN, "Nothing to commit — working tree is clean.\n");
			return;
		}

		eris_printf(ERIS_LOG_WARN, "This will stage all changes, commit and push.\n\n");

		/* commit type */
		eris_printf(ERIS_LOG_INFO, "Select the type of commit:\n");
		const char *options[] = {
			"feat", "fix", "chore", "refactor", "docs", "test", "perf", "style", "build",
			"ci", "revert", "wip", "merge", "release", "security", "deps", "infra", "ux",
			"i18n", "hotfix", NULL
		};
		int choice = summon_select(options);
		const char *type = (choice >= 0 && options[choice]) ? options[choice] : "chore";

		/* optional scope */
		char *scope_raw = readline("\n" RL_CYAN "  scope  " RL_RESET RL_DIM " (optional) " RL_RESET ": ");
		char scope_buf[128] = {0};
		if (scope_raw && scope_raw[0])
			snprintf(scope_buf, sizeof(scope_buf), "(%s)", scope_raw);
		free(scope_raw);

		/* short description */
		char *subject = prompt_required(RL_CYAN "  subject " RL_RESET ": ");

		/* Build conventional commit message */
		int pos;
		if (branch[0])
			pos = snprintf(final_msg, sizeof(final_msg), "%s%s: %s %s", type, scope_buf, branch, subject);
		else
			pos = snprintf(final_msg, sizeof(final_msg), "%s%s: %s", type, scope_buf, subject);
		free(subject);
		(void)pos;
	}

	if (has_root)
		save_cache(eris_root, branch, final_msg);

	eris_printf(ERIS_LOG_DEBUG, "\n  committing: ");
	eris_printf(ERIS_LOG_CMD, "%s\n\n", final_msg);

	int ret = eris_git_commit_and_push(".", final_msg);
	if (ret != 0)
		eris_printf(ERIS_LOG_ERROR, "Git commit/push failed.\n");
	else
		eris_printf(ERIS_LOG_INFO, "  pushed.\n");
}

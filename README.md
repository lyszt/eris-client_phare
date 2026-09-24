# Eris (pronounced "Eris")

Eris is a developer CLI that wraps your git workflow with a structured commit flow, project-local macros, and a Prolog-powered command router that supports chaining and negation.

You run `eris` instead of raw git for day-to-day work.

---

## What it does

**`eris init [name]`** — sets up an `.eris/` directory in the current folder, writes the macros file, and adds `.eris/` to git's local exclude so it never shows up in your repo.

**`eris commit` / `eris copush`** — interactive commit + push in one shot. Prompts you to pick a conventional commit type (`feat`, `fix`, `refactor`, etc.) from a menu, then asks for a message, formats it as `type: message`, stages everything, commits, and pushes. Works from anywhere inside the project.

**`eris ignore <file>`** — adds a file to `.eris/.eris.ignore` so it's skipped during `copush`. Per-project, not global.

**`eris alias add <name> do <cmd> do <cmd>`** — saves a named macro (sequence of shell commands) to `.eris/.eris.macros`.

**`eris alias run <name>`** / **`eris run <name>`** — runs a saved macro.

**`eris rebuild`** — rebuilds the eris binary itself using CMake.

**`eris root`** — prints the detected eris project root.

---

## Command chaining

The command router is written in Prolog. It supports `&&`, `||`, and `not` directly in the argument list:

```bash
eris init myapp && eris commit      # commit only if init succeeded
eris init || true                   # ignore failure
eris not init                       # succeed when init fails
```

---

## Macros

Macros are stored in `.eris/.eris.macros` at the project root:

```ini
[deploy]
make build
git push origin main
ssh prod "systemctl restart app"
```

**Add with `do` separator** (no quoting needed):
```bash
eris alias add deploy do make build do git push origin main
```

**Add with quotes** (for commands that contain semicolons or shell syntax):
```bash
eris alias add test "npm run lint" "npm test"
```

**Run:**
```bash
eris run deploy
```

---

## Install

```bash
make
```

This builds the binary and symlinks it into `~/.local/bin/eris` (override with `LINK_DIR=...`), so `eris` works immediately without sudo. For a system-wide install instead:

```bash
sudo make install
```

This builds the binary, installs it (plus its Prolog router files) to `/usr/local/lib/eris/`, and symlinks `/usr/local/bin/eris` to it. No shell alias is needed as long as `/usr/local/bin` is on your `PATH`. To install without root, use a user prefix:

```bash
make install PREFIX=$HOME/.local   # binary lands in ~/.local/bin
```

Dependencies (`cmake`, `gcc`, `libcurl`, `libssl`, etc.) are checked and installed automatically during build if missing.

---

## Build from source

```bash
make        # configure + build (Debug)
make CONFIG=Release
make JOBS=8
```

Requires: `cmake`, `gcc`/`g++`, `libcurl-dev`, `libssl-dev`, `pkg-config`.  
Optional: SWI-Prolog (`libswipl-dev`) for the Prolog router — falls back to a C router if not present.

---

## Docs

- [User guide](docs/user-guide.md)
- [Architecture](docs/architecture.md)
- [Contributing](docs/contributing.md)

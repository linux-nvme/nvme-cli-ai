# C coding conventions

> File paths in this document refer to the **nvme-cli** repository at `../nvme-cli/` relative to this repo. These conventions are enforced by convention and code review (they recur constantly in maintainer review), and partly by checkpatch; follow kernel style throughout (see the top-level `CLAUDE.md` "Code style" section for the basics: tabs, 80 columns, brace placement).

## Prefer existing helpers over reinventing them

Before writing a small utility, grep for one — nvme-cli/libnvme already has most of them, and a duplicate is a review rejection.

- **`xstrdup(s)`** (`libnvme/src/nvme/private.h`) — an allocation-checking `strdup` that returns `NULL` for a `NULL` input. Use it instead of hand-rolling `strdup` + NULL-check, or `malloc` + `strcpy`.
- **`asprintf(&p, fmt, …)`** — use it to build a string whose length you'd otherwise have to compute. It replaces the `malloc(N)` + `snprintf(p, N, …)` pattern (and removes the risk of an off-by-one `N`). Remember to check its return value and `free(p)`.
- **Trimming whitespace** — a trimming helper already exists (`trim_white_space()`); reuse it rather than open-coding another `while (isspace(*p)) p++` loop.
- **Parsing `key=value;…` transport strings** — use `libnvmf_tid_parse()`. Do not add a second, parallel parser for the same syntax.

## Paths and environment variables

- **Never hard-code `/etc`.** Use the build-provided **`SYSCONFDIR`** prefix, exactly as the existing code does: `#define PATH_FOO SYSCONFDIR "/nvme/foo"` (see `fabrics.c`, `libnvme/src/nvme/fabrics.c`). This is what lets distributions relocate config.
- **Environment variables that libnvme reads use the `LIBNVME_` prefix** (e.g. an override for a config directory). Keep the namespace consistent so the variables are discoverable and don't collide.

## Control flow

- **Use the early-return / guard-clause pattern.** Handle the error or the trivial case up front and return, rather than nesting the main body inside an `if`. Prefer:
  ```c
  if (fd == -1)
          return;
  fsync(fd);
  close(fd);
  ```
  over wrapping the body in `if (fd != -1) { … }`.
- **Don't abuse the ternary `?:` operator.** It's fine for a simple value selection, but when it starts nesting or spanning the line it's harder to read than a plain `if`/`else` — write the `if`/`else`.

## Whitespace and layout

- **80 columns max**, including comments (checkpatch enforces it).
- **Add a blank line before the final `return`** of a function, and use blank lines to separate logical blocks. Dense, unbroken function bodies draw review comments.
- **Wrapped function arguments in headers get a double-tab indent**, matching the style the headers already use:
  ```c
  __libnvme_public void libnvmf_tid_set_transport(
                  struct libnvmf_tid *p, const char *val);
  ```

## Comments

- **Keep comment style consistent within a file.** Use `//` for short, single-line, and trailing comments; reserve `/* … */` for multi-line block comments. Don't mix the two arbitrarily in the same file.
- **Document *why*, not *what*.** A comment that restates the code goes stale and adds noise. In particular, do **not** enumerate a function's callers or "used by X, Y, Z" in its doc comment — that's exactly what `grep` is for, and it rots the moment a caller is added or removed. Explain non-obvious rationale (why a protocol is correct, why an ordering matters) instead.
- **Don't duplicate a comment that already lives at the definition** (e.g. re-explaining a `#define`'s meaning at each use site).

## Naming — match the existing spelling for each parameter

Match the spelling the codebase **already uses for that specific parameter** — aim for consistency *with existing usage*, not internal uniformity across parameters:

- `hostnqn`, `subsysnqn` — no underscore (that's how the code spells them).
- `host_traddr`, `host_iface` — with an underscore (that's how the code spells them).

Do not introduce a third spelling (e.g. `host_nqn`). If full uniformity is ever wanted — underscore everywhere or nowhere — that is a separate, dedicated cleanup PR done "in one big swoop", never folded into a feature change.

# checkpatch: known false positives

> File paths and commands in this document refer to the **nvme-cli** repository at `../nvme-cli/` relative to this repo.

nvme-cli enforces Linux kernel checkpatch style; CI runs it on every PR via `checkpatch.yml`, and `make checkpatch-diff` runs the same check locally from `../nvme-cli/`. Run it before every commit and fix everything it reports — **except** the categories below, which are recurring, confirmed false positives specific to this codebase. Don't spend time "fixing" these; don't restructure working code to dodge them; don't treat a checkpatch-red CI job as a blocker without first checking whether the hits are already on this list.

This list only applies to **your own** PRs, where you hold yourself to a clean `checkpatch-diff` before calling a branch ready. When reviewing someone *else's* PR, don't raise checkpatch nits at all — the maintainer has full authority to disregard them, and pointing them out reads as telling them their own job.

---

## `__cleanup_*` — "Missing a blank line after declarations"

checkpatch doesn't recognize `__cleanup_free`, `__cleanup_json`, `__cleanup_libnvme_free`, etc. as variable declarations. When two or more `__cleanup_*` declarations appear together, or one appears alongside a plain declaration, checkpatch may report a missing blank line regardless of actual spacing. False positive — ignore it.

**This one can fail CI**, not just local checks, when the `__cleanup_*` line is itself part of a freshly-added diff hunk: CI's checkpatch job runs `git format-patch --stdout origin/master..HEAD | checkpatch.pl -`, and a newly-added `__cleanup_*` declaration shows up in that stream and trips the same false positive (exit 1). Still safe to ignore — just don't assume the CI job stays green the first time this pattern is newly introduced in a PR.

## Multi-commit PRs — "Duplicate signature"

Whenever a PR has more than one commit by the same author, CI's `checkpatch.pl` sees the same `Signed-off-by:` trailer once per commit (since `format-patch` concatenates every commit's patch into one stream) and flags each repeat as a duplicate. Harmless and not fixable — every commit needs its own DCO trailer regardless.

## `for_each` in a function name — bogus brace-placement ERROR

checkpatch treats any identifier where `for_each` is followed by more characters (e.g. `libnvmf_config_for_each_conn`) as an iterator macro and demands `{` on the same line as the declaration — a false ERROR on an ordinary function definition. Names *ending* in `for_each` (e.g. `libnvmf_exclusion_entry_for_each`) parse fine. This is also the codebase's own naming convention regardless: name iterators `<object>_for_each`, never `for_each_<object>`.

## Function printing its own name — "Prefer `"%s...", __func__`"

When a function logs its own name as a literal (e.g. `printf("test_foo:\n")`), checkpatch suggests `__func__` instead. **Do not "fix" this** — switching to `__func__` trades it for a different warning (`Unnecessary ftrace-like logging - prefer using ftrace`). Confirmed twice, independently, by actually making the change both times. This is a genuine no-win in the current checkpatch version; leave the literal name as-is, matching the codebase's existing style (e.g. `libnvme/test/registry.c`).

## Code-generator annotation comments over 80 columns

Lines like `struct libnvme_global_ctx { // !generate-accessors:read=none,write=none,prefix=libnvme !generate-python:alias=GlobalCtx` must stay on one line — the generator's parser needs the whole annotation together, even though it happens to tolerate the annotation being split onto its own line below the opening brace (verified: regenerating with it split produces byte-identical output). Splitting it just to satisfy the 80-column check is not worth the divergence from the pattern every other annotated struct uses. Accept the LONG_LINE warning here.

## `volatile` on a field that's deliberately never cached

checkpatch flags every `volatile` declaration with "Use of volatile is usually wrong." In the lazy-sysfs accessor structs (`libnvme_ctrl_sysfs`, `libnvme_path_sysfs`, …), a `volatile`-qualified member is a deliberate design choice — the whole point is that it's re-read from sysfs on every call and never cached, so the C keyword documents real intent, not a mistake. Precedent for legitimate `volatile` predates this pattern too (`util/sighdl-linux.c`, `util/sighdl-win.c`, `libnvme/test/register.c`). Don't remove `volatile` or restructure the field just to silence this warning when the semantics genuinely require it.

## `sscanf` — "Prefer kstrto\<type\> to single variable sscanf"

`kstrto*()` is a Linux **kernel** function family; it does not exist in userspace/glibc, so this suggestion is never actually actionable in this codebase. nvme-cli parses sysfs attribute strings with `sscanf()` pervasively (`tree.c`, `tree-linux.c`, every lazy sysfs getter that parses a numeric attribute) — this warning fires on all of them and always will. Ignore it; there is no userspace equivalent to switch to.

## Typedef'd pointer type, wrapped multi-line signature — "need consistent spacing around '*'"

On a function signature that wraps across lines, checkpatch's pointer-spacing heuristic sometimes doesn't recognize a typedef'd type name across the wrap (seen on `sd_event_source *src`, `sd_bus_error *ret_err` in nvme-discoverd/nvme-stas-adjacent code) and flags an ERROR. Its own `--fix-inplace` suggestion for this would itself violate real kernel style (`sd_event_source * src`). Not worth fixing reactively — same tier as the other categories here.

---

## Not a false positive: checkpatch still gates your own PRs

All of the above are specific, confirmed exceptions. Everything else checkpatch reports — real style violations, genuinely missing blank lines, real line-length problems in code you wrote (not generator annotations), unrelated warnings — must be fixed before a branch is called ready. When `make checkpatch-diff` reports something not on this list, treat it as real until proven otherwise.

If you find a new recurring false positive not listed here, add it — this list is meant to grow, not to be treated as exhaustive on day one.

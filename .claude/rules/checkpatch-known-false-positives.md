# checkpatch: known false positives

> File paths and commands in this document refer to the **nvme-cli** repository at `../nvme-cli/` relative to this repo.

nvme-cli enforces Linux kernel checkpatch style; CI runs it on every PR via `checkpatch.yml`, and `make checkpatch-diff` runs the same check locally from `../nvme-cli/`. Run it before every commit and fix everything it reports — **except** the categories below, which are recurring, confirmed false positives specific to this codebase. Don't spend time "fixing" these; don't restructure working code to dodge them; don't treat a checkpatch-red CI job as a blocker without first checking whether the hits are already on this list.

This list only applies to **your own** PRs, where you hold yourself to a clean `checkpatch-diff` before calling a branch ready. When reviewing someone *else's* PR, don't raise checkpatch nits at all — the maintainer has full authority to disregard them, and pointing them out reads as telling them their own job.

---

## `__cleanup_*` — "Missing a blank line after declarations"

checkpatch doesn't recognize `__cleanup_free`, `__cleanup_json`, `__cleanup_libnvme_free`, etc. as variable declarations. When two or more `__cleanup_*` declarations appear together, or one appears alongside a plain declaration, checkpatch may report a missing blank line regardless of actual spacing. False positive — ignore it.

**This one can fail CI**, not just local checks, when the `__cleanup_*` line is itself part of a freshly-added diff hunk: CI's checkpatch job runs `checkpatch.pl --git origin/master..HEAD`, and a newly-added `__cleanup_*` declaration shows up in that commit's patch and trips the same false positive (exit 1). Still safe to ignore — just don't assume the CI job stays green the first time this pattern is newly introduced in a PR.

## Multi-commit PRs — "Duplicate signature" (no longer occurs)

This warning came from piping a whole series into checkpatch as one patch: the author's `Signed-off-by:` then appears once per commit. CI runs `checkpatch.pl --git <range>` since `aecd0dd0c` (2026-10-01), which checks each commit on its own, and `make checkpatch` does the same since the `checkpatch-per-commit` change. If you see it now, you piped `git format-patch --stdout` into checkpatch by hand. Use `--git` instead. It cannot be masked: checkpatch reports it as `BAD_SIGN_OFF`, which also covers real sign-off errors.

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

## Wrapped `OPT_*` help/description strings — "quoted string split across lines" (`SPLIT_STRING`)

checkpatch's `SPLIT_STRING` check exists so a message that appears verbatim in a kernel log stays one grep'able string instead of being silently reconstructed from concatenated literals at compile time. That rationale is about *runtime log output*, not source text in general. An `OPT_STRING`/`OPT_FLAG` help/description string (e.g. `desc_persistent` in `src/config-create.c`) is never emitted to a log for someone to grep — it only ever reaches `--help` text — so wrapping it across multiple string-literal lines for source readability carries none of the downside the rule exists to prevent. Confirmed by Martin (2026-08-10): `desc_persistent`'s wrapping is fine as-is; the general "never wrap string literals" convention (see `coding-style.md`) is about grep'able runtime messages, not argument help text.

## `OPT_*` entries in a shared args macro (e.g. `NVMF_ARGS` in `src/fabrics.h`) — line length

Every entry in this macro is already one unwrapped line, column-aligned with its siblings for readability, and already exceeds 80 columns as a block-wide, pre-existing convention — not something a single new/edited entry introduced. Confirmed by Martin (2026-08-17): wrapping just the one or two lines you happen to touch, to satisfy the 80-column check in isolation, breaks that alignment and looks worse than just accepting the warning. Leave `OPT_*` entries in this macro as single lines and accept the `LONG_LINE` warning, matching every sibling entry already there.

## Rows of a column-aligned table — line length

A static table whose rows are column-aligned (e.g. `keys[]` in `libnvme/src/nvme/config-ini.c`) is easier to read when every row is one line. Keep a new row on one line, aligned with its siblings, even if it goes over 80 columns. Do not wrap it. Confirmed by Martin (2026-09-28) for the `key-source` row. Accept the `LONG_LINE` warning.

## `NVME_ARGS_OUTPUT_FORMATS` continuation lines — line length, but NOT indentation

Editing an `OPT_*` help string in `src/args.h` puts its continuation line into the diff, and checkpatch then reports three hits against a line whose length and indentation the edit never changed. Only one of the three is a false positive.

**Accept:** `WARNING: line length of 88 exceeds 80 columns`. Every line in this macro aligns its trailing `\` at display column 88. Pulling one line's backslash left to satisfy the check in isolation misaligns it from all 25 siblings — the same case, and the same ruling, as the `NVMF_ARGS` entry above.

**Fix:** `ERROR: code indent should use tabs where possible` and `WARNING: please, no spaces at the start of a line`. The block is **mixed**, not uniformly space-indented: four continuation lines use spaces (verbose, quiet, timeout, dry-run) and six already use three tabs plus a space (no-retries, no-ioctl-probing, output-format-version, set-options ×2). Tabs are the majority and are kernel style, so convert the line you touched:

```c
		OPT_FLAG("quiet",        0, &nvme_args.quiet,                          \
			 "suppress informational output"),                             \
```

Keep the backslash at column 88. Confirmed 2026-08-28 on PR #3941: this took the CI checkpatch job from 1 error + 2 warnings to 0 errors + 1 warning. The job stays red either way — `checkpatch.pl` exits 1 on warnings alone — but the error is gone.

## Length-preserving rename on an already-over-80 line

`git diff`-based checkpatch only inspects touched lines, so renaming a token to a same-length replacement (e.g. `dhchap-secret` → `kxchap-secret`, both 13 characters) makes an already-over-80-column line "newly" flagged, even though the line's length is completely unchanged by the edit. Confirmed 2026-08-17 on the `kxchap-secret` INI-key rename: `config-ini.c`'s `keys[]` table entries, a `libnvme_msg()` error string in `fabrics.c`, and a test assertion in `config-api.c` all pre-existed at their current column width under the old spelling and were never flagged, because they'd never been part of a diff since. Verify with `git show <base>:<file> | sed -n '<line>p'` before assuming a LONG_LINE warning is new; if the line length is unchanged by your edit, it's pre-existing and out of scope for the rename.

## `-ENOSYS` from a build-time stub — "ENOSYS means 'invalid syscall nr' and nothing else"

That rule is about kernel syscall dispatch. In userspace, ENOSYS means "not implemented", and glibc's own stubs return it for a feature that was not compiled in. A stub such as `discoverd/src/no-mdns.c` returns `-ENOSYS` so the caller can tell "built without this feature" apart from a runtime `-EOPNOTSUPP`. Confirmed by Martin (2026-09-25, PR #4065). Keep `-ENOSYS` and accept the warning.

---

## Not a false positive: checkpatch still gates your own PRs

All of the above are specific, confirmed exceptions. Everything else checkpatch reports — real style violations, genuinely missing blank lines, real line-length problems in code you wrote (not generator annotations), unrelated warnings — must be fixed before a branch is called ready. When `make checkpatch-diff` reports something not on this list, treat it as real until proven otherwise.

If you find a new recurring false positive not listed here, add it — this list is meant to grow, not to be treated as exhaustive on day one.

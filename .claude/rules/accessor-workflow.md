# Accessor regeneration workflow

> All file paths and build commands in this document refer to the **nvme-cli**
> repository at `../nvme-cli/` relative to this repo.

libnvme exposes auto-generated getter/setter functions for its opaque structs.
The generated files are committed to the nvme-cli tree and validated by CI
(`check-accessors.yml`). They must be kept in sync with `private.h` and
`private-fabrics.h`.

## When to regenerate

Regenerate whenever a struct annotated with `!generate-accessors` in
`private.h` or `private-fabrics.h` has a member **added, removed, renamed,
or has its `!access:` annotation changed**.

## How to regenerate

```shell
# Regenerate both common and fabrics accessors in one step:
meson compile -C .build update-accessors

# Or regenerate only one set:
meson compile -C .build update-common-accessors
meson compile -C .build update-fabrics-accessors
```

The script updates `.h` and `.c` files atomically when their content changes.

## Files touched by regeneration

| Target | Input | Generated files |
|---|---|---|
| `update-common-accessors` | `libnvme/src/nvme/private.h` | `accessors.h`, `accessors.c`, `accessors.ld` |
| `update-fabrics-accessors` | `libnvme/src/nvme/private-fabrics.h` | `accessors-fabrics.h`, `accessors-fabrics.c`, `accessors-fabrics.ld` |

## Maintaining the `.ld` version-script files

The `.ld` files are **not** updated automatically — new exported symbols must
be placed in the correct ABI version section by hand.

**During a major version release (e.g. 3.0 alpha/rc):** ABI breaks are
intentional and permitted. Add new symbols directly to the existing version
section (e.g. `LIBNVME_ACCESSORS_3`). Do **not** create a new section.

**After a stable release (3.0 and later):** never change or remove an
existing exported symbol. For a new symbol there are two options:

1. Add it to the existing section (e.g. `LIBNVME_ACCESSORS_3`). Simple to
   maintain. A program that uses the symbol and runs against an older
   library fails only when the symbol is resolved, not at load time.
2. Add it to a new section named after the next release, chaining the
   previous one. The version a program needs is then recorded in the
   binary. A program that uses only older symbols keeps a lower minimum
   libnvme version. Packaging tools read these versions too (RPM generates
   a dependency such as `libnvme.so.1(LIBNVME_3.2)`). The dynamic loader
   reports a missing version at load time.
   ```
   LIBNVME_ACCESSORS_3.2 {
       global:
           libnvme_ctrl_get_new_field;
           libnvme_ctrl_set_new_field;
   } LIBNVME_ACCESSORS_3;
   ```

**Do not choose.** Ask the user which option to use before editing any
`.ld` file, and name the symbols and the release. If a section for the
next release already exists, ask whether to add to it. The maintainer prefers
option 1 (PR #4070 review, 2026-09-25).

When the generator detects drift it prints the following. Its suggestion of a
new section is not a decision: ask the user, as above.
```
WARNING: accessors.ld needs manual attention.

  Symbols to ADD (new version section, e.g. <PREFIX>_ACCESSORS_X_Y):
    libnvme_ctrl_get_new_field
    libnvme_ctrl_set_new_field
```

## Committing

Commit the `private.h` change and the regenerated files together (or as a
logical pair of commits). Do not commit regenerated files without the
corresponding struct change, and do not commit the struct change without
regenerating first.

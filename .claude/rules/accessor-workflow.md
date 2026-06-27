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

**After a stable release:** Adding a symbol to an already-published section
would break binary compatibility. Instead, create a **new** version section
that chains the previous one:
```
LIBNVME_ACCESSORS_3.1 {
    global:
        libnvme_ctrl_get_new_field;
        libnvme_ctrl_set_new_field;
} LIBNVME_ACCESSORS_3;
```

When the generator detects drift it prints:
```
WARNING: accessors.ld needs manual attention.

  Symbols to ADD (new version section, e.g. LIBNVME_ACCESSORS_X_Y):
    libnvme_ctrl_get_new_field
    libnvme_ctrl_set_new_field
```

## Committing

Commit the `private.h` change and the regenerated files together (or as a
logical pair of commits). Do not commit regenerated files without the
corresponding struct change, and do not commit the struct change without
regenerating first.

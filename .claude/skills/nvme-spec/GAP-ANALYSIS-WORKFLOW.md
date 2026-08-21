# Systematic Gap Analysis Workflow

A chapter-by-chapter method for finding what libnvme is *missing* relative to
a spec chapter — as opposed to [COMMAND-VERIFICATION.md](COMMAND-VERIFICATION.md),
which verifies a command you already believe exists. Use this when the task
is "does libnvme have everything in section X", not "check command Y".

Developed and validated across a full pass of NVMe Base Spec 2.4 chapter 5
(Admin Command Set): the opcode table, then a field-level dive into Get
Features, Get Log Page, Identify, Lockdown, and all of chapter 5.3/5.4.
Found real gaps and one real bug (see examples throughout).

## Where the actual spec PDFs live

The skill's own `specs/` directory (referenced elsewhere in this skill) is
typically empty. In practice, spec PDFs for this project are kept in
`~/specs/` (e.g. `~/specs/NVM-Express-Base-Specification-Revision-2.4-*.pdf`),
sometimes with a `~/specs/ECN/` subdirectory for standalone ECN PDFs. Check
both locations before asking the user to download anything:

```bash
find ~ -maxdepth 3 -iname "*nvme*spec*.pdf" -o -iname "*NVM-Express*.pdf" 2>/dev/null
```

## Step 0: extract the spec text once, grep it many times

For a whole-chapter pass, `pdftotext -layout` the entire spec once to a
scratch file and grep/sed line ranges out of it, rather than repeatedly
using the PDF-paging Read tool. It's faster, and `grep -n` gives you line
numbers you can `sed -n 'START,ENDp'` against for any subsection.

```bash
pdftotext -layout ~/specs/NVM-Express-Base-Specification-Revision-*.pdf /tmp/basespec.txt
grep -n "^5\.2\.[0-9]* " /tmp/basespec.txt        # list every command subsection
grep -n "^5\.2\.30\.1\.[0-9]* " /tmp/basespec.txt # list every Feature Identifier subsection
```

Watch for pdftotext artifacts at page breaks: repeated table headers,
duplicated paragraphs, or an occasional dropped row. If a table's rows don't
form a clean arithmetic sequence (e.g. skips a FID that should be there),
re-check the surrounding pages before concluding the spec itself has a gap —
it's more often the text extraction, not the spec.

## Step 1: verify the master enum/table first

Every chapter has one or more "master" tables (Admin Command Opcodes, Log
Page Identifiers, Feature Identifiers, Identify CNS Values, Generic/Command
Specific Status Values). Extract the full table and diff every value against
the corresponding `enum` in `nvme-types-base.h` (or the relevant
`nvme-types-*.h`). This is fast and catches two distinct things:

1. **Missing values** — an opcode/LID/FID/CNS/status code the table defines
   that the enum doesn't have at all.
2. **Legitimate cross-references** — the base spec sometimes says "Refer to
   the NVM Express \<other\> Command Set Specification" for a value (e.g.
   FID `03h` LBA Range Type, LID `28h` Rate Limiting, CNS values that hand
   off to Key Value or ZNS specs). These are NOT gaps — they still get a
   slot in the *same* enum here, because libnvme keeps one flat ID space
   regardless of which spec document defines a given value's format. Confirm
   the value is present under its expected name; don't flag it as
   "shouldn't be here" or "missing" just because its format lives elsewhere.

## Step 2: check constructor existence, not just the enum

Every implemented Admin command in this codebase has a
`nvme_init_<command>()` inline constructor in one of the `nvme-cmds-*.h`
files that fills a `struct libnvme_passthru_cmd`. An opcode/LID/CNS/FID
enum value with **no matching constructor** is the single highest-yield
signal for a real gap — it was true in every one of chapter 5's commands
audited so far.

**Critical**: grep across *all* `nvme-cmds-*.h` files, not just
`nvme-cmds-base.h`:

```bash
grep -rn "nvme_init_get_log_mi_cmd_supported_effects" libnvme/src/nvme/*.h
```

MI-specific and Fabrics-specific constructors live in `nvme-cmds-mi.h` and
`nvme-cmds-fabrics.h` respectively, not in the base file. Grepping only the
base file produces false-positive "gaps" — this happened once in this audit
(NVMe-MI Commands Supported and Effects, LID `0x13`, looked missing until
`nvme-cmds-mi.h` was checked and it turned out to be fully implemented
there).

### Legitimate reasons a constructor is absent (not a gap)

- **Kernel/driver-internal commands.** Create/Delete I/O SQ/CQ (opcodes
  `0x00/0x01/0x04/0x05`) and Doorbell Buffer Config (`0x7C`) have no
  constructor and never will in this codebase: I/O queue lifecycle is
  entirely kernel-managed on Linux, there's no vfio/userspace-driver role
  here (confirmed by grepping for `vfio` project-wide and finding nothing),
  and Doorbell Buffer Config's two-independent-physical-address requirement
  doesn't even fit the standard Linux passthru ioctl ABI that
  `struct libnvme_passthru_cmd` mirrors.
- **Raw protocol tunnels.** NVMe-MI Send/Receive (`0x1D/0x1E`) and Fabrics
  Commands (`0x7F`) have no fixed Dword10-15 layout to wrap — they carry an
  entirely different protocol's bytes. Not a gap.
- **Deliberately deferred, larger features.** The "Exported NVM Subsystem"
  command/feature family (Clear Exported NVM Resource Config, Manage
  Exported Namespace/NVM Subsystem Send/Receive, Manage Exported Port, the
  Exported NVM Subsystem Template UUID List CNS value) is real, sizeable,
  cross-cutting work that spans several chapters. Confirm it's the same
  family before deferring — but do defer it as one unit rather than doing
  it piecemeal.

## Step 3: before writing a new struct/enum, check if it already exists

Repeatedly in this audit, the bitfield shift/mask macros and backing structs
for a "missing" constructor turned out to already exist — added incidentally
during an earlier, unrelated TP/feature session — and only the constructor
itself (the plumbing) was missing. Example: all 17 missing Get/Set Features
constructors in section 5.2.12 reused pre-existing `NVME_FEAT_*` macros;
zero new bitfield definitions were needed for 15 of the 17.

```bash
grep -n "NVME_FEAT_POWER_LIMIT" libnvme/src/nvme/nvme-types-base.h
```

Check before assuming a full new-type implementation is needed. This turns
what looks like a large task into "just write the constructor."

## Step 4: field-level verification, once enum + constructor exist

For each Dword figure in the spec:

1. Extract the bit range for the field (e.g. `31:14`).
2. Find the corresponding `_SHIFT`/`_MASK` pair in the code.
3. Verify `shift` and `shift + popcount(mask) - 1` reproduce the spec's bit
   range exactly. A one-line check:

```python
shift, mask = 0, 0x3f          # values from the code
hi = shift + mask.bit_length() - 1
print(f"computed {hi}:{shift}")   # compare by eye to the spec's "NN:MM"
```

   This is how a real bug was found in Lockdown (5.2.16):
   `NVME_LOCKDOWN_CDW14_UIDX_MASK` was `0x3f` (bits 5:0) where the spec
   says UUID Index is bits `06:00` (`0x7f`) — the only outlier among every
   other UUID Index field in the file. Cross-checking one field's mask
   against *all other same-named fields elsewhere in the file* is a cheap,
   high-yield sanity check beyond just re-reading the spec.
4. Check CQE result-bit decoding gets the same treatment as command input
   fields — it's easy to add the input-side encoding and skip the
   output-side decode enum (e.g. Get Features Figure 201's
   CHANG/NSSPEC/SVBL bits, Abort's IANP bit — both existed as *concepts* in
   the spec long before anyone added a named enum for them).

## Step 5: verify new struct layouts with a standalone test program

Don't trust manual byte-offset arithmetic for anything non-trivial. Compile
a throwaway program against the built headers and check `sizeof`/`offsetof`
for every new struct before committing:

```bash
cat > /tmp/test_layout.c <<'EOF'
#include <stdio.h>
#include <stddef.h>
#include <nvme/nvme-types-base.h>
int main(void) {
	printf("%zu\n", sizeof(struct nvme_new_thing));
	printf("%zu\n", offsetof(struct nvme_new_thing, some_field));
	return 0;
}
EOF
gcc -I libnvme/src -I libnvme/src/nvme -I .build/libnvme/src \
    -o /tmp/test_layout /tmp/test_layout.c
/tmp/test_layout
rm -f /tmp/test_layout /tmp/test_layout.c
```

Note the three include paths: `libnvme/src` (umbrella `<nvme/...>` prefix),
`libnvme/src/nvme` itself, and `.build/libnvme/src` for the generated
`config.h`. Including `<nvme/types.h>` alone is not sufficient for a
standalone test — include the specific `nvme-types-*.h` header directly.

This caught nothing wrong in this audit (every new struct matched on the
first try), but it's what makes that claim trustworthy — see
[feedback: unaligned spec fields need verification] in project memory for a
case from an earlier session where it *did* catch a real bug (4 bytes of
silent padding from a non-4-byte-aligned `__le32`).

## Step 6: file placement conventions

- `nvme-cmds-base.h` / `nvme-types-base.h`: every generic Admin command,
  **including** ones scoped to a specific transport model if they're still
  a normal Admin opcode (Lockdown, Capacity Management, Virtualization
  Management, Discovery Information Management all live here despite being
  memory- or message-transport-specific).
- `nvme-cmds-fabrics.h` / types alongside them in `nvme-types-base.h`:
  commands and log pages that only make sense in an NVMe-oF/Discovery
  context — Cross-Controller Reset, Fabric Zoning Send/Receive/Lookup, Send
  Discovery Log Page, the Discovery/Host Discovery/AVE Discovery/Pull Model
  DDC Request log pages, plus genuine Fabrics-protocol-only operations
  (Connect, Property Get/Set, Authentication).
- `nvme-cmds-mi.h` / `nvme-types-mi.h`: anything under the NVMe-MI opcode
  space.
- New headers need registering in **two** meson lists plus, for new public
  functions, the relevant `.ld` version script — see
  [[feedback_libnvme_new_header_needs_two_meson_lists]] in project memory.
  (Not relevant for `static inline` constructors added to an existing
  header — those need none of this.)

## Step 7: reporting and commit cadence

- Report findings *before* fixing them when a finding implies a judgment
  call (e.g. "is this actually a gap, or a deliberate boundary?"). Report
  after fixing, with the diff, when the finding is unambiguous.
- Batch a whole command/section's fixes into one commit; don't commit
  per-field.
- Always run the full test suite (`meson test -C .build`) including the
  `kdoc` test before considering a batch done — it validates that every
  `@field:` in a kernel-doc comment matches the actual struct/enum member
  names, which is easy to typo when writing many similar constructors in a
  row.
- Keep a running project-memory note of chapter/section coverage so a later
  session can resume without re-auditing what's already done. See
  `project_chapter5_admin_gap_audit.md`-style memory notes for the format:
  what's verified clean, what was found and fixed (with commit hash), and
  what's explicitly deferred and why.

## Quick command reference

```bash
# Pull a spec section by line range once you know it
sed -n '13592,13650p' /tmp/basespec.txt

# Find every nvme_init_* constructor for a family of commands
grep -rn "nvme_init_get_log" libnvme/src/nvme/nvme-cmds-*.h | grep -oE "nvme_init_get_log_[a-z_0-9]+" | sort -u

# Diff a spec table's ID list against the enum in one pass
grep -n "enum nvme_identify_cns" -A 60 libnvme/src/nvme/nvme-types-base.h | grep -E "NVME_IDENTIFY_CNS_.*="

# Check whether a shift/mask pair already exists before adding a new one
grep -n "NVME_FEAT_POWER_LIMIT" libnvme/src/nvme/nvme-types-base.h
```

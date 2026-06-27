# NVMe Spec Verification Workflows

Detailed step-by-step procedures for verifying nvme-cli code compliance with NVMe specifications.

## Prerequisites

### Required Tools

**pahole** - Analyze struct layouts from compiled code

```bash
# Debian/Ubuntu
sudo apt install dwarves

# Fedora/RHEL
sudo dnf install dwarves
```

### Build with Debug Symbols

```bash
cd ../nvme-cli
meson setup .build -Dbuildtype=debug
meson compile -C .build
```

## Individual Struct/Enum Verification

### Phase 1: Locate the Specification

Check available specs:
```bash
ls -la /home/user/nvme-cli-ai/.claude/skills/nvme-spec/specs/
```

If needed spec is missing, user must download from https://nvmexpress.org/specifications/

### Phase 2: Read the Spec Definition

Use the Read tool with PDF support:
```
Read(file_path="/home/user/nvme-cli-ai/.claude/skills/nvme-spec/specs/nvme-base-spec-2.1.pdf", 
     pages="150-155")
```

**Tips for finding pages:**
- Identify structures: Figure 247-270 range (Base Spec 2.x)
- Controller registers: Section 3 (pages 50-150)
- I/O Commands: Section 5
- Admin Commands: Section 4

### Phase 3: Read Current Implementation

Find and read the code:
```bash
# Find struct definition
grep -n "struct nvme_id_ctrl" ../nvme-cli/libnvme/src/nvme/nvme-types*.h

# Read the implementation
# Use Read tool on the found file
```

### Phase 4: Verify with pahole

Get actual compiled struct layout:
```bash
# Show complete layout
pahole -C nvme_id_ctrl .build/libnvme/src/libnvme.so.1

# Check for holes/padding
pahole --holes -C nvme_id_ctrl .build/libnvme/src/libnvme.so.1

# Show total size
pahole -s -C nvme_id_ctrl .build/libnvme/src/libnvme.so.1
```

**Interpreting pahole output:**
```
struct nvme_id_ctrl {
    __le16                     vid;                  /*     0     2 */
    __le16                     ssvid;                /*     2     2 */
    char                       sn[20];               /*     4    20 */
    
    /* size: 4096, cachelines: 64, members: 65 */
};
```
- `/* 0 2 */` = offset 0, size 2 bytes
- `/* XXX N bytes hole */` = unexpected padding (should be reserved field)
- `/* size: 4096 */` = total struct size

### Phase 5: Field-by-Field Comparison

**Checklist:**

**A. Structure Size**
- [ ] Total size matches spec
- [ ] No unexpected padding/holes

**B. Each Field**
- [ ] Field exists
- [ ] Name matches (or reasonable abbreviation)
- [ ] Type correct (byte count, endianness)
- [ ] Offset matches spec

**C. Reserved Fields**
- [ ] All reserved bytes present
- [ ] Named as `rsvdN` (N = byte offset)
- [ ] Correct size

**D. Bitfields** (if applicable)
- [ ] `_SHIFT` and `_MASK` constants defined
- [ ] Accessor macros present
- [ ] All enumerated values defined

**E. Documentation**
- [ ] Kernel-doc comment present
- [ ] All fields documented with `@field:`
- [ ] Spec version/section referenced

### Phase 6: Generate Report

Produce compliance report with:
- Size verification
- Field-by-field table
- Missing/extra fields
- Type issues
- Recommendations

See [SKILL.md](SKILL.md) "Output Format" section for template.

## File-Level Batch Verification

### Step 1: Identify Target File

Resolve file path:
```bash
# If given just filename
find ../nvme-cli/libnvme/src/nvme -name "nvme-types-zns.h"
```

### Step 2: Extract All Definitions

Parse file for all structs and enums:
```bash
# Extract struct definitions
grep -n "^struct nvme_" /path/to/header.h

# Extract enum definitions
grep -n "^enum nvme_" /path/to/header.h

# Get comprehensive list
awk '/^struct nvme_|^enum nvme_/ {print NR": "}' /path/to/header.h
```

Create inventory:
```
Structs found (8):
  Line 32: struct nvme_zns_lbafe
  Line 60: struct nvme_zns_id_ns
  ...

Enums found (6):
  Line 108: enum nvme_zns_zt
  Line 120: enum nvme_zns_za
  ...
```

### Step 3: Determine Spec Source

Map file to spec document:

| Header File | Spec Document |
|-------------|---------------|
| nvme-types-base.h | nvme-base-spec-*.pdf |
| nvme-types-nvm.h | nvme-nvm-command-set-spec-*.pdf |
| nvme-types-zns.h | nvme-zns-command-set-spec-*.pdf |
| nvme-types-fabrics.h | nvme-oF-spec-*.pdf |
| nvme-types-mi.h | nvme-mi-spec-*.pdf |

Check availability:
```bash
ls -la /home/user/nvme-cli-ai/.claude/skills/nvme-spec/specs/ | grep -i zns
```

### Step 4: Batch Verification

For each definition, perform checks:

**Lightweight (all definitions):**
- Name follows `nvme_*` convention
- Has kernel-doc comment
- Type system compliance (`__u8`, `__le16`, etc.)
- Reserved field naming
- Naming pattern compliance (see Step 5)

**Detailed (structs > 16 bytes):**
- Full individual verification workflow
- pahole layout analysis
- Size/offset verification

**Skip detailed for:**
- Small helper structs (< 16 bytes)
- Simple enums

### Step 5: Pattern Detection

Look for systemic issues:
```bash
# Check for wrong types
grep -E "uint[0-9]+_t" /path/to/header.h

# Find structs missing _SHIFT/_MASK
grep -A 20 "^enum nvme_" /path/to/header.h | grep -L "_SHIFT"

# Check reserved field naming
grep "rsvd" /path/to/header.h | grep -v "rsvd[0-9]"
```

**Naming Pattern Checks**:

See [NAMING-PATTERNS.md](NAMING-PATTERNS.md) for comprehensive naming consistency audit:
- Struct name patterns (`nvme_*` prefix, abbreviations)
- Enum name patterns and value naming (`NVME_*` uppercase)
- Bitfield `_SHIFT`/`_MASK` pairs
- Accessor macro presence and naming
- Reserved field naming (`rsvd<offset>`)
- Spec terminology matching

### Step 6: Generate Summary Report

Produce executive summary:
- Total definitions count
- Pass/Warning/Fail statistics
- Per-definition findings
- Pattern analysis (including naming patterns)
- Definition order check
- Naming consistency report
- Recommendations

See [FILE-VERIFICATION-GUIDE.md](FILE-VERIFICATION-GUIDE.md) for output format.

**Naming Pattern Report Section**:
The report should include a dedicated section on naming pattern compliance covering:
- Struct/enum name consistency
- Enum value naming (uppercase, proper prefixes)
- SHIFT/MASK pair completeness
- Accessor macro presence
- Reserved field naming
- Abbreviation consistency
- Spec terminology matching

See [NAMING-PATTERNS.md](NAMING-PATTERNS.md) for detailed criteria.

## Common Verification Tasks

### Verify Register Definition

```bash
# 1. Find register enum
grep -n "enum nvme_csts" ../nvme-cli/libnvme/src/nvme/nvme-types-base.h

# 2. Check for _SHIFT and _MASK
grep -A 20 "enum nvme_csts" header.h | grep -E "SHIFT|MASK"

# 3. Check for accessor macros
grep "#define NVME_CSTS_" header.h
```

**Checklist:**
- [ ] All bitfields have `_SHIFT` constant
- [ ] All bitfields have `_MASK` constant
- [ ] Accessor macros defined using `NVME_GET()`
- [ ] Spec values match

### Check Definition Order

**For registers:**
```bash
# Extract register order
grep "^enum nvme_\(cap\|vs\|intms\|intmc\|cc\|csts\)" header.h

# Compare to spec register offset order:
# 0x00 CAP, 0x08 VS, 0x0C INTMS, 0x10 INTMC, 0x14 CC, 0x1C CSTS
```

**For structs:**
```bash
# Check if helper types come before types that use them
# Check if order matches spec figure numbers
grep -B5 "^struct nvme_" header.h | grep -E "Figure|struct nvme_"
```

### Verify Spec Version

Check if implementation matches latest spec:

1. Identify current spec version in code comments
2. Compare to latest available spec in specs directory
3. Note differences in report
4. Recommend update if significant changes

## Troubleshooting

### pahole shows unexpected holes

**Cause**: Missing reserved fields or wrong field sizes

**Fix**:
1. Compare hole location to spec
2. Add `__u8 rsvdN[size]` field at that offset
3. Rebuild and verify hole is gone

### Size mismatch

**Cause**: Missing fields or wrong array sizes

**Fix**:
1. Calculate expected size from spec
2. Find which fields are missing/wrong
3. Add missing fields or correct sizes
4. Verify with pahole

### pahole not available

**Fallback**: Manual offset calculation
```c
#include <stddef.h>
printf("offset of field: %zu\n", offsetof(struct nvme_id_ctrl, field));
printf("size: %zu\n", sizeof(struct nvme_id_ctrl));
```

### Spec PDF not available

**Action**: Inform user to download:
```
⚠️ Spec not found: nvme-zns-command-set-spec-*.pdf
Please download from https://nvmexpress.org/specifications/
and place in /home/user/nvme-cli-ai/.claude/skills/nvme-spec/specs/
```

## Quick Reference Commands

```bash
# Find struct in code
grep -rn "struct nvme_id_ctrl" ../nvme-cli/libnvme/src/nvme/

# Get struct layout
pahole -C nvme_id_ctrl .build/libnvme/src/libnvme.so.1

# Check for holes
pahole --holes -C nvme_id_ctrl .build/libnvme/src/libnvme.so.1

# List all definitions in file
grep -n "^\(struct\|enum\) nvme_" header.h

# Check type usage
grep -E "__u[0-9]+|__le[0-9]+|uint[0-9]+_t" header.h

# Verify order
grep -n "^enum nvme_" header.h | head -20
```

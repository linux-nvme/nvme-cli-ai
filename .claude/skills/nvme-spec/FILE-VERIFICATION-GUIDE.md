# File-Level Verification Quick Reference

## Overview

The `/nvme-spec` skill now supports batch verification of entire header files, in addition to individual struct/enum checking.

## Quick Commands

### Basic File Verification

```bash
# Verify all definitions in a file
/nvme-spec verify-file nvme-types-zns.h

# Alternative invocations (all equivalent)
/nvme-spec check-file nvme-types-nvm.h
/nvme-spec audit nvme-types-fabrics.h
```

### Output Modes

```bash
# Default: Summary report with details
/nvme-spec verify-file nvme-types-zns.h

# Compact mode (for CI)
/nvme-spec audit nvme-types-zns.h --compact
# Output: nvme-types-zns.h: 14 definitions, 13 pass, 1 warning, 0 fail (93%)

# Detailed mode (full spec check for every definition)
/nvme-spec verify-file nvme-types-base.h --detailed
```

### Pattern Detection

```bash
# Check for systemic issues across the file
/nvme-spec check-patterns nvme-types-base.h

# Detects:
# - Wrong type usage patterns (uint8_t vs __u8)
# - Missing _SHIFT/_MASK in bitfield enums
# - Missing accessor macros
# - Inconsistent reserved field naming
# - Out-of-order definitions
# - Naming pattern violations (see NAMING-PATTERNS.md)
```

## What Gets Checked

### For All Definitions

- ✅ Naming convention (`nvme_*` or `nvmf_*`)
- ✅ Naming pattern compliance (prefixes, abbreviations, spec terminology)
- ✅ Kernel-doc comment presence
- ✅ Type system compliance (`__u8`, `__le16`, etc.)
- ✅ Reserved field naming (`rsvd<offset>` pattern)

### For Structs (> 16 bytes)

- ✅ Size verification (if spec known)
- ✅ Field offset verification via `pahole`
- ✅ Complete field list
- ✅ Correct types (endianness)

### For Enums

- ✅ Enum name patterns (`nvme_<concept>`)
- ✅ Enum value patterns (`NVME_<ENUM>_<VALUE>`)
- ✅ Bitfield `_SHIFT`/`_MASK` presence (both required)
- ✅ Accessor macro availability (`NVME_<STRUCT>_<FIELD>()`)

## Report Structure

```markdown
# File Verification Report: nvme-types-zns.h

## Executive Summary
- Total definitions: 14 (8 structs, 6 enums)
- ✅ Pass: 13
- ⚠️ Warning: 1
- ❌ Fail: 0
- Overall: 93% compliant

## Detailed Results
### Structs
- ✅ nvme_zns_lbafe (line 32): fully compliant
- ⚠️ nvme_zns_id_ctrl (line 87): missing field documentation
  
### Enums
- ✅ nvme_zns_zt (line 108): compliant
...

## Naming Pattern Compliance

### Structs (8 total)
- ✅ All struct names follow nvme_zns_* pattern
- ✅ All use standard abbreviations consistently
- ✅ Reserved fields use rsvd<offset> pattern

### Enums (6 total)  
- ✅ All enum names follow nvme_zns_* pattern
- ✅ All enum values use NVME_ZNS_* uppercase pattern
- ✅ All bitfields have both SHIFT and MASK constants
- ✅ All bitfields have accessor macros

### Spec Terminology
- ✅ Names closely follow ZNS spec terminology
- ⚠️ Consider: nvme_zns_changed_zone_log (currently verbose but acceptable)

See NAMING-PATTERNS.md for detailed naming conventions.

## Definition Order Analysis
- ✅ Matches spec order
- Helper types before structs that use them

## Recommendations
- Required: (none)
- Optional: Add detailed docs to nvme_zns_id_ctrl.zasl field
```

## Use Cases

### Pre-Release Audit

```bash
# Check all type headers before release
/nvme-spec verify-file nvme-types-base.h
/nvme-spec verify-file nvme-types-nvm.h
/nvme-spec verify-file nvme-types-zns.h
/nvme-spec verify-file nvme-types-fabrics.h
/nvme-spec verify-file nvme-types-mi.h
```

### CI Integration

```bash
# In CI script:
/nvme-spec audit nvme-types-zns.h --compact
exit_code=$?

# Exit code: 0 if all pass/warning, 1 if any critical failures
```

### Development Workflow

```bash
# 1. Individual check while developing
/nvme-spec check nvme_new_struct

# 2. File-level check before commit
/nvme-spec verify-file nvme-types-base.h

# 3. If issues found, drill down
/nvme-spec check nvme_id_ctrl  # detailed analysis
```

## File Size Considerations

| File | Lines | Definitions | Est. Time |
|------|-------|-------------|-----------|
| nvme-types-zns.h | 235 | ~14 | < 30 sec |
| nvme-types-nvm.h | 814 | ~30 | 1-2 min |
| nvme-types-fabrics.h | 569 | ~25 | 1 min |
| nvme-types-base.h | 9038 | ~150 | 5-10 min |

For large files:
- Progress updates shown during verification
- Results grouped by section
- Use `--compact` mode for quick checks

## Header File Mapping

| Header File | Spec Document |
|-------------|---------------|
| nvme-types-base.h | NVMe Base Specification |
| nvme-types-nvm.h | NVM Command Set Spec |
| nvme-types-zns.h | ZNS Command Set Spec |
| nvme-types-fabrics.h | NVMe-oF Specification |
| nvme-types-mi.h | NVMe-MI Specification |

## When to Use File vs Individual Verification

### Use File-Level When:

- ✅ Pre-release compliance audit
- ✅ Detecting patterns of issues
- ✅ New file creation/major refactor
- ✅ Generating compliance reports
- ✅ CI/CD automated checks

### Use Individual When:

- ✅ Developing a single new type
- ✅ Debugging specific struct issue
- ✅ Need detailed offset tables
- ✅ Iterative development
- ✅ Quick feedback cycle

## Examples from Real Files

### Small File (nvme-types-zns.h - 235 lines)

```bash
/nvme-spec verify-file nvme-types-zns.h

# Expected output:
# - 8 structs checked
# - 6 enums checked  
# - Completes in ~30 seconds
# - Summary report with all findings
```

### Large File (nvme-types-base.h - 9038 lines)

```bash
# Compact check first
/nvme-spec audit nvme-types-base.h --compact

# If issues found, detailed check
/nvme-spec verify-file nvme-types-base.h

# Progress shown:
# "Checking definitions 1-50..."
# "Checking definitions 51-100..."
# Results grouped by section (registers, identify, logs, etc.)
```

## See Also

- Full documentation: `/home/user/nvme-cli-ai/.claude/skills/nvme-spec/SKILL.md`
- Invoke help: `/nvme-spec help`
- Type implementation guide: See SKILL.md "Type System Implementation Guide" section

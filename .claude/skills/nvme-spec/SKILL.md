---
name: nvme-spec
description: Reference the NVMe specification to verify code compliance and implement data types consistently. Use when checking if code follows NVMe spec requirements, validating command implementations, verifying data structure layouts, or refactoring types for spec compliance.
tools: Read, Bash, Write
---

# NVMe Specification Handler

References the NVMe specification to verify that nvme-cli code complies with the official NVMe specification, and provides guidance for implementing NVMe data types and commands consistently in C code.

## Quick Start

This skill provides three main capabilities:

1. **Type Implementation Guidance**: Learn how to implement NVMe types following established patterns
   - See [REFERENCE.md](REFERENCE.md) for the complete Type System Implementation Guide

2. **Spec Compliance Checking**: Verify existing code matches the NVMe specification
   - Individual struct/enum verification
   - Full-file batch verification
   - Automated layout analysis using `pahole`
   - Completeness checking (missing fields, wrong types, incorrect offsets)
   - See [WORKFLOWS.md](WORKFLOWS.md) for detailed verification procedures

3. **Command Implementation Verification**: Verify NVMe commands match spec requirements
   - Command opcode verification
   - Command structure field mapping
   - Input/output parameter validation
   - Admin vs I/O command classification
   - See [COMMAND-VERIFICATION.md](COMMAND-VERIFICATION.md) for detailed procedures

**Common use cases:**
- `/nvme-spec check nvme_id_ctrl` - Check if struct is complete and matches spec
- `/nvme-spec verify nvme_lbaf` - Verify struct against spec definition
- `/nvme-spec check CSTS register` - Check register bitfield implementation
- `/nvme-spec verify-file nvme-types-zns.h` - Batch verify all types in a file (includes naming audit)
- `/nvme-spec verify-command identify` - Verify Identify command implementation
- `/nvme-spec audit-commands nvme-cmds.h` - Audit all command definitions
- `/nvme-spec refactor types` - Get type system refactoring guidance

## When to Use

### Individual Verification
Use for:
- Implementing a new type - verify a single struct/enum
- Debugging a specific issue
- Detailed analysis with offset tables
- Iterative development with quick feedback
- Working with large files (don't want full scan)

**Example:**
```bash
/nvme-spec check nvme_id_ctrl
```

### File-Level Verification
Use for:
- Pre-release audit of all types
- Pattern detection across many definitions
- New file creation/verification
- Compliance reporting for documentation
- CI/CD integration
- Spec version migration overview

**Example:**
```bash
/nvme-spec verify-file nvme-types-zns.h
```

### Comparison

| Aspect | Individual | File-Level |
|--------|-----------|------------|
| **Time** | Fast (seconds) | Slower (minutes for large files) |
| **Detail** | Very detailed | Summary + selective detail |
| **Scope** | One definition | All definitions in file |
| **Output** | Full spec comparison | Executive summary + findings |
| **Best for** | Development | Audit/Release/CI |

## Specification Sources

**Location**: `/home/user/nvme-cli-ai/.claude/skills/nvme-spec/specs/`

Check available specifications:
```bash
ls -la /home/user/nvme-cli-ai/.claude/skills/nvme-spec/specs/
```

**Manual Download Required**: Due to nvmexpress.org's anti-bot protections, specifications must be manually downloaded by the user.

**User Instructions**:
1. Visit https://nvmexpress.org/specifications/
2. Download the desired specifications (PDF format)
3. Place them in the specs directory

**Naming Convention**:
- `nvme-base-spec-2.1.pdf`
- `nvme-nvm-command-set-spec-1.0c.pdf`
- `nvme-zns-command-set-spec-1.1a.pdf`
- `nvme-oF-spec-1.1c.pdf`
- `nvme-mi-spec-1.2a.pdf`

## Header File to Spec Mapping

| Header File | Spec Document | Lines | Definitions |
|-------------|---------------|-------|-------------|
| nvme-types-base.h | NVMe Base Specification | 9038 | ~150 |
| nvme-types-nvm.h | NVM Command Set Spec | 814 | ~30 |
| nvme-types-zns.h | ZNS Command Set Spec | 235 | ~14 |
| nvme-types-fabrics.h | NVMe-oF Specification | 569 | ~25 |
| nvme-types-mi.h | NVMe-MI Specification | 461 | ~20 |

## Quick Examples

### Individual Struct Verification

```bash
# Check if struct is complete and matches spec
/nvme-spec check nvme_id_ctrl

# Verify with detailed offset analysis
/nvme-spec verify nvme_id_ns
```

**Output**: Full completeness report with:
- Size verification (expected vs actual)
- Field-by-field offset table
- Missing/extra fields
- Type correctness
- Recommendations

### File-Level Verification

```bash
# Verify entire file (default: summary report)
/nvme-spec verify-file nvme-types-zns.h

# Compact mode (for CI)
/nvme-spec audit nvme-types-zns.h --compact

# Detailed mode (full analysis of every definition)
/nvme-spec verify-file nvme-types-base.h --detailed
```

**Output**: Executive summary with:
- Pass/Warning/Fail statistics
- Per-definition findings
- Pattern detection
- Definition order analysis
- Recommendations

### Register Verification

```bash
# Check register bitfield implementation
/nvme-spec check CSTS register

# Verify accessor macros present
/nvme-spec check CAP register bitfields
```

### Pattern Detection

```bash
# Find systemic issues across a file (included in verify-file)
/nvme-spec check-patterns nvme-types-base.h
```

Detects:
- Wrong type usage patterns (`uint8_t` vs `__u8`)
- Missing `_SHIFT`/`_MASK` in bitfield enums
- Missing accessor macros
- Inconsistent reserved field naming
- Out-of-order definitions
- Naming pattern violations (struct/enum names, prefixes, abbreviations)

See [NAMING-PATTERNS.md](NAMING-PATTERNS.md) for detailed naming conventions.

### Command Verification

```bash
# Verify a specific command implementation
/nvme-spec verify-command identify

# Check admin command opcodes
/nvme-spec verify-opcodes admin

# Check I/O command opcodes
/nvme-spec verify-opcodes io

# Audit all commands in a file
/nvme-spec audit-commands nvme-cmds.h
```

**Output**: Command verification report with:
- Opcode value verification
- Command structure layout (DW0-15)
- Field offset and type checking
- Data transfer direction
- Function signature verification

## Workflow Overview

### Individual Verification Workflow

1. **Locate spec** - Find relevant PDF in specs directory
2. **Read spec definition** - Extract figure/table from PDF
3. **Read code** - Get current implementation
4. **Compare** - Field-by-field verification using checklist
5. **Use pahole** - Verify actual compiled layout
6. **Report findings** - Comprehensive compliance report

See [WORKFLOWS.md](WORKFLOWS.md) for detailed step-by-step procedures.

### File-Level Verification Workflow

1. **Extract definitions** - Parse file for all structs/enums
2. **Determine spec source** - Map file to spec document
3. **Batch verify** - Check each definition (lightweight or detailed)
4. **Detect patterns** - Find systemic issues
5. **Generate report** - Executive summary + detailed findings

See [FILE-VERIFICATION-GUIDE.md](FILE-VERIFICATION-GUIDE.md) for quick reference.

### Command Verification Workflow

1. **Identify command** - Determine which command to verify (admin/io/nvm)
2. **Locate spec** - Find command definition in appropriate spec
3. **Verify opcode** - Check opcode value matches spec
4. **Verify structure** - Field-by-field DW0-15 verification
5. **Verify function** - Check function signature and parameters
6. **Use pahole** - Verify compiled structure size and offsets
7. **Report findings** - Comprehensive command compliance report

See [COMMAND-VERIFICATION.md](COMMAND-VERIFICATION.md) for detailed step-by-step procedures.

## Practical Use Cases

### Pre-Release Audit

```bash
# Check all type headers before release
/nvme-spec verify-file nvme-types-base.h
/nvme-spec verify-file nvme-types-nvm.h
/nvme-spec verify-file nvme-types-zns.h
/nvme-spec verify-file nvme-types-fabrics.h
/nvme-spec verify-file nvme-types-mi.h
```

### Development Workflow

```bash
# 1. While developing: individual checks
/nvme-spec check nvme_new_struct
# ... fix issues, iterate ...

# 2. Before commit: file-level verification
/nvme-spec verify-file nvme-types-base.h

# 3. If issues found: drill down
/nvme-spec check nvme_id_ctrl  # detailed analysis
```

### CI Integration

```bash
# Compact mode for CI scripts
/nvme-spec audit nvme-types-zns.h --compact

# Output: nvme-types-zns.h: 14 definitions, 13 pass, 1 warning, 0 fail (93%)
# Exit code: 0 if pass/warning, 1 if critical failures
```

### Type System Refactoring

```bash
# Get type implementation guidance
/nvme-spec refactor types

# Check patterns before refactoring
/nvme-spec check-patterns nvme-types-base.h

# Verify after refactoring
/nvme-spec verify-file nvme-types-base.h
```

### Command Implementation Workflow

```bash
# 1. Implementing a new command: verify against spec
/nvme-spec verify-command get-log-page

# 2. Verify all admin command opcodes
/nvme-spec verify-opcodes admin

# 3. Audit all commands in the file
/nvme-spec audit-commands libnvme/src/nvme/types.h

# 4. Check specific command structure with pahole
pahole -C nvme_identify .build/libnvme/src/libnvme.so.3
```

## Tools Used

### pahole (Required)

Used for analyzing actual struct layouts from compiled code.

**Installation**:
```bash
# Debian/Ubuntu
sudo apt install dwarves

# Fedora/RHEL
sudo dnf install dwarves
```

**Usage**:
```bash
# Build with debug symbols
meson setup .build -Dbuildtype=debug
meson compile -C .build

# Analyze struct layout
pahole -C nvme_id_ctrl .build/libnvme/src/libnvme.so.1

# Check for holes/padding
pahole --holes -C nvme_id_ctrl .build/libnvme/src/libnvme.so.1
```

### PDF Reading

The skill uses the Read tool with PDF support to extract spec definitions:
```
Read(file_path="/path/to/spec.pdf", pages="150-155")
```

For large PDFs, specify page ranges to avoid loading entire document.

## Output Format

### Individual Verification Report

```markdown
## Spec Compliance Check: nvme_id_ctrl

**Spec Reference**: NVMe Base Spec 2.1, Figure 247
**Code Location**: libnvme/src/nvme/nvme-types-base.h:1234
**Spec Size**: 4096 bytes
**Code Size**: 4096 bytes

### Summary
| Category | Status |
|----------|--------|
| Size | ✅ Match |
| Field Count | ✅ All present |
| Types | ✅ Correct |
| Offsets | ✅ Aligned |

### Findings
✅ All fields present and correctly typed
✅ Offsets match spec exactly
⚠️ Field 'oacs' missing detailed documentation

### Recommendations
- Optional: Add detailed description to oacs field
```

### File-Level Verification Report

```markdown
# File Verification Report: nvme-types-zns.h

## Executive Summary
- Total: 14 definitions (8 structs, 6 enums)
- ✅ Pass: 13 | ⚠️ Warning: 1 | ❌ Fail: 0
- Overall: 93% compliant

## Detailed Results
### Structs
✅ nvme_zns_lbafe (16 bytes) - fully compliant
⚠️ nvme_zns_id_ctrl (4096 bytes) - missing field docs

### Definition Order
✅ Matches spec order

## Recommendations
- Optional: Enhance documentation
```

## Documentation

- **SKILL.md** - This file (skill overview and quick start)
- **REFERENCE.md** - Type System Implementation Guide (detailed reference)
- **WORKFLOWS.md** - Detailed verification procedures (step-by-step)
- **FILE-VERIFICATION-GUIDE.md** - File-level verification quick reference
- **NAMING-PATTERNS.md** - Naming consistency audit guide (structs, enums, getters/setters)
- **COMMAND-VERIFICATION.md** - Command implementation verification guide

## Notes

- PDFs are read-only references, never modified
- Keep multiple spec versions for backwards compatibility checking
- Reference the spec version in all compliance reports
- When checking code, always cite the specific spec section and version
- For multi-version projects, note which spec version the implementation targets

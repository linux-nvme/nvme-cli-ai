# NVMe Spec Verification Skill

This skill helps verify that nvme-cli code complies with the official NVMe specification and provides guidance for implementing NVMe data types consistently.

## Documentation Structure

The skill documentation is organized into focused files:

| File | Purpose | Lines | Use When |
|------|---------|-------|----------|
| **[SKILL.md](SKILL.md)** | Main skill definition and quick start | 341 | First-time use, overview, invocation examples |
| **[REFERENCE.md](REFERENCE.md)** | Type system implementation guide | 201 | Implementing new types, refactoring types |
| **[WORKFLOWS.md](WORKFLOWS.md)** | Detailed verification procedures | 337 | Step-by-step verification instructions |
| **[FILE-VERIFICATION-GUIDE.md](FILE-VERIFICATION-GUIDE.md)** | File-level verification quick reference | 211 | Batch verification, CI integration |

## Quick Start

### Install This Skill

The skill is already installed in `/home/user/nvme-cli-ai/.claude/skills/nvme-spec/`

### Download Specifications

Specifications must be manually downloaded and placed in `specs/` directory:

1. Visit https://nvmexpress.org/specifications/
2. Download desired specs (PDF format)
3. Place in `/home/user/nvme-cli-ai/.claude/skills/nvme-spec/specs/`

**Naming convention:**
- `nvme-base-spec-2.1.pdf`
- `nvme-nvm-command-set-spec-1.0c.pdf`
- `nvme-zns-command-set-spec-1.1a.pdf`

### Common Commands

```bash
# Verify individual struct
/nvme-spec check nvme_id_ctrl

# Verify entire header file
/nvme-spec verify-file nvme-types-zns.h

# Get type implementation guidance
/nvme-spec refactor types

# Pattern detection across file
/nvme-spec check-patterns nvme-types-base.h
```

## Which Document to Read?

### I want to...

**...understand what this skill does**
→ Read [SKILL.md](SKILL.md) "Quick Start" section

**...verify a specific struct matches the spec**
→ Read [WORKFLOWS.md](WORKFLOWS.md) "Individual Struct/Enum Verification"

**...implement a new NVMe type**
→ Read [REFERENCE.md](REFERENCE.md) "Struct Implementation Pattern"

**...verify an entire header file**
→ Read [FILE-VERIFICATION-GUIDE.md](FILE-VERIFICATION-GUIDE.md)

**...understand bitfield patterns**
→ Read [REFERENCE.md](REFERENCE.md) "Bitfield/Enum Implementation Pattern"

**...fix type system issues**
→ Read [REFERENCE.md](REFERENCE.md) "Common Type Issues to Fix"

**...set up CI checks**
→ Read [FILE-VERIFICATION-GUIDE.md](FILE-VERIFICATION-GUIDE.md) "CI Integration"

**...understand pahole usage**
→ Read [WORKFLOWS.md](WORKFLOWS.md) "Phase 4: Verify with pahole"

## Capabilities

### Individual Verification
- Check single struct/enum against spec
- Detailed offset analysis using `pahole`
- Field-by-field comparison
- Type correctness validation
- Comprehensive compliance reports

### File-Level Verification
- Batch verify all types in a header file
- Pattern detection across many definitions
- Executive summary with statistics
- Definition order analysis
- CI-friendly compact output mode

### Type Implementation Guidance
- Base type system reference
- Struct implementation patterns
- Bitfield/enum patterns
- Helper macro usage
- Refactoring checklists

## File Organization

```
nvme-spec/
├── README.md                      ← You are here
├── SKILL.md                       ← Main skill definition
├── REFERENCE.md                   ← Type system implementation guide
├── WORKFLOWS.md                   ← Detailed verification procedures
├── FILE-VERIFICATION-GUIDE.md     ← File-level verification reference
└── specs/                         ← NVMe specification PDFs (user-provided)
    ├── nvme-base-spec-2.1.pdf
    ├── nvme-nvm-command-set-spec-1.0c.pdf
    └── ... (other spec versions)
```

## Examples

### Example 1: Verify a struct you just implemented

```bash
/nvme-spec check nvme_new_struct
```

Gets detailed report with:
- Size verification
- Field-by-field offset table
- Missing/wrong fields
- Type correctness
- Recommendations

### Example 2: Pre-release audit

```bash
/nvme-spec verify-file nvme-types-base.h
```

Gets executive summary with:
- Pass/Warning/Fail statistics
- Per-definition findings
- Pattern issues
- Definition order analysis

### Example 3: CI integration

```bash
/nvme-spec audit nvme-types-zns.h --compact
```

Compact output:
```
nvme-types-zns.h: 14 definitions, 13 pass, 1 warning, 0 fail (93%)
```

## Requirements

**pahole** - Analyze struct layouts (required for detailed verification)

```bash
# Debian/Ubuntu
sudo apt install dwarves

# Fedora/RHEL
sudo dnf install dwarves
```

**nvme-cli built with debug symbols** - For pahole analysis

```bash
cd ../nvme-cli
meson setup .build -Dbuildtype=debug
meson compile -C .build
```

## Contributing

This skill is part of the nvme-cli-ai repository. Improvements and updates are welcome.

**Key principles:**
- Keep SKILL.md concise (< 500 lines)
- Detailed procedures go in WORKFLOWS.md
- Reference material goes in REFERENCE.md
- File-level specifics go in FILE-VERIFICATION-GUIDE.md
- Cross-reference between docs liberally

## See Also

- nvme-cli project: https://github.com/linux-nvme/nvme-cli
- NVMe specifications: https://nvmexpress.org/specifications/
- libnvme documentation: https://libnvme.github.io/

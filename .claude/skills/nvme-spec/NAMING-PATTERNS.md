# NVMe Naming Pattern Audit Guide

This guide covers auditing and verifying consistent naming patterns for NVMe types, structs, enums, and accessor macros to ensure they follow the NVMe specification closely while maintaining consistency across the codebase.

## Overview

Consistent naming patterns are critical for:
- **Spec traceability** — Names should clearly map to spec terminology
- **Code readability** — Developers can quickly understand what a type represents
- **API consistency** — Similar patterns for similar concepts
- **Maintainability** — New additions follow established conventions

## Naming Pattern Rules

### 1. Struct Names

**Pattern**: `struct nvme_<spec_acronym>[_<qualifier>]`

**Rules**:
- Always use `nvme_` prefix for base spec types
- Use command-set specific prefixes: `nvme_zns_`, `nvme_mi_`, `nvmf_`
- Follow spec terminology as closely as possible
- Use lowercase snake_case
- Use abbreviations when spec names are too long (see Abbreviation Guide)

**Examples**:
```c
✅ struct nvme_id_ctrl           // Identify Controller (spec: "Identify Controller data structure")
✅ struct nvme_id_ns             // Identify Namespace
✅ struct nvme_smart_log         // SMART/Health Information Log
✅ struct nvme_zns_id_ns         // ZNS Identify Namespace
✅ struct nvme_zns_changed_zone_log  // ZNS Changed Zone List Log
✅ struct nvmf_disc_log_entry    // Fabrics Discovery Log Entry

❌ struct nvme_identify_controller_data_structure  // Too verbose
❌ struct nvme_ctrl_id           // Doesn't match spec terminology order
❌ struct NvmeIdCtrl             // Wrong case style
```

### 2. Enum Names

**Pattern**: `enum nvme_<concept>[_<qualifier>]`

**Rules**:
- Use `enum` for sets of related constants
- Name should describe what the enum represents, not just list items
- Follow spec section/figure terminology
- Use lowercase snake_case

**Examples**:
```c
✅ enum nvme_admin_opcode        // Admin Command Opcodes
✅ enum nvme_io_opcode           // I/O Command Opcodes
✅ enum nvme_identify_cns        // Identify CNS values
✅ enum nvme_features_id         // Feature Identifiers
✅ enum nvme_zns_zt              // Zone Type
✅ enum nvme_zns_za              // Zone Attributes
✅ enum nvme_zns_zs              // Zone State

❌ enum nvme_admin_commands      // Use "opcode" to match spec
❌ enum identify_cns_values      // Missing nvme_ prefix
```

### 3. Enum Value Names

**Pattern**: `NVME_<ENUM_CONCEPT>_<VALUE_NAME>`

**Rules**:
- All uppercase with underscores
- Include enum concept in the prefix
- Use spec terminology for value names
- Be specific but concise

**Examples**:
```c
// Admin opcodes
enum nvme_admin_opcode {
    ✅ NVME_ADMIN_DELETE_SQ        = 0x00,
    ✅ NVME_ADMIN_CREATE_SQ        = 0x01,
    ✅ NVME_ADMIN_GET_LOG_PAGE    = 0x02,
    ✅ NVME_ADMIN_IDENTIFY        = 0x06,
    
    ❌ NVME_DELETE_SQ        = 0x00,  // Missing ADMIN qualifier
    ❌ delete_sq            = 0x00,  // Wrong case
};

// Identify CNS values
enum nvme_identify_cns {
    ✅ NVME_IDENTIFY_CNS_NS            = 0x00,
    ✅ NVME_IDENTIFY_CNS_CTRL        = 0x01,
    ✅ NVME_IDENTIFY_CNS_NS_ACTIVE_LIST    = 0x02,
    
    ❌ NVME_CNS_NS            = 0x00,  // Unclear what CNS is
    ❌ NVME_IDENTIFY_NAMESPACE    = 0x00,  // Doesn't match spec term CNS
};

// Zone types
enum nvme_zns_zt {
    ✅ NVME_ZNS_ZT_SEQ_WRITE_REQ    = 0x02,
    
    ❌ NVME_ZNS_SEQUENTIAL        = 0x02,  // Doesn't match spec ZT abbreviation
    ❌ ZNS_ZT_SEQ_WRITE_REQ        = 0x02,  // Missing NVME_ prefix
};
```

### 4. Bitfield Constants (SHIFT and MASK)

**Pattern**: 
- `NVME_<STRUCT>_<FIELD>_SHIFT`
- `NVME_<STRUCT>_<FIELD>_MASK`

**Rules**:
- Both SHIFT and MASK must be defined for each bitfield
- Use the struct/register name and field name
- SHIFT defines bit position, MASK defines the field width

**Examples**:
```c
// For register bitfields
enum nvme_csts {
    ✅ NVME_CSTS_RDY_SHIFT        = 0,
    ✅ NVME_CSTS_RDY_MASK        = 0x1,
    ✅ NVME_CSTS_CFS_SHIFT        = 1,
    ✅ NVME_CSTS_CFS_MASK        = 0x1,
    ✅ NVME_CSTS_SHST_SHIFT        = 2,
    ✅ NVME_CSTS_SHST_MASK        = 0x3,
    
    ❌ NVME_CSTS_RDY        = 0x1,   // Missing _SHIFT/_MASK pattern
    ❌ CSTS_RDY_SHIFT        = 0,     // Missing NVME_ prefix
};

// For struct field bitfields
enum nvme_id_ctrl_oncs {
    ✅ NVME_ID_CTRL_ONCS_COMPARE_SHIFT        = 0,
    ✅ NVME_ID_CTRL_ONCS_COMPARE_MASK        = 0x1,
    ✅ NVME_ID_CTRL_ONCS_WRITE_UNCORRECTABLE_SHIFT    = 1,
    ✅ NVME_ID_CTRL_ONCS_WRITE_UNCORRECTABLE_MASK    = 0x1,
};
```

### 5. Accessor Macros (Getters/Setters)

**Pattern**: 
- `NVME_<STRUCT>_<FIELD>(value)` for getters
- `NVME_SET_<STRUCT>_<FIELD>(value, new_value)` for setters

**Rules**:
- Use `NVME_GET()` helper macro for bitfield extraction
- Use `NVME_SET()` helper macro for bitfield assignment
- Macro name should match the field being accessed
- First parameter is always the value being read/modified

**Examples**:
```c
// Register accessors
✅ #define NVME_CSTS_RDY(csts)        NVME_GET(csts, CSTS_RDY)
✅ #define NVME_CSTS_CFS(csts)        NVME_GET(csts, CSTS_CFS)
✅ #define NVME_CSTS_SHST(csts)        NVME_GET(csts, CSTS_SHST)

// Struct field accessors
✅ #define NVME_ID_CTRL_ONCS_COMPARE(oncs)    NVME_GET(oncs, ID_CTRL_ONCS_COMPARE)

// ZNS accessors
✅ #define NVME_ZNS_ZOC_VCB(zoc)        NVME_GET(zoc, ZNS_ZOC_VCB)
✅ #define NVME_ZNS_ZOC_ZAE(zoc)        NVME_GET(zoc, ZNS_ZOC_ZAE)

❌ #define GET_NVME_CSTS_RDY(csts)        NVME_GET(csts, CSTS_RDY)  // Wrong prefix order
❌ #define NVME_CSTS_RDY_GET(csts)        NVME_GET(csts, CSTS_RDY)  // Redundant _GET suffix
❌ #define nvme_csts_rdy(csts)        NVME_GET(csts, CSTS_RDY)  // Wrong case
```

### 6. Reserved Field Names

**Pattern**: `__u8 rsvd<byte_offset>[<size>]`

**Rules**:
- Always name reserved fields as `rsvd` followed by byte offset
- Use array notation for multi-byte reserved regions
- Byte offset should be the actual byte offset in the structure
- If multiple reserved fields at different offsets, each gets its own offset

**Examples**:
```c
struct nvme_identify {
    __u8  opcode;        // offset 0
    __u8  flags;         // offset 1
    __le16 command_id;   // offset 2
    __le32 nsid;         // offset 4
    ✅ __u64 rsvd8[2];   // offset 8, 16 bytes reserved
    union nvme_data_ptr dptr;  // offset 24
    __u8  cns;           // offset 40
    ✅ __u8  rsvd41;     // offset 41, 1 byte reserved
    __le16 ctrlid;       // offset 42
    ✅ __u8  rsvd44[3];  // offset 44, 3 bytes reserved
    __u8  csi;           // offset 47
    ✅ __le32 rsvd48[4]; // offset 48, 16 bytes reserved
    
    ❌ __u64 reserved[2];     // Missing offset number
    ❌ __u64 rsvd1[2];        // Wrong offset (should be 8)
    ❌ __u64 rsvd_8_23[2];    // Too verbose, just use start offset
};
```

## Abbreviation Guidelines

When spec terms are too long, use standard abbreviations:

| Full Term | Abbreviation | Example |
|-----------|--------------|---------|
| Identify | id | `nvme_id_ctrl` not `nvme_identify_controller` |
| Controller | ctrl | `nvme_id_ctrl` not `nvme_id_controller` |
| Namespace | ns | `nvme_id_ns` not `nvme_id_namespace` |
| Command | cmd | `nvme_admin_cmd` not `nvme_admin_command` |
| Submission Queue | sq | `nvme_create_sq` not `nvme_create_submission_queue` |
| Completion Queue | cq | `nvme_create_cq` not `nvme_create_completion_queue` |
| Features | feat | `nvme_feat_id` not `nvme_features_identifier` |
| Optional | opt | `nvme_opt_cmd` not `nvme_optional_command` |
| Capabilities | cap | `nvme_cap` not `nvme_capabilities` |
| Status | sts or stat | `nvme_csts` not `nvme_controller_status` |
| Configuration | cfg or cc | `nvme_cc` not `nvme_controller_configuration` |
| Zone | (keep) | `nvme_zns_zone` not `nvme_zns_z` |
| Management | mgmt | `nvme_zone_mgmt` not `nvme_zone_management` |

**Abbreviation Rules**:
- Only abbreviate when the full term makes the name excessively long (>30 chars)
- Use spec abbreviations when they exist (CNS, ZNS, etc.)
- Be consistent — always use the same abbreviation for the same term
- Common terms (id, ctrl, ns, cmd) should always be abbreviated

## Naming Audit Checklist

### For Each Header File

- [ ] **Struct names** follow `nvme_<concept>` pattern
- [ ] **Enum names** follow `nvme_<concept>` pattern  
- [ ] **Enum values** follow `NVME_<ENUM>_<VALUE>` pattern
- [ ] **Bitfield enums** have both `_SHIFT` and `_MASK` for each field
- [ ] **Accessor macros** defined using `NVME_<STRUCT>_<FIELD>()` pattern
- [ ] **Reserved fields** named as `rsvd<offset>`
- [ ] **Abbreviations** are consistent throughout the file
- [ ] **Names match spec** terminology (or use standard abbreviations)
- [ ] **Command-set prefixes** used correctly (zns, mi, fabrics)

### Pattern Detection Queries

```bash
# Check for inconsistent struct prefixes
grep -E "^struct [^n]|^struct n[^v]|^struct nv[^m]" header.h

# Check for enum values missing prefix
grep -E "^\s+[a-z_]+\s*=" header.h

# Find bitfields missing _SHIFT or _MASK
awk '/^enum nvme_.*{/,/^}/' header.h | grep -E "SHIFT|MASK" | sort | uniq -c | awk '$1 != 2'

# Check for inconsistent reserved field naming
grep -E "rsvd|reserved|res[0-9]|rsd" header.h | grep -v "rsvd[0-9]"

# Find accessor macros not following pattern
grep "#define NVME_" header.h | grep -v "NVME_[A-Z_]*("

# Check for missing accessor macros (SHIFT/MASK without accessor)
comm -23 \
  <(grep "_SHIFT" header.h | sed 's/_SHIFT.*//' | sort) \
  <(grep "#define NVME_.*NVME_GET" header.h | sed 's/#define //;s/(.*//' | sort)
```

## Common Naming Issues and Fixes

### Issue 1: Inconsistent Prefixes

**Problem**: Some structs missing `nvme_` prefix
```c
❌ struct zns_id_ns {
❌ struct id_ctrl {
```

**Fix**: Always use full prefix
```c
✅ struct nvme_zns_id_ns {
✅ struct nvme_id_ctrl {
```

### Issue 2: Enum Values Without Proper Prefix

**Problem**: Enum values don't include enum concept
```c
enum nvme_zns_zt {
    ❌ SEQ_WRITE_REQ = 0x02,
};
```

**Fix**: Include full prefix chain
```c
enum nvme_zns_zt {
    ✅ NVME_ZNS_ZT_SEQ_WRITE_REQ = 0x02,
};
```

### Issue 3: Missing SHIFT/MASK Pairs

**Problem**: Only MASK defined, no SHIFT
```c
enum nvme_csts {
    NVME_CSTS_RDY_MASK = 0x1,
    // Missing SHIFT!
};
```

**Fix**: Always define both
```c
enum nvme_csts {
    ✅ NVME_CSTS_RDY_SHIFT    = 0,
    ✅ NVME_CSTS_RDY_MASK    = 0x1,
};
```

### Issue 4: Accessor Macro Missing

**Problem**: SHIFT/MASK defined but no accessor
```c
enum nvme_csts {
    NVME_CSTS_RDY_SHIFT = 0,
    NVME_CSTS_RDY_MASK = 0x1,
};
// No #define NVME_CSTS_RDY(csts) ...
```

**Fix**: Add accessor macro
```c
enum nvme_csts {
    NVME_CSTS_RDY_SHIFT = 0,
    NVME_CSTS_RDY_MASK = 0x1,
};

✅ #define NVME_CSTS_RDY(csts)    NVME_GET(csts, CSTS_RDY)
```

### Issue 5: Inconsistent Reserved Field Naming

**Problem**: Multiple naming styles for reserved fields
```c
❌ __u8 reserved1;
❌ __u8 rsd2;
❌ __u8 res[3];
```

**Fix**: Use `rsvd<offset>` consistently
```c
✅ __u8 rsvd41;
✅ __u8 rsvd42[3];
```

### Issue 6: Overly Verbose Names

**Problem**: Not using standard abbreviations
```c
❌ struct nvme_identify_controller_data_structure {
❌ struct nvme_submission_queue_entry {
```

**Fix**: Use standard abbreviations
```c
✅ struct nvme_id_ctrl {
✅ struct nvme_sq_entry {
```

## Automated Naming Audit Script

Here's a comprehensive audit approach:

```bash
#!/bin/bash
# nvme-naming-audit.sh - Audit NVMe header file for naming consistency

FILE="$1"

echo "=== NVMe Naming Pattern Audit: $FILE ==="
echo

echo "1. Checking struct/enum prefixes..."
BAD_PREFIX=$(grep -E "^(struct|enum) [^n]|^(struct|enum) n[^v]|^(struct|enum) nv[^m]" "$FILE")
if [ -n "$BAD_PREFIX" ]; then
    echo "❌ Found structs/enums with incorrect prefix:"
    echo "$BAD_PREFIX"
else
    echo "✅ All structs/enums have correct nvme_ prefix"
fi
echo

echo "2. Checking enum value naming..."
LOWERCASE_ENUM=$(grep -E "^\s+[a-z][a-z_]+\s*=" "$FILE" | grep -v "nvme_")
if [ -n "$LOWERCASE_ENUM" ]; then
    echo "❌ Found enum values not following NVME_* pattern:"
    echo "$LOWERCASE_ENUM"
else
    echo "✅ All enum values use uppercase NVME_* pattern"
fi
echo

echo "3. Checking SHIFT/MASK pairs..."
# Extract all *_SHIFT constants
SHIFTS=$(grep -o "[A-Z_]*_SHIFT" "$FILE" | sed 's/_SHIFT//' | sort)
# Extract all *_MASK constants  
MASKS=$(grep -o "[A-Z_]*_MASK" "$FILE" | sed 's/_MASK//' | sort)
# Find SHIFT without MASK
NO_MASK=$(comm -23 <(echo "$SHIFTS") <(echo "$MASKS"))
# Find MASK without SHIFT
NO_SHIFT=$(comm -13 <(echo "$SHIFTS") <(echo "$MASKS"))
if [ -n "$NO_MASK" ] || [ -n "$NO_SHIFT" ]; then
    echo "❌ Found incomplete SHIFT/MASK pairs:"
    [ -n "$NO_MASK" ] && echo "Missing MASK for: $NO_MASK"
    [ -n "$NO_SHIFT" ] && echo "Missing SHIFT for: $NO_SHIFT"
else
    echo "✅ All bitfields have both SHIFT and MASK"
fi
echo

echo "4. Checking accessor macros..."
BITFIELDS=$(grep -o "[A-Z_]*_SHIFT" "$FILE" | sed 's/_SHIFT//' | sort)
ACCESSORS=$(grep "#define NVME_.*NVME_GET" "$FILE" | sed 's/#define //;s/(.*//' | sort)
NO_ACCESSOR=$(comm -23 <(echo "$BITFIELDS") <(echo "$ACCESSORS"))
if [ -n "$NO_ACCESSOR" ]; then
    echo "❌ Bitfields missing accessor macros:"
    echo "$NO_ACCESSOR"
else
    echo "✅ All bitfields have accessor macros"
fi
echo

echo "5. Checking reserved field naming..."
BAD_RSVD=$(grep -iE "reserved|rsd[^v]|res[0-9]" "$FILE" | grep -v "\/\*" | grep -v "\/\/")
if [ -n "$BAD_RSVD" ]; then
    echo "⚠️  Found non-standard reserved field naming:"
    echo "$BAD_RSVD"
else
    echo "✅ All reserved fields use rsvd<offset> pattern"
fi
echo

echo "6. Summary"
echo "Check complete. Review any ❌ or ⚠️ items above."
```

## Integration with /nvme-spec audit

The naming pattern audit should be integrated into the existing `/nvme-spec audit` and `/nvme-spec verify-file` workflows:

### Enhanced audit command

```bash
/nvme-spec audit nvme-types-zns.h --check-naming
```

This should produce a section in the output:

```markdown
## Naming Pattern Compliance

### Structs (8 total)
✅ All struct names follow nvme_* pattern
✅ All use standard abbreviations consistently

### Enums (6 total)  
✅ All enum names follow nvme_* pattern
✅ All enum values use NVME_* uppercase pattern

### Bitfields
✅ 12/12 have SHIFT constant
✅ 12/12 have MASK constant
✅ 12/12 have accessor macro

### Reserved Fields
✅ All reserved fields use rsvd<offset> pattern

### Spec Terminology Matching
✅ Names closely follow spec terminology
⚠️  Consider abbreviating: nvme_zns_changed_zone_list_log → nvme_zns_changed_zone_log
```

## References

- NVMe Base Specification: Section naming conventions
- Linux kernel coding style: Naming conventions
- libnvme existing patterns in nvme-types-*.h files
- Accessor workflow documentation

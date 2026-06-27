# NVMe Type System Implementation Reference

This document provides detailed guidance for implementing NVMe specification data types in C code following the established patterns in `libnvme/src/nvme/nvme-types*.h`.

## Base Type System

The codebase uses Linux kernel-style types defined in `libnvme/src/nvme/types.h`:

**Unsigned types:**
- `__u8` - 8-bit unsigned (uint8_t)
- `__u16` - 16-bit unsigned (uint16_t)  
- `__u32` - 32-bit unsigned (uint32_t)
- `__u64` - 64-bit unsigned (uint64_t)

**Signed types:**
- `__s8` - 8-bit signed (int8_t)
- `__s16` - 16-bit signed (int16_t)
- `__s32` - 32-bit signed (int32_t)
- `__s64` - 64-bit signed (int64_t)

**Endian-aware types:**
- `__le16`, `__le32`, `__le64` - little-endian unsigned
- `__be16`, `__be32`, `__be64` - big-endian unsigned

**Type selection rules:**
- Use `__u*` for local variables and function parameters
- Use `__le*` for struct members representing little-endian wire-format data (most NVMe structures)
- Use `__be*` for struct members representing big-endian data (rare in NVMe)
- Arrays use the base type: `__u8 array[N]` not `__le8`

## Struct Implementation Pattern

When implementing data structures from the NVMe spec:

```c
/**
 * struct nvme_example - Brief description from spec
 * @field1: Description of field1 from spec (typically verbatim from spec)
 * @field2: Description of field2 from spec
 * @rsvdN: Reserved (N is the byte offset where field starts)
 */
struct nvme_example {
    __le16            field1;
    __u8            field2;
    __u8            rsvd3;
    __le32            field3;
};
```

**Struct field rules:**
1. Member names should match spec field names (lowercase, abbreviated if needed)
2. Use little-endian types (`__le*`) for multi-byte fields unless spec indicates otherwise
3. Reserved fields named `rsvdN` where N is the byte offset
4. Align field names with tabs after type declaration (Linux kernel style)
5. Document each field using kernel-doc style with `@field:` notation

## Bitfield/Enum Implementation Pattern

For register fields or bitfields packed into a single value:

```c
/**
 * enum nvme_example_field - Description of the register/field from spec
 * @NVME_EXAMPLE_SUBFIELD1_SHIFT: Shift amount to get subfield1
 * @NVME_EXAMPLE_SUBFIELD2_SHIFT: Shift amount to get subfield2
 * @NVME_EXAMPLE_SUBFIELD1_MASK: Mask to get subfield1 (after shift)
 * @NVME_EXAMPLE_SUBFIELD2_MASK: Mask to get subfield2 (after shift)
 * @NVME_EXAMPLE_VALUE1: Enumerated value definition from spec
 * @NVME_EXAMPLE_VALUE2: Another enumerated value
 */
enum nvme_example_field {
    NVME_EXAMPLE_SUBFIELD1_SHIFT    = 0,
    NVME_EXAMPLE_SUBFIELD2_SHIFT    = 8,
    NVME_EXAMPLE_SUBFIELD1_MASK    = 0xff,
    NVME_EXAMPLE_SUBFIELD2_MASK    = 0xff,
    NVME_EXAMPLE_VALUE1        = 0,
    NVME_EXAMPLE_VALUE2        = 1,
};
```

**Then provide accessor macros:**

```c
#define NVME_EXAMPLE_SUBFIELD1(val)    NVME_GET(val, EXAMPLE_SUBFIELD1)
#define NVME_EXAMPLE_SUBFIELD2(val)    NVME_GET(val, EXAMPLE_SUBFIELD2)
```

**Bitfield enum rules:**
1. All enum constants use `NVME_` prefix followed by register/field name in UPPER_CASE
2. Each bitfield **requires both** `_SHIFT` and `_MASK` constants
3. `_MASK` is the mask applied **after** shifting (not the positioned mask)
4. Enumerated values for the field follow the shift/mask definitions
5. Provide accessor macros using `NVME_GET()` for extracting values
6. For 64-bit masks, use: `static const __u64 NVME_FIELD_MASK = 0x...ull;` (cannot use enum)

## Helper Macros

The codebase provides these utility macros (defined in `nvme-types-base.h`) for working with bitfields:

- `NVME_GET(value, name)` - Extract field from complex value using shift and mask
- `NVME_SET(value, name)` - Set field into complex value  
- `NVME_CHECK(value, name, check)` - Check if field matches enumerated value
- `NVME_VAL(name)` - Get the positioned mask value (mask << shift)

Example usage:
```c
__u32 csts = read_csts_register();
if (NVME_CSTS_RDY(csts))  // expands to NVME_GET(csts, CSTS_RDY)
    printf("Controller ready\n");
```

## Type Refactoring Checklist

When refactoring types for spec compliance:

1. **Verify spec reference**: Confirm field names, sizes, offsets match the NVMe specification
2. **Check type consistency**: 
   - Multi-byte wire-format fields → `__le*` types
   - Local variables/parameters → `__u*` types
   - Arrays → base type `__u8 array[N]`
3. **Validate bitfield patterns**:
   - Each bitfield has **both** `_SHIFT` and `_MASK`
   - Accessor macros defined for all bitfields
   - 64-bit masks use `static const`, not enum
4. **Review documentation**:
   - All structs have kernel-doc comments
   - All fields documented with spec-based descriptions
   - Enum values documented
5. **Check naming consistency**:
   - All caps for enum constants: `NVME_PREFIX_FIELD`
   - Lowercase for struct members: `field_name`
   - Reserved fields: `rsvdN` where N is byte offset

## Common Type Issues to Fix

- **Inconsistent base types**: `uint8_t` instead of `__u8`, `uint16_t` instead of `__u16`
- **Missing endianness**: Using `__u16` where `__le16` is needed for wire format
- **Incomplete bitfield enums**: Missing `_SHIFT` or `_MASK` constants
- **Missing accessor macros**: Bitfields without `#define` accessors using `NVME_GET()`
- **Wrong 64-bit mask syntax**: Using enum for 64-bit mask instead of `static const __u64`
- **Inconsistent naming**: Not following `NVME_PREFIX_FIELD` uppercase pattern

## File Organization

Types are split across multiple headers in `libnvme/src/nvme/`:

- `nvme-types.h` - Main include that pulls in all type headers
- `nvme-types-base.h` - Base NVMe spec types (Identify, registers, etc.)
- `nvme-types-nvm.h` - NVM Command Set specific types
- `nvme-types-zns.h` - Zoned Namespace Command Set types
- `nvme-types-fabrics.h` - NVMe over Fabrics types
- `nvme-types-mi.h` - Management Interface types

When adding new types, place them in the appropriate file based on which spec section they come from.

**Ordering within files:**
- Register definitions: ordered by register offset (BAR0 layout)
- Data structures: ordered by figure number or logical command grouping
- Referenced types appear before types that reference them
- Each major section should have a DOC comment indicating spec section
- New definitions inserted in proper sequence, not appended at end

## Type Sizes (for offset calculations)

| C Type | Size (bytes) | Usage |
|--------|--------------|-------|
| `__u8` | 1 | Single byte, arrays |
| `__s8` | 1 | Signed single byte |
| `__u16` / `__le16` / `__be16` | 2 | 16-bit integer |
| `__u32` / `__le32` / `__be32` | 4 | 32-bit integer |
| `__u64` / `__le64` / `__be64` | 8 | 64-bit integer |
| `char[N]` | N | Character arrays (ASCII strings) |
| `__u8[N]` | N | Byte arrays |

## Common NVMe Structure Sizes

| Structure | Expected Size | Spec Reference |
|-----------|---------------|----------------|
| `struct nvme_id_ctrl` | 4096 bytes | Identify Controller Data |
| `struct nvme_id_ns` | 4096 bytes | Identify Namespace Data |
| `struct nvme_id_psd` | 32 bytes | Power State Descriptor |
| `struct nvme_lbaf` | 4 bytes | LBA Format |
| `struct nvme_smart_log` | 512 bytes | SMART / Health Log |
| `struct nvme_error_log_page` | 64 bytes | Error Log Entry |

## Maintaining Spec Order

Keeping header file definitions in the same order as the specification provides several benefits:

1. **Easier cross-referencing**: Developers can quickly find code corresponding to spec sections
2. **Easier review**: Reviewers can verify completeness by scanning in sequence
3. **Easier updates**: When new spec versions add fields, they can be inserted in proper sequence
4. **Reduced errors**: Missing fields are more obvious when order matches
5. **Better maintainability**: Future developers can navigate code matching their spec knowledge

**Best practices:**
- When adding new definitions, insert them in spec order, don't append at end
- When refactoring, take the opportunity to restore spec order if it has drifted
- Use section comments to mark major spec sections (registers, identify structs, logs, etc.)
- Document any intentional deviations from spec order with clear comments
- Include spec figure/section references in struct documentation

# NVMe Command Implementation Verification Guide

This guide covers verifying NVMe command implementations against the NVMe specification.

## Overview

NVMe commands in nvme-cli are implemented in two layers:

1. **Command structures** (`libnvme/src/nvme/types.h`, `nvme-cmds.h`) - Data structures for command DWORDs
2. **Command functions** (`libnvme/src/nvme/cmds.h`) - Functions that submit commands to the controller

This guide focuses on verifying both layers match the NVMe specification.

---

## Command Structure Locations

### libnvme Command Structures

Located in `../nvme-cli/libnvme/src/nvme/types.h`:

```c
/**
 * struct nvme_identify - Identify command structure
 * @opcode: Operation code (06h for Identify)
 * @flags: Command flags
 * @cns: Controller or Namespace Structure
 * @cntid: Controller Identifier
 * @nsid: Namespace Identifier
 * @uuid_index: UUID Index
 * ...
 */
struct nvme_identify {
    __u8  opcode;
    __u8  flags;
    __u16 command_id;
    __le32 nsid;
    __u64 rsvd2[2];
    union nvme_data_ptr dptr;
    __u8  cns;
    __u8  rsvd3;
    __le16 ctrlid;
    __u8  rsvd11[3];
    __u8  csi;
    __le32 rsvd12[4];
    __u8  uuid_index;
    __u8  rsvd13[15];
};
```

### Legacy Command Structures

Located in `../nvme-cli/nvme-cmds.h`:

These are older struct definitions that may need migration to libnvme.

---

## Command Verification Workflow

### 1. Identify the Command to Verify

Common commands to verify:
- **Admin Commands**: Identify, Get Features, Set Features, Firmware Download, etc.
- **I/O Commands**: Read, Write, Flush, Compare, etc.
- **NVM Command Set**: Dataset Management, Write Zeroes, etc.
- **Fabrics Commands**: Connect, Property Get/Set, etc.

### 2. Locate the Spec Definition

**NVMe Base Specification** (Admin and I/O commands):
- Admin commands: Section 5, Figure/Table references
- I/O commands: Section 6
- Command completion: Section 4.15

**NVM Command Set Specification**:
- NVM-specific commands
- Command set specific features

**Example locations**:
- Identify command: NVMe Base Spec, Figure 275
- Write command: NVM Command Set Spec, Figure 6

### 3. Verify Command Opcode

Check that the opcode constant matches the spec:

```c
// In types.h or command enum
enum nvme_admin_opcode {
    nvme_admin_delete_sq         = 0x00,
    nvme_admin_create_sq         = 0x01,
    nvme_admin_get_log_page      = 0x02,
    nvme_admin_delete_cq         = 0x04,
    nvme_admin_create_cq         = 0x05,
    nvme_admin_identify          = 0x06,  // ✅ Must match spec
    nvme_admin_abort_cmd         = 0x08,
    nvme_admin_set_features      = 0x09,
    nvme_admin_get_features      = 0x0a,
    // ...
};
```

**Verification**:
1. Read spec section for the command
2. Find the opcode value (typically in a table)
3. Compare with code constant

### 4. Verify Command Structure Layout

Compare the command structure DWORDs against the spec:

**Spec format** (typical command layout):
```
DW0: Opcode, Flags, Command ID
DW1: Namespace ID (NSID)
DW2-3: Reserved
DW4-5: Metadata Pointer (MPTR)
DW6-9: Data Pointer (DPTR)
DW10-15: Command specific fields
```

**Code verification checklist**:
- [ ] DW0: opcode (byte 0), flags (byte 1), command_id (bytes 2-3)
- [ ] DW1: nsid field at offset 4 (if applicable)
- [ ] DW2-3: Reserved fields properly sized
- [ ] DW4-9: Data pointer union placement
- [ ] DW10-15: Command-specific fields match spec

**Example verification**:

```c
struct nvme_identify {
    __u8  opcode;        // DW0 byte 0
    __u8  flags;         // DW0 byte 1
    __u16 command_id;    // DW0 bytes 2-3
    __le32 nsid;         // DW1
    __u64 rsvd2[2];      // DW2-3
    union nvme_data_ptr dptr;  // DW4-9 (PRP or SGL)
    __u8  cns;           // DW10 byte 0 ✅
    __u8  rsvd3;         // DW10 byte 1 ✅
    __le16 ctrlid;       // DW10 bytes 2-3 ✅
    // ... verify all DW10-15 fields
};
```

### 5. Verify Command-Specific Fields

For each command-specific field (typically DW10-15):

1. **Field name**: Should match spec terminology
2. **Field offset**: Must match DW and byte position in spec
3. **Field type**: Use appropriate `__u8`, `__le16`, `__le32`, `__le64`
4. **Field size**: Must match spec size
5. **Reserved fields**: All reserved bytes accounted for

**Common field verification**:

| Spec Field | Code Type | Notes |
|------------|-----------|-------|
| Byte field | `__u8` | Single byte |
| Word field (2 bytes) | `__le16` | Little-endian 16-bit |
| Dword field (4 bytes) | `__le32` | Little-endian 32-bit |
| Qword field (8 bytes) | `__le64` | Little-endian 64-bit |
| Reserved (N bytes) | `__u8 rsvd[N]` | Array of bytes |

### 6. Verify Command Function Signature

Check the function that submits the command matches expected parameters:

```c
/**
 * nvme_identify() - Send NVMe Identify command
 * @fd: File descriptor of nvme device
 * @cns: Controller or Namespace Structure
 * @nsid: Namespace identifier (if applicable)
 * @cntid: Controller identifier
 * @uuid_index: UUID index
 * @data: Data buffer for identify data
 *
 * Return: 0 on success, negative errno on failure
 */
int nvme_identify(int fd, enum nvme_identify_cns cns, __u32 nsid,
                  __u16 cntid, __u8 uuid_index, void *data);
```

**Verification checklist**:
- [ ] All spec-required input parameters present
- [ ] Parameter types match spec field types
- [ ] Data buffer parameter for data transfer commands
- [ ] Return type is `int` (0 success, -errno failure)
- [ ] Function documentation complete

### 7. Verify Data Transfer Direction

Commands fall into categories:

| Category | Data Direction | Example |
|----------|----------------|---------|
| Non-data | None | Flush, Abort |
| Data to host | Controller → Host | Identify, Get Log Page, Read |
| Data to controller | Host → Controller | Firmware Download, Write |
| Bidirectional | Both directions | Format NVM (rare) |

**Verification**:
```c
// Read spec to determine data direction
// Verify ioctl call uses correct direction:
// - NVME_IOCTL_ADMIN_CMD or NVME_IOCTL_IO_CMD
// - Data buffer handling matches direction
```

---

## Common Command Verification Tasks

### Verify Admin Command Opcodes

```bash
/nvme-spec verify-opcodes admin
```

Expected output:
```
Admin Command Opcode Verification
Spec: NVMe Base Spec 2.3

✅ Delete I/O Submission Queue (00h) - MATCH
✅ Create I/O Submission Queue (01h) - MATCH
✅ Get Log Page (02h) - MATCH
✅ Identify (06h) - MATCH
⚠️  Reserved opcode 03h defined in code
...
```

### Verify I/O Command Opcodes

```bash
/nvme-spec verify-opcodes io
```

### Verify Specific Command Structure

```bash
/nvme-spec verify-command identify
```

Expected output:
```
Command Structure Verification: nvme_identify
Spec: NVMe Base Spec 2.3, Figure 275

✅ Size: 64 bytes (matches spec)
✅ DW0: opcode, flags, command_id - MATCH
✅ DW1: nsid - MATCH
✅ DW2-3: reserved - MATCH
✅ DW4-9: dptr - MATCH
✅ DW10: cns, reserved, ctrlid - MATCH
✅ DW11: reserved, csi - MATCH
✅ DW14: uuid_index - MATCH
✅ All fields present and correctly positioned
```

### Audit All Commands in a File

```bash
/nvme-spec audit-commands nvme-cmds.h
```

---

## Command Implementation Patterns

### Pattern 1: Simple Admin Command (No Data Transfer)

```c
// Structure
struct nvme_abort_cmd {
    __u8  opcode;
    __u8  flags;
    __u16 command_id;
    __le32 rsvd1[9];
    __le16 sqid;      // DW10
    __le16 cid;       // DW10
    __le32 rsvd2[5];
};

// Function
int nvme_abort_cmd(int fd, __u32 nsid, __u16 sqid, __u16 cid);
```

### Pattern 2: Admin Command with Data to Host

```c
// Structure
struct nvme_identify {
    __u8  opcode;
    __u8  flags;
    __u16 command_id;
    __le32 nsid;
    __u64 rsvd2[2];
    union nvme_data_ptr dptr;
    // command-specific fields
};

// Function
int nvme_identify(int fd, enum nvme_identify_cns cns, __u32 nsid,
                  __u16 cntid, __u8 uuid_index, void *data);
```

### Pattern 3: I/O Command with Data Transfer

```c
// Structure
struct nvme_rw {
    __u8  opcode;
    __u8  flags;
    __u16 command_id;
    __le32 nsid;
    __u64 rsvd2;
    __le64 metadata;
    union nvme_data_ptr dptr;
    __le64 slba;      // Starting LBA
    __le16 length;    // Number of logical blocks
    __le16 control;
    __le32 dsmgmt;
    __le32 reftag;
    __le16 apptag;
    __le16 appmask;
};

// Function
int nvme_read(int fd, __u32 nsid, __u64 slba, __u16 nlb,
              void *data, __u32 data_len);
```

---

## Verification Checklist Template

When verifying a command, use this checklist:

### Command: `<command_name>`

**Specification Reference**:
- [ ] Spec: NVMe Base Spec / NVM CS / ZNS CS / Fabrics
- [ ] Section: X.Y.Z
- [ ] Figure/Table: N

**Opcode Verification**:
- [ ] Opcode value matches spec
- [ ] Opcode constant name follows convention
- [ ] Command type: Admin / I/O / Fabrics

**Structure Verification** (`struct nvme_<cmd>`):
- [ ] DW0: opcode, flags, command_id
- [ ] DW1: nsid (if applicable)
- [ ] DW2-3: Reserved or metadata
- [ ] DW4-9: Data pointer (dptr)
- [ ] DW10: Command-specific fields match spec
- [ ] DW11: Command-specific fields match spec
- [ ] DW12: Command-specific fields match spec
- [ ] DW13: Command-specific fields match spec
- [ ] DW14: Command-specific fields match spec
- [ ] DW15: Command-specific fields match spec
- [ ] Total size: 64 bytes

**Function Verification** (`nvme_<cmd>()`):
- [ ] All required parameters present
- [ ] Parameter types match command structure
- [ ] Data buffer parameter (if data transfer)
- [ ] Return type: `int`
- [ ] Documentation complete

**Data Transfer**:
- [ ] Direction: None / To Host / To Controller / Bidirectional
- [ ] Data size parameter (if applicable)
- [ ] Buffer alignment requirements noted

---

## Common Issues and Fixes

### Issue 1: Wrong Field Type

**Problem**: Using `__u16` instead of `__le16` for multi-byte field

```c
// ❌ Wrong
struct nvme_bad {
    __u16 field;  // Should be little-endian
};

// ✅ Correct
struct nvme_good {
    __le16 field;  // Explicitly little-endian
};
```

### Issue 2: Incorrect Field Alignment

**Problem**: Fields not aligned to spec DWORD boundaries

```c
// ❌ Wrong - missing padding
struct nvme_bad {
    __u8 byte_field;
    __le32 dword_field;  // Not DWORD aligned!
};

// ✅ Correct - with proper padding
struct nvme_good {
    __u8 byte_field;
    __u8 rsvd[3];        // Pad to DWORD boundary
    __le32 dword_field;  // Now DWORD aligned
};
```

### Issue 3: Missing Reserved Fields

**Problem**: Not accounting for all reserved bytes

```c
// ❌ Wrong - spec shows DW11-13 are reserved
struct nvme_bad {
    // ... DW10 fields
    // Missing DW11-13!
    __le32 dw14_field;
};

// ✅ Correct
struct nvme_good {
    // ... DW10 fields
    __le32 rsvd11[3];   // DW11-13 reserved
    __le32 dw14_field;  // DW14
};
```

### Issue 4: Incorrect Opcode Value

**Problem**: Opcode doesn't match spec

```c
// ❌ Wrong
enum nvme_admin_opcode {
    nvme_admin_identify = 0x05,  // Spec says 06h!
};

// ✅ Correct
enum nvme_admin_opcode {
    nvme_admin_identify = 0x06,  // Matches spec
};
```

---

## Testing Command Implementations

### Unit Test Pattern

```c
// Test command structure size
assert(sizeof(struct nvme_identify) == 64);

// Test field offsets
assert(offsetof(struct nvme_identify, opcode) == 0);
assert(offsetof(struct nvme_identify, nsid) == 4);
assert(offsetof(struct nvme_identify, cns) == 40);
```

### Integration Test Pattern

```c
// Test actual command submission
int fd = nvme_open("/dev/nvme0");
struct nvme_id_ctrl ctrl;
int ret = nvme_identify(fd, NVME_IDENTIFY_CNS_CTRL, 0, 0, 0, &ctrl);
assert(ret == 0);
assert(ctrl.vid != 0);  // Vendor ID should be non-zero
```

---

## Command Documentation Standards

Every command function should have kernel-doc style documentation:

```c
/**
 * nvme_identify() - Send NVMe Identify command
 * @fd: File descriptor of nvme device
 * @cns: Controller or Namespace Structure type to return
 * @nsid: Namespace identifier, if applicable
 * @cntid: Controller identifier
 * @uuid_index: UUID index
 * @data: Userspace buffer for identify data (4096 bytes)
 *
 * Issues an Identify command to retrieve controller or namespace
 * identification data structures as specified by the CNS field.
 *
 * Return: 0 on success, negative errno on failure.
 */
int nvme_identify(int fd, enum nvme_identify_cns cns, __u32 nsid,
                  __u16 cntid, __u8 uuid_index, void *data);
```

---

## Useful pahole Commands for Command Verification

```bash
# Check command structure size
pahole -C nvme_identify libnvme.so | grep "size:"

# Check field offsets
pahole -C nvme_identify libnvme.so

# Check for padding/holes
pahole --holes -C nvme_identify libnvme.so

# Show just the structure layout
pahole -E -C nvme_identify libnvme.so
```

---

## References

- NVMe Base Specification 2.3: Section 5 (Admin Commands), Section 6 (I/O Commands)
- NVM Command Set Specification 1.2: NVM-specific commands
- NVMe over Fabrics Specification: Fabrics-specific commands
- libnvme documentation: Command submission patterns

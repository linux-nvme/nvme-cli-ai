# libnvme Spec Coverage Tracker

Tracks, per NVMe specification document and chapter, whether libnvme has been
**gap-analyzed** against it (see [GAP-ANALYSIS-WORKFLOW.md](GAP-ANALYSIS-WORKFLOW.md))
and what was found. This is a living document — update it whenever a gap-analysis
pass covers new ground, so a future session doesn't have to rediscover what's
already been checked (or re-ask whether a command set is worth auditing).

**Status legend:**
- ✅ **Audited, implemented** — checked field-by-field against the spec; gaps found were fixed; nothing outstanding as of the date given.
- ⚠️ **Audited, gaps remain** — checked, but known gaps were deliberately left (documented below).
- 🔍 **Skimmed only** — read for shape/content but not verified field-by-field; low confidence.
- ❌ **Not implemented (confirmed)** — verified via code search that no real implementation exists (greenfield).
- ❔ **Not yet audited** — no gap-analysis pass has been done; existing code (if any) is unverified against current spec text.

Spec PDFs live in `~/specs/` (see GAP-ANALYSIS-WORKFLOW.md's "Where the actual
spec PDFs live" section for the exact search).

---

## NVM Express Base Specification, Revision 2.4

| Chapter | Status | Notes |
|---|---|---|
| 1-4 (Intro, Architecture, Registers/Properties, Base Model) | ❔ | Not chapter-audited as standalone chapters; some content (e.g. register bitfields) incidentally touched via skill's individual-verification mode over time. |
| 5 — Admin Command Set | ✅ 2026-08-21 | Full pass: opcode table (5.1), then field-level dives into Get Features (all FIDs, 17 missing constructors added), Get Log Page, Identify, Lockdown, and the whole Exported NVM Subsystem family (5.2.4/17-19, 5.4.6, previously deferred then fully implemented). See `project_chapter5_admin_gap_audit` / `plan_exported_nvm_subsystem_family` memories. |
| 6 — Fabrics Command Set | ✅ 2026-08-21 | All 6 Fabrics Command Types verified field-correct. Connect/Disconnect are implemented as connection-management functions (`libnvmf_connect*`, `libnvmf_disconnect_ctrl`) rather than simple passthru constructors — correct architectural choice, but **not bit-level verified** (different code shape than a passthru constructor). |
| 7 — I/O Commands | ✅ 2026-08-21 | Opcode table complete (8/8). Cancel command (0x18) was entirely missing a constructor — added, along with `NVME_SC_INVALID_CMD_ID` (0x84) which had no name despite the value already being reused by other command-scoped statuses. |
| 8 — Extended Capabilities | ❔ | **Not audited.** Largest remaining chapter in the base spec — covers ANA, Sanitize, Power Management, Reachability, Capacity Management, Live Migration, Exported NVM Subsystems (behavioral half), and more, organized by *feature* not by *command*. Needs a different reading strategy than chapters 5-7 (identify per-feature which structs/logs/commands are referenced, then cross-check those — not an opcode-table sweep). **Natural next target.** |
| Annexes | ❔ | Not reviewed. |

## NVM Command Set Specification, Revision 1.3

| Chapter | Status | Notes |
|---|---|---|
| 1-2 (Intro, Model) | n/a | Conceptual/overview content, not implementation-bearing. |
| 3 — I/O Commands | ✅ 2026-08-21 | Opcode-level pass: Flush, Write, Read, Write Uncorrectable, Compare, Write Zeroes, Verify, Copy, Dataset Management — all correct, no gaps. |
| 4 — Admin Command behavior | ✅ 2026-08-21 | Deep dive on user request: Performance Characteristics (Feature 1Ch) was missing its Get/Set Features constructors (fixed); FDP Statistics/Events log pages had one real struct-layout bug (`nvme_fdp_event_realloc` packing, fixed) with everything else byte-exact; Identify Namespace/Controller structures (Figures 123/127/129) had 3 small gaps (2 missing NSFEAT bits, a swapped doc comment, 2 missing decode enums — all fixed); Namespace Granularity List had a real bug (`NVME_ID_ND_DESCRIPTOR_MAX` hardcoded to 16 instead of the LBAFEE-enabled max of 64, causing a potential out-of-bounds read — fixed); Get LBA Status and Namespace Management host-specified fields verified byte-exact. See `project_nvm_command_set_gap_audit` memory. |
| 5 — Extended Capabilities | 🔍 | Skimmed only: ANA effects on NVM-Command-Set-specific Feature IDs, Get LBA Status host-implementation examples. Appears to be prose/behavioral content referencing structures already covered in chapter 4, not new data structures — but not formally verified field-by-field. |

## Zoned Namespace (ZNS) Command Set Specification, Revision 1.5

| Chapter | Status | Notes |
|---|---|---|
| 1-2 (Intro, Model) | n/a | Conceptual/overview content. |
| 3 — I/O Commands | ✅ 2026-08-21 | Opcode table (0x79/0x7A/0x7D) and all 10 ZNS-specific status codes (B6h-BFh) correct. Zone Append/Zone Management Send/Receive command encoding verified field-exact. Found and fixed missing decode enums for the Zone Descriptor's bitfields (ZT, ZS — note ZS is bits 7:4 not the whole byte, RZRTL/FZRTL) and the Zone Management Send CQE's ZCC bit. |
| 4 — Admin Commands | ✅ 2026-08-21 | Found and fixed a **major gap**: `struct nvme_zns_id_ctrl` was stuck at the Revision 1.4 shape (`zasl` + all-reserved) — missing the entire Zone Resource Management block (ZCTRATT, TAR/UAR/TOR/UOR/TZRWAR/UZRWAR, VER) added by an ECN folded into 1.5. Also fixed a missing AER notice value (0xEF, Zone Descriptor Changed) and missing decode enums for `nvme_zns_id_ns`'s ZOC/OZCS/ZRWACAP bitfields (cross-verified against nvme-cli's own print code, which already decoded them correctly). Namespace Management host-specific fields verified byte-exact (shared struct with the NVM Command Set spec). |
| 5 — Extended Capabilities | ✅ 2026-08-21 | Prose/behavioral only (Reservations, Directives, Zone Descriptor Extension, Reset/Finish Zone Recommended, Zone Active Excursions, Zone Random Write Area, Sanitize Operations) — no new data structures or command fields; nothing to implement. |
| Annex A — Host Considerations | ❔ | Informative host-usage guidance, not implementation-bearing; not reviewed. |

See `project_zns_gap_audit` memory for full detail. **`nvme-types-zns.h`'s own doc comment still says "Revision 1.4"** — worth a one-line update given this pass confirmed 1.5 content, though not done (comment-only, no functional impact).

## Key Value Command Set Specification, Revision 1.4

| Scope | Status | Notes |
|---|---|---|
| Entire spec | ❌ | Only `enum nvme_kv_opcode` exists (opcodes for Flush/Store/Retrieve/List/Delete/Exist/Reservation commands). No `nvme-types-kv.h`, no `nvme-cmds-kv.h`, no data structures, no command constructors, no plugin. Confirmed via code search 2026-08-21. This is a greenfield implementation project, not a gap-fix candidate — user explicitly chose to skip it for that reason. |

## Computational Programs Command Set Specification, Revision 1.3

| Scope | Status | Notes |
|---|---|---|
| Entire spec | ❌ | Even less than KV: no opcode enum at all. Only the CSI value (`NVME_CSI_CP`) and the "supported" bit (`NVME_IOCS_IOCSC_CPNCS`) in the generic Identify I/O Command Set Data Structure enum exist. Confirmed via code search 2026-08-21. Greenfield. |

## Subsystem Local Memory (SLM) Command Set Specification, Revision 1.3

| Scope | Status | Notes |
|---|---|---|
| Host Addressable Namespaces log page (0x85, TP4184) | ✅ 2026-08-21 | The one piece that exists (`nvme-types-slm.h`) — verified byte-exact against Revision 1.3 (MAT bits BARA/CXLA, 16-byte descriptor, 16-byte header, all correct). Header doc comment says "Revision 1.2" but content matches 1.3 too (no drift between those revisions for this specific log page). |
| Everything else (Identify data structures, SLM Read/Write/Copy/Fill commands) | ❌ | Confirmed unimplemented — the header's own doc comment says so explicitly. Greenfield. |

## Not yet touched by any gap-analysis pass

| Spec | Status | Notes |
|---|---|---|
| NVMe Management Interface (MI) Specification, Rev 2.2 | ❔ | Has substantial existing implementation (many prior commits: NVMe-MI Persistent Data Area, TDISP DEVICE_INTERFACE_REPORT, etc. — see git log), but no systematic gap-analysis pass has been run against the current spec revision. Unknown completeness. |
| NVMe-oF Specification (base) | ❔ | Not audited this pass. |
| NVMe over PCIe Transport Specification, Rev 1.4 | ❔ | Not audited. |
| NVMe over RDMA Transport Specification, Rev 1.3 | ❔ | Not audited. |
| NVMe over TCP Transport Specification, Rev 1.3 | ❔ | Not audited. |
| NVMe Boot Specification, Rev 1.4 | ❔ | Not audited. |

---

## Suggested order for future passes

1. **Base Spec chapter 8** (Extended Capabilities) — biggest known-untouched chunk of a spec that's otherwise fully audited; real gap-fix territory given the pattern seen so far (roughly one real bug + a handful of missing decode enums per chapter).
2. **NVM Command Set chapter 5** and **NVMe-MI** — smaller, bounded follow-ups to specs already mostly covered.
3. Transport specs / Boot spec — lower priority, less frequently touched by day-to-day nvme-cli work.
4. KV / Computational Programs / SLM — only if a full implementation project is explicitly requested; do not fold into a "systematic gap analysis" request.

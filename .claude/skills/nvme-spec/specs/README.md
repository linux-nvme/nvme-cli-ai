# NVMe Specification PDFs

This directory stores NVMe specification PDFs for reference during code compliance checks.

## How to Obtain Specifications

1. Visit https://nvmexpress.org/specifications/
2. Download the desired specification PDFs
3. Place them in this directory

## Naming Convention

Use descriptive names with version numbers:
- `nvme-base-spec-2.1.pdf`
- `nvme-base-spec-2.0c.pdf`
- `nvme-command-set-spec-1.0a.pdf`
- `nvme-nvm-command-set-spec-1.0c.pdf`
- `nvme-oF-spec-1.1c.pdf`
- `nvme-mi-spec-1.2a.pdf`

## Key Specifications

### Core Specifications
- **NVMe Base Specification** - Core NVMe protocol (PCIe and transports)
- **NVMe Command Set Specifications** - I/O command set definitions
  - NVM Command Set
  - Key Value Command Set
  - Zoned Namespace Command Set

### Transport Specifications
- **NVMe over Fabrics (NVMe-oF)** - RDMA, TCP, FC transports
- **NVMe Management Interface (NVMe-MI)** - Out-of-band management

## Usage

The `/nvme-spec` skill will automatically detect and use PDFs placed in this directory for compliance verification tasks.

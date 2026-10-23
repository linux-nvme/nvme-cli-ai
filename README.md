# nvme-cli-ai

Optional AI-assisted development resources for `nvme-cli` and related `linux-nvme` projects.

This repository is a companion project providing shared AI workflow resources such as prompts, project context, rules, and assistant configuration files intended to help contributors work more effectively with the `linux-nvme` ecosystem.

The goal is to keep AI tooling separate from the main source repositories while still allowing contributors to share and evolve common workflows and development practices.

## Philosophy

This repository is intentionally:
- optional
- tool-neutral
- non-invasive

Projects such as `nvme-cli` and related components should remain fully usable and buildable without any dependency on AI tooling or proprietary services.

Contributors who wish to use AI-assisted workflows can clone this repository alongside the source tree and use the provided context and configuration files with their preferred tools.

## Scope

Examples of content that may live here include:
- Claude project context (`CLAUDE.md`)
- reusable skills and slash commands (e.g. the `/nvme-spec` spec-compliance verifier)
- shared AI prompts and workflows
- coding and review guidelines
- helper scripts
- editor integrations
- documentation and knowledge organization
- experimental AI-assisted development workflows

The repository is not limited to any single AI platform and may evolve to support multiple tools over time.

## Repository Contents

| Path | Purpose |
|------|---------|
| `CLAUDE.md` | Shared project context and an nvme-cli reference guide loaded by Claude Code |
| `.claude/rules/` | Coding and workflow rules — accessor regeneration, API naming, commit messages, git workflow, license headers |
| `.claude/settings.json` | Shared permission allowlist for common build, test, and git commands |
| `.claude/skills/nvme-spec/` | The `/nvme-spec` skill — verifies C types and commands against the NVMe specification |
| `scripts/` | Helper scripts for checkpatch, fuzzing and CLI review, usable with or without an AI assistant |

## Scripts

Run the scripts from the top of an nvme-cli checkout, e.g.
`../nvme-cli-ai/scripts/checkpatch-series`. Each one prints its usage
with `-h` or without arguments.

| Script | Purpose |
|--------|---------|
| `checkpatch-series [BASE]` | Run the current `checkpatch.pl` on `BASE..HEAD` (default `upstream/master`) through `scripts/checkpatch-filter.pl`, as the CI does. The exit status is the CI verdict. |
| `fuzz-triage DIR...` | Group fuzz-device artifacts by sanitizer finding and first nvme-cli frame, and decode the command each input runs. `--show FILE` prints the stack of one artifact. |
| `fuzz-seed-check -C BUILDDIR SEED...` | Run regression seeds one by one through the test-suite driver of a sanitizer build, as `meson test` does, and print the sanitizer summary of a failure. |
| `fuzz-fetch HOST DIR` | Copy the findings of a `scripts/fuzz/nvme-fuzz run` on a remote host which `DIR` does not have yet, and check them against their SHA-1. `-s` triages them on the host instead. |
| `fuzz-replay build\|gen\|run\|cmp` | Compare what two builds print for the same generated device data, e.g. to show that a refactoring keeps the output. |
| `cli-snapshot NVME OUTDIR` | Save the command metadata and the `--help` of every command, to diff the CLI of two builds. |

The fuzz scripts need a sanitizer build:

```bash
CC=clang CXX=clang++ meson setup .build-asan \
    -Db_sanitize=address,undefined -Db_lundef=false
```

## Suggested Layout

Clone both repositories side-by-side under the same parent directory:

```text
workspace/
├── nvme-cli/      # git clone git@github.com:linux-nvme/nvme-cli.git
└── nvme-cli-ai/   # git clone git@github.com:linux-nvme/nvme-cli-ai.git
```

This allows AI tooling and workflow configuration to remain separate from the primary project repositories.

## Getting Started with Claude Code

Claude Code searches for configuration by walking **up** the directory tree from the current working directory. Because `nvme-cli-ai` is a sibling repository rather than a parent, it is not picked up automatically.

Use the `--add-dir` flag when starting a session to include this repository:

```bash
# from nvme-cli/
claude --add-dir ../nvme-cli-ai
```

Skills under `.claude/skills/` (such as the `/nvme-spec` spec-compliance verifier) are discovered automatically from an `--add-dir` directory.

The shared `CLAUDE.md` project context, however, is **not** loaded from an `--add-dir` directory by default. To load it as well, also set `CLAUDE_CODE_ADDITIONAL_DIRECTORIES_CLAUDE_MD=1`:

```bash
# from nvme-cli/
CLAUDE_CODE_ADDITIONAL_DIRECTORIES_CLAUDE_MD=1 claude --add-dir ../nvme-cli-ai
```

For convenience, add a shell alias to your `~/.bashrc` or `~/.zshrc`:

```bash
alias claude='CLAUDE_CODE_ADDITIONAL_DIRECTORIES_CLAUDE_MD=1 claude --add-dir ../nvme-cli-ai'
```

This alias applies only when `nvme-cli-ai` is present as a sibling directory. If you work on other projects, use the full `claude` command without the alias or scope the alias to your nvme-cli workspace.

## Licensing

This repository is licensed under the Apache License 2.0.

See the LICENSE file for details.

## Status

Actively maintained alongside `nvme-cli`.

Contributions, ideas, and workflow suggestions are welcome.

# Fuzzing nvme-cli

nvme-cli has libFuzzer targets in `tests/fuzz/` and a tool,
`scripts/fuzz/nvme-fuzz`, that builds, runs and evaluates them. The tool's
reference is the "Fuzzing" section of `Documentation/TESTING.md` in
nvme-cli. This file covers the workflow around it: how to run, triage and
fix findings, and how to get coverage where fuzzed data never reaches.

## The targets

- `argconfig`, `ini`, `global-config`, `fabrics-conf`: parsers.
- `device`: the most productive one. It runs nvme-cli commands in-process
  and answers every command the program sends to the device from the
  fuzzed input, through the device mock (`tests/cli/libmock_nvme.c` built
  with `MOCK_FUZZ`). Log pages, identify data and vendor logs are all
  device-controlled data, so this is where the bugs are.

A `device` input is: the command index into `commands[]` in
`tests/fuzz/fuzz-device.c`, a variant byte (json, verbose, namespace
device, argument), an argument byte, for entries with `.pci = true` the
PCI vendor and device ID (le16 each), and then one answer per command the
program sends: a control byte (0 success, >= 0xe0 NVMe status from the
next le16, >= 0xf8 EIO), the le32 completion result, then the data. Data
the input doesn't have is zero-filled. `nvme-fuzz triage` decodes the
header for you.

## Builds

```bash
scripts/fuzz/nvme-fuzz build                 # libFuzzer build in .build-fuzz
CC=clang CXX=clang++ meson setup .build-asan \
    -Db_sanitize=address,undefined -Db_lundef=false   # suites under ASan/UBSan
```

- Sanitizer and fuzz builds need C and C++ from the same toolchain;
  `meson setup` aborts with clang and g++. Set `CXX=clang++`.
- `nvme-fuzz build` uses `debugoptimized`, so reports name source lines.
- libFuzzer `-fork` crashes in the parent with clang 22 (Fedora 44);
  nvme-fuzz uses `-jobs`/`-workers`, don't switch it to fork mode there.
- The test suite runs every target briefly (`meson test --suite fuzz`,
  `FUZZ_SECONDS`, `FUZZ_SEED`) with a plain driver. It loads the seeds in
  readdir order, at most 256 of them and 16 KiB each, so CI mutations
  differ from yours. When hunting a CI-only fuzz failure, try many
  `FUZZ_SEED` values.

## Running

```bash
scripts/fuzz/nvme-fuzz run --targets device --seconds 3600 --jobs 8
```

- Each job of the device target uses about 1 GiB RSS. Leave memory for
  builds next to it; on a laptop use a handful of jobs.
- The device target runs in stages with growing device data (16K, 64K,
  256K, 1M). Most structures fit a page and small data runs fast. Coverage
  saturates in about 15 minutes per stage, but bugs still turn up hours in.
  For real hunting, run overnight or over a weekend.
- Every stage starts from a corpus minimized for its data limit, merged per
  command in parallel. The corpus in `--results` grows across runs; keep
  the results directory outside the build directory, because
  `scripts/build.sh fuzz` removes the build directory, results included.
- Before a run, `nvme-fuzz run` adds seeds from the mock tests (see
  Coverage).
- On a remote box, use `scripts/fuzz-box` (this repo, see below). Output
  is block-buffered; look at `summary.txt` and the job logs instead.
- An input that crashes almost every run kills throughput: each finding
  ends a job and the new job reloads the corpus. Fix trivial crashers
  first.

## A remote fuzz box

Long runs go to a big box. `scripts/fuzz-box` (this repo) drives them
over ssh from the local nvme-cli checkout; the box needs an nvme-cli
clone, clang with libFuzzer, meson and python3. The box runs
`scripts/fuzz-box-run`, which waits for the run before, updates the tree
to the upstream master, applies the patch queue, builds and fuzzes until
an end time.

```bash
box() { ../nvme-cli-ai/scripts/fuzz-box -s "-i $HOME/.ssh/box" "$@"; }
box queue root@box p1 upstream/master..fix-foo  # queue the open fixes
box start root@box "2026-10-27 08:00"          # the box's time zone
box status root@box                            # progress, findings
box stop root@box
box prune root@box                             # drop merged patches
```

- Queue every open fix, also one which can't trigger on the box, so the
  run doesn't re-find fixed bugs or report against code that is about to
  change. `queue` and `verify` apply the whole queue to the upstream master
  in a scratch worktree; `prune` moves the patches which are upstream to
  `QUEUE.merged`. A patch the box can't apply is skipped, see its log.
- `start` while a run is going waits for it, so the next run is queued.
  Each run uses the master of the moment it starts.
- Triage a run and look at its `coverage` before the next run starts:
  `fuzz-box-run` moves the tree to the new master, and inputs select
  commands by index. Later, replay them only on a tree with the same
  `tests/fuzz/fuzz-device.c`. `fuzz-fetch -n SRC -r RESULTS` copies the
  findings with the same paths as `fuzz-box -n SRC -r RESULTS`.
- `-P DIR` puts DIR first in PATH on the box, e.g. a directory with a
  `python3` link to a newer Python than the system one (meson's tests
  pick `python3`).
- Don't start remote commands as part of a list in the background
  (`a && setsid nohup b &`): the subshell keeps the ssh connection open
  until the run ends. Don't use `pkill -f` over ssh: the pattern also
  matches the remote shell running your command; `fuzz-box stop` kills
  by PID.

## Before adding commands to the device table

- Append only. An input selects a command by its index, so existing
  indices must never change meaning (the regression inputs depend on it).
  To give an existing command PCI IDs, add a new entry instead of setting
  `.pci` on the old one.
- No commands that write files or change the device. Read-and-print
  commands only.
- Run `nvme-fuzz sweep --entries N,M` on the new entries: it reports
  crashes, slow runs and files written within seconds, before a fuzz run
  stumbles over them.
- `.pci = true` entries get a fake sysfs with the input's PCI IDs plus
  fixed link, NUMA and AER attributes. Give the plugin model IDs in
  `scripts/fuzz/nvme_fuzz/device.py` (`PCI_IDS`, one per capability group
  of its model table); the unit tests require them.
- Commit order for bisect: fixes for what the new entries find go first,
  then the table change.

## Triage

```bash
scripts/fuzz/nvme-fuzz triage RESULTS/artifacts/device       # group by bug
scripts/fuzz/nvme-fuzz triage --show FILE                     # one stack
scripts/fuzz/nvme-fuzz check FILE...                          # replay once
```

- `triage` groups by kind and first nvme-cli frame, and names the command.
  Findings named `data-<limit>-crash-*` are replayed with that data limit.
- A long run finds the same bug many times, through different commands.
  Replay the findings against a build with your fixes and only look at
  what still fails.
- Note which commit the run fuzzed. Inputs select commands by index, so
  replay on another tree only if `tests/fuzz/fuzz-device.c` hasn't changed
  between them.
- Timeouts are mostly big but valid logs under ASan. OOMs are device-sized
  allocations (the target caps allocations at 1 GiB; ASan returns NULL
  instead of aborting). Neither is a bug by itself.
- `scripts/fuzz-fetch` (this repo) copies new findings from a remote box
  and verifies them; `-s` triages them there.

## Fixing a finding

- Bound every count, length and index that comes from the device by the
  buffer it indexes. Read unaligned fields with the unaligned little-endian
  helpers (`shared/unaligned-util.h`), not pointer casts. Avoid C bitfields
  for device registers; use masks. Shifts by device values (LBADS) go
  through `nvme_lbaf_data_size()`.
- Never change the JSON output for valid data (keys, types, structure).
- Add the finding's input as a regression seed,
  `tests/fuzz/corpus/device/regress-<what>`. Seeds run with the default
  data limit and are capped at 16 KiB, so check that yours still triggers:
  `nvme-fuzz check` must fail without the fix and pass with it.
- Failing-first means a real build of the old code. After reverting the
  fix, make sure the build succeeded: a `-Werror` failure (e.g. a variable
  your revert left unused) leaves the old binary, and the test "passes".
- If the code isn't in the fuzz table, write a mock test under
  `tests/cli/` instead (see `nvme_mock_ipc.py`); it must fail without the
  fix. Run new mock tests in an ASan/UBSan build too: UB that the plain
  build hides shows up there.
- Fixes go in their own PR off master, one bug per commit. Generate the
  `Fixes:` tag with `git log -1 --abbrev=12 --format='%h ("%s")' <rev>`;
  don't type hashes. `git blame` often points at a refactor; follow
  `git log -S'<line>'` to the commit that introduced the code.

## Coverage

```bash
scripts/fuzz/nvme-fuzz coverage                  # per command, random data
scripts/fuzz/nvme-fuzz coverage --corpus DIR     # per command, a corpus
```

`coverage` shows how many edges each command reaches beyond a command that
only identifies the controller. A low number means the command stops at a
check the fuzzer can't pass: a GUID, a version, a supported-log check, a
model table.

- **Seeds from the mock tests** are the cheapest fix. `nvme-fuzz seeds`
  runs the mock suite with `NVME_FUZZ_RECORD` set and turns each run of a
  table command into a seed with valid answers. A mock test that walks a
  plugin through realistic device data (e.g. `tests/cli/wdc/`) lifts the
  coverage of all its commands at once: the wdc commands went from about
  170 to 900-1960 edges.
- **PCI model checks**: a plugin must read PCI IDs with
  `nvme_get_pci_ids()` and sysfs attributes via `nvme_sysfs_ctrl_path()`.
  Both honour the test sysfs root of `--set-options test-sysfs-dir`;
  absolute `/sys` paths are invisible to tests. Mock tests fake the IDs by
  writing `sys/class/nvme/nvme0/device/{vendor,device,...}` (plus
  `sys/class/nvme-subsystem`) in their sysfs directory.
- **Mocking a vendor device**: find what each command checks by running it
  against a mock server that answers with zeros, and adding what it
  complains about. Get struct offsets with `pahole -C <struct>
  <object file>` from a debug build instead of counting by hand. OCP-style
  log pages have the version at `size - 18` and the GUID at `size - 16`.
  Logs that share an ID are told apart by the UUID index in CDW14.
- Measure before building infrastructure. Compare coverage or exec/s
  before and after; e.g. LSan rechecks turned out to cost at most 7% and
  weren't worth fixing.

## CI gotchas

- The cross matrix is fail-fast: one real failure shows the other archs as
  failed too, but their logs end in "The operation was canceled". Find the
  job that really failed.
- `gh run view --log-failed` can come back empty; use
  `gh api --allow-escape-sequences repos/<owner>/<repo>/actions/jobs/<id>/logs`.
  The "log files" artifact has the full test log with the assertion.
- Reproduce cross failures with the CI image
  (`ghcr.io/linux-nvme/ubuntu-cross-<arch>`, `scripts/build.sh -b release
  -c gcc -t <arch> cross`). s390x is big-endian and catches byte-order and
  bitfield bugs.
- CI also runs the Python unit tests under valgrind. A blocking wait that
  relies on a signal to interrupt it hangs there and on FreeBSD; wait with
  timeouts.

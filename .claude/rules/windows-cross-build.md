# Windows cross build from Linux

nvme-cli builds on Windows (MSYS2). CI has two Windows jobs in
`.github/workflows/build.yml`:

- `windows-msys2-ucrt64`: x86_64, gcc
- `windows-msys2-clangarm64`: ARM64, clang

Both run `scripts/build.sh`, which configures with `--werror`, so **any
compiler warning breaks Windows CI**. Before you push changes that touch
portable code (shared/, libnvme, plugins enabled on Windows), check them
locally with a cross build. You don't need Windows, wine, containers or
root.

## One-time setup: llvm-mingw

[llvm-mingw](https://github.com/mstorsjo/llvm-mingw) is a self-contained
clang/lld toolchain that targets Windows. One toolchain covers x86_64 and
aarch64. Unpack it anywhere in your home directory:

```bash
mkdir -p ~/.local/opt && cd ~/.local/opt
# pick the newest *-ucrt-ubuntu-*-x86_64.tar.xz from the releases page
curl -LO https://github.com/mstorsjo/llvm-mingw/releases/download/<tag>/llvm-mingw-<tag>-ucrt-ubuntu-22.04-x86_64.tar.xz
tar xf llvm-mingw-<tag>-ucrt-ubuntu-22.04-x86_64.tar.xz
ln -sfn llvm-mingw-<tag>-ucrt-ubuntu-22.04-x86_64 llvm-mingw
```

To upgrade later, unpack a newer release and point the `llvm-mingw` symlink
at it. The cross files don't need to change.

## Meson cross files

Meson looks up cross files by name in `~/.local/share/meson/cross/`. Create
`~/.local/share/meson/cross/mingw-w64`:

```ini
[constants]
# meson does not expand ~ or $HOME; use the absolute path
toolchain = '/home/<you>/.local/opt/llvm-mingw/bin/'
triple = 'x86_64-w64-mingw32-'

[binaries]
c = toolchain + triple + 'clang'
cpp = toolchain + triple + 'clang++'
ar = toolchain + 'llvm-ar'
strip = toolchain + 'llvm-strip'
windres = toolchain + triple + 'windres'
pkg-config = 'false'

[host_machine]
system = 'windows'
cpu_family = 'x86_64'
cpu = 'x86_64'
endian = 'little'
```

For ARM64, copy the file to `mingw-w64-aarch64` and set
`triple = 'aarch64-w64-mingw32-'` and `cpu_family`/`cpu = 'aarch64'`.

`pkg-config = 'false'` stops meson from picking up host (Linux) libraries.

## Building

Run from `../nvme-cli/`, using a build directory outside the source tree:

```bash
meson setup /tmp/win-build --cross-file mingw-w64 -Djson-c=disabled
ninja -C /tmp/win-build 2>&1 | grep -E "warning:|error:|FAILED"

# ARM64
meson setup /tmp/win-arm-build --cross-file mingw-w64-aarch64 -Djson-c=disabled
ninja -C /tmp/win-arm-build
```

- **json-c:** `-Djson-c=disabled` is needed because there's no Windows
  json-c here. Plugins that require json-c are skipped, but CI builds them.
- **Plugin list:** the "Plugins" section of the `meson setup` summary shows
  which plugins are enabled for Windows. Gating lives in
  `plugins/<name>/meson.build`.
- **Warnings:** treat every warning as an error, just as CI does. A common
  one is UCRT's deprecation of `timezone`/`_timezone`; use
  `shr_tm_gmtoff()` from `shared/time-util.h` instead.
- **Compile-only:** the binaries can't run, so `meson test` fails. Use
  `file /tmp/win-build/nvme.exe` to confirm a PE32+ binary was produced.
- **Different compiler:** CI's x86_64 job uses gcc rather than clang, so it
  can very occasionally warn about something clang accepts.

## Checking every commit of a series

Each commit must build on its own (see `git-workflow.md`):

```bash
for c in $(git rev-list --reverse master..HEAD); do
	git worktree add -q --detach /tmp/wt-win $c
	(meson setup /tmp/wt-win/.build-win /tmp/wt-win \
		--cross-file mingw-w64 -Djson-c=disabled >/dev/null &&
	 ninja -C /tmp/wt-win/.build-win >/dev/null) || echo "FAIL: $c"
	git worktree remove --force /tmp/wt-win
done
```

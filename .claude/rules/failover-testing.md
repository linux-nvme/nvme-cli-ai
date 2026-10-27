# NVMe-oF failover testing

Changes to nvme-discoverd, to the fabrics code of nvme-cli and libnvme, or
to the kernel's NVMe-oF transports need tests that break paths on a real
host. [nvmf-failover](https://github.com/igaw/nvmf-failover) does that and
checks the recovery; its README is the reference, the "NVMe-oF failover
tests" section of `Documentation/TESTING.md` in nvme-cli the overview. This
file covers how to run it from a session, and what went wrong before.

## Setup on the test host

- Clone the tool on the test host (`git clone`, `pip install '.[ontap]'`)
  or copy a checkout over with `tar` (not `scp`, it flattens directories).
  The scenarios live in the checkout, pip doesn't install them.
- The configuration (`~/.config/nvmf-failover/config.toml`, mode 0600) holds
  the passwords of the arrays and switches. The user writes it. Never put a
  password on a command line, also not in an environment variable of an ssh
  call: read it through the tool, which loads the file.
- Before starting anything remote that uses new or changed files, copy them
  over and check them there (`--help`, `plan`). A night run once aborted at
  once because the scenario existed only locally.

## Running

- `status`, then `discover`, then `plan SCENARIO` before every `run`. `plan`
  prints the rounds, the faults and the paths each one hits on this host;
  read it. Never set `allow_shared` (a switch port other hosts use too) or
  `allow_outage` without asking: lab switches and arrays are shared.
- `--io all` reads from every namespace of the selected subsystems.
  `--io-verify` destroys the data on its device: only on a namespace or LUN
  without a filesystem or partition table, named by `/dev/disk/by-id` (the
  `/dev/nvmeXnY` names change across boots), and only after the user agreed.
- Start a long run detached, as a statement of its own:
  `cd DIR; setsid nohup nvmf-failover run ... > LOG 2>&1 < /dev/null &`.
  `a && b && setsid nohup c &` over ssh backgrounds the whole list, and the
  ssh call hangs until the run ends.
- The run restores every fault it applied, also on Ctrl-C and SIGTERM: stop
  a run with `kill -TERM`, never `kill -9`. `pgrep -f`/`pkill -f` over ssh
  also match the ssh call's own shell; find the PID with
  `ps -eo pid=,args= | grep "[n]vmf_failover"`.
- After restarting nvme-discoverd, wait a minute or two before a run: it
  restarts the units of the controllers it adopts, and a fault during that
  time looks like a reconnect failure.

## What the timers mean

- On FC, `dev_loss_tmo` of the remote port decides: a path down for less is
  reconnected by the kernel, the controller stays (`reconnected_by =
  "kernel"`); a path down for longer loses its controller, and only
  nvme-discoverd can connect it again (`reconnected_by = "discoverd"`).
  Test both for a change to nvme-discoverd.
- nvme-discoverd disconnects a discovery controller whose EPCSD is 0 and
  reconnects it every `epcsd-poll-interval-minutes` to poll its log page.
  Discovery controllers coming and going are no failover problem; the
  verdict counts I/O controllers only.
- A controller that is reconnecting shows `(efault)` as its subsystem NQN
  in sysfs; the tool reports it as not identified yet.

## Reading the results

- `summary.json` has the verdict; `events.log` the timeline: the steps, every
  controller and ANA state change, and the nvme-discoverd journal.
- Per round, check which I/O controllers were deleted and when each came back
  after the port came up. A controller the kernel deleted and nvme-discoverd
  didn't log about ("removed but still desired, reconnecting") points at a
  silent branch in nvme-discoverd: two such bugs were fixed in nvme-cli
  PR #4326. Add `log_info()` to the silent returns of `on_nvme_remove()` and
  `on_ioc_add()` in a test build to find the branch.
- `debug-level = debug` in `nvme-discoverd.conf` helps for short runs; for a
  night it can hit journald's rate limit and lose lines. Use `info`.
- "connection unit failed" for target ports whose link is down on the array
  side is the expected retry, not a failover problem.

## Before blaming nvme-discoverd

- The legacy udev autoconnect must be off: `70-nvmf-autoconnect.rules`
  masked, `nvmf-autoconnect.service` and `nvmefc-boot-connections.service`
  disabled, and not in the initrd (`lsinitrd | grep nvmf`; a dracut config
  left from an earlier install keeps pulling the rule in). Otherwise both
  connect at once (nvme-cli issue #4327).
- Files left from an earlier package or source install (`rpm -qf`, owned
  by nothing) survive an install of a new build: check the units, rules and
  dracut configs actually in use.
- `meson install` of nvme-cli needs `--prefix=/usr -Dsysconfdir=/etc` to use
  `/etc/nvme`: with the relative default, `--prefix=/usr` gives `/usr/etc`.

# USB 2.0 ASIX Ethernet (experimental)

The built-in driver is selected automatically during network initialization.
Recognized USB 2.0 ASIX adapters use **DHCP by default**: the router supplies
the IP, subnet mask, gateway and DNS, without an IP file or menu toggle. The
normal IOS path remains for adapters the custom driver does not recognize.
Starting without online play leaves the ASIX driver inactive.

## Install (router connection)

Install this build over the existing SlippWii app, keep its online data and
private user.json account file, connect the Ethernet adapter to a router with
DHCP enabled, and launch SlippWii. No separate ASIX library, firmware, IOS
installation or Melee ISO patch is needed.

**If an older slippi_asix.ini already exists, rename or remove it to use DHCP.**
It is at the root of the device holding Melee (SD:/ for an SD game, USB:/ for a
USB game). Existing static files deliberately keep their static behavior.
No file means DHCP; optionally, a file containing only dhcp=1 also selects DHCP.
The included example uses DHCP and can be copied without editing router values.

Address acquisition waits up to 30 seconds after the Ethernet link is ready,
including retries and address-conflict checks. If no lease arrives and no
fallback is configured, startup shows “No router IP. Check Ethernet/router; restart SlippWii.” Without an explicit fallback, it does not
silently assign a guessed static address. DHCP does not fix a missing Ethernet
link, unsupported hardware, disabled router DHCP or internet/NAT restrictions.

Renewal and rebinding continue while Melee runs. Router-supplied DNS is used for
matchmaking; the lease's IP is also used by local discovery and online play.
If a lease expires, the old IP is cleared while the client tries again. An
ongoing match may disconnect if the address changes or the router disappears.
A router that supplies no DNS can support numeric-IP LAN tests but needs its
DHCP/DNS configuration fixed for normal hostname-based matchmaking.

## Optional static address

Advanced setups can keep the original format, using an unused IP in the actual
subnet, for example:

~~~ini
ip=192.168.1.250
netmask=255.255.255.0
gateway=192.168.1.1
dns=192.168.1.1
~~~

DNS defaults to gateway if omitted. A static file can include dhcp=0.
With dhcp=1, static address/DNS fields are accepted only when fallback=1 is
explicitly configured. Invalid or unreadable files fail startup.

## DHCP first with a static fallback

For a laptop sharing Ethernet at 192.168.137.1, this opt-in setup tries DHCP
for up to 30 seconds, then uses the previous working static connection:

~~~ini
dhcp=1
fallback=1
ip=192.168.137.250
netmask=255.255.255.0
gateway=192.168.137.1
dns=192.168.137.1
~~~

The file belongs at the root of the device holding Melee, as above. A copy is
provided as Nintendont/kernel/asix/slippi_asix.fallback.ini.example. These
addresses are specific to that laptop subnet; use valid, unused addresses for
other networks. Ordinary router users can keep the default DHCP-only setup.

Successful DHCP takes precedence over all fallback values, including DNS.
Fallback is used only when initial DHCP setup fails and USB remains healthy;
an adapter failure is still reported. DHCP is stopped before the static values
are applied, so a late reply cannot switch addresses during a match. There is
no background DHCP retry after fallback. Lease expiry after an initially
successful DHCP connection retains normal DHCP recovery instead of falling back.

The boot log reports address_mode=static-fallback, dhcp_result=-106 and the
assigned dotted IP when selected. Successful DHCP reports address_mode=dhcp.
To skip DHCP entirely, use dhcp=0 and fallback=0 with the same static fields.


Native IOS networking still uses the Wii Settings profile; the custom driver
uses DHCP or the optional file above.

## Coverage

| Chip family | Driver path | Physical Wii validation |
| --- | --- | --- |
| AX88172 | Raw Ethernet, board-specific GPIO reset | Needed |
| AX88172A | Legacy framing, internal/external PHY selection | Needed |
| AX88772 / AX88772A | Legacy framing, chipcode-specific reset | Needed |
| AX88760 Ethernet function | AX88772A-compatible identity/protocol | Needed |
| AX88772B / AX88772C | Legacy header mode, software checksums | Needed |
| AX88178 | Legacy framing, Marvell or RTL8211CL external PHY | Needed |
| AX88178A | USB 2.0 gigabit register/framing path | Needed |
| AX88772D / AX88772E | Existing revision-0300 register/framing path | One revision-0300 adapter verified; D vs E is not distinguishable by these IDs |

Detection uses the explicit IDs in [device_list.h](../Nintendont/kernel/asix/device_list.h),
including known legacy OEM IDs, and validates the USB device descriptor.
AX88772D/E requires **0b95:1790, bcdDevice 0300**. AX88178A requires
**0b95:178a, bcdDevice 0100**. The legacy AX88772 chipcode selects original,
A or B/C initialization even when the advertised model name differs.
Bulk endpoints are discovered from one interface/alternate setting rather than
assuming all adapters have the same endpoints.

USB 3.0 chipsets (AX88179/A/B and AX88279 variants) are excluded, including when
connected at USB 2.0 speed. Unknown OEM IDs are left to IOS rather than guessed.
Special MII/RMII bridge configurations, custom external-PHY boards, and AX88178
PHY modes other than Marvell/RTL8211CL are not implemented. AX88772B/C currently
requires the standard internal Ethernet MAC mode. Gigabit families negotiate
10/100/1000 links, but this does not make the Wii's USB bus run at gigabit speed.


All new families remain physically unverified. The revision-0300 adapter was
verified in the related Slippi Nintendont mirroring build; this SlippWii port
still requires hardware validation. Hotplug and runtime link renegotiation are
not implemented.

## Transport and diagnostics

The hardware implementation is ported from ryanclanigan/Nintendont commit
098feedb10f08ca44edc7453469b7f39d26c8cfb. It shares one serialized NO_SYS lwIP
2.2.1 instance between mirroring and online play. SlippWii's receive and sender
threads use a separate general UDP API with bounded queues and descriptor
generations. Matchmaking and netplay retain the same session port; the log sink
uses a separate socket. Native IOS behavior is preserved when ASIX is inactive.
DNS uses lwIP rather than asking IOS to resolve on a different network interface.
IPv4 fragmentation/reassembly supports SlippWii's existing 1536-byte datagram limit.

Initialization writes slippi_asix_boot.log and initial raw USB captures
slippi_asix_rx.bin / slippi_asix_tx.bin to the game device root. The log contains
family, revision, endpoints, negotiated link, address mode, DHCP result/state, DNS
and I/O results. It does not include
user.json or account credentials. Empty captures are normal before traffic.

USB buffers, the mirror receive buffer, and the lwIP heap are allocated at
runtime because SlippWii's fixed kernel data region is almost full. The kernel
linker regions and thread stack locations are unchanged. All USB/raw-stack
operations run under one mutex; receive waiters release it while sleeping.

## Build and checks

The driver and lwIP sources are vendored and built into boot.dol; no additional
SDK is needed. See [BUILDING.md](BUILDING.md). lwIP's BSD license is in
Nintendont/kernel/asix/lwip/COPYING. Hardware source attribution cites Linux
v6.18, ASIX driver 42feb1252fe0669845a8d6714bcc77bfc19eea1d and legacy driver
e87b8576e6fa3accd373cff326850ab0a216df36, as in the original port.

~~~sh
cd Nintendont
sh kernel/asix/tests/run.sh
~~~

On Windows: Nintendont/kernel/asix/tests/run.ps1 -Compiler gcc (TinyCC users
also provide -CompilerInclude). Tests cover hardware initialization, packet
formats, native fallback, TCP mirroring, general UDP socket isolation, overflow,
fragmentation/reassembly, numeric addresses and DNS success/failure. DHCP tests cover loss/retry, wrong transaction IDs, NAK
recovery, renewal, rebinding, expiry and startup timeout. Fallback tests verify
static addressing/DNS, a mirroring TCP handshake, SlippWii UDP and DNS after
timeout, DHCP precedence, stopped lease timers and USB failures. Existing
slippi_online unit tests and loader settings tests should also pass. Host tests
do not establish USB latency or online game performance on a Wii.

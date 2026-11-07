# USB 2.0 ASIX Ethernet (experimental)

The built-in driver is selected automatically during network initialization.
There is no additional menu toggle, ISO patch, firmware download or separate
ASIX library to install. SlippWii keeps its normal IOS network path for
unrecognized adapters, and for recognized legacy adapters when the static
configuration file is absent. A recognized adapter with an invalid file reports
an error. Starting without online play leaves the ASIX driver inactive.

## Install

Install a SlippWii build containing this change over the existing app folder.
Keep the existing slippwii data folder and private user.json account file.
Copy Nintendont/kernel/asix/slippi_asix.ini.example to the **root of the device
holding Melee**, renamed **slippi_asix.ini** (SD:/ for an SD game, USB:/ for a
USB game). Release packages include this example without activating it.

For a laptop sharing internet on 192.168.137.1:

~~~ini
ip=192.168.137.250
netmask=255.255.255.0
gateway=192.168.137.1
dns=192.168.137.1
~~~

Use an unused IP in the actual Ethernet subnet. For online play, the gateway
must route to the internet and the DNS server must answer queries. DNS is
optional in the file and defaults to gateway; set it explicitly when your
router does not provide DNS. With gateway and DNS both omitted, only local
numeric-IP communication works. The custom stack uses static addressing;
DHCP is not implemented. These values replace the Wii Settings network profile
for the custom path; IOS networking still uses the normal profile.

Connect the adapter/cable before launching SlippWii. First ping the configured
IP while Melee is running, then try Direct matchmaking with a known opponent.
Internet sharing, NAT and firewall rules must allow SlippWii's normal UDP
traffic. No online match over the custom path has yet been verified on a Wii.

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

If no supported adapter is detected, SlippWii uses existing IOS networking.
Legacy adapters also keep that path when **slippi_asix.ini is absent**, preserving
normal Wii LAN-adapter setups. Providing a valid file selects the custom driver
for a recognized legacy adapter. An invalid file is reported as an error.
AX88772D/E and AX88178A require the file. Native fallback does not imply that IOS
supports an otherwise unrecognized adapter.


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
family, revision, endpoints, negotiated link and I/O results. It does not include
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
fragmentation/reassembly, numeric addresses and DNS success/failure. Existing
slippi_online unit tests and loader settings tests should also pass. Host tests
do not establish USB latency or online game performance on a Wii.

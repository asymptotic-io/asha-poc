# Notes

- Interesting packets start at around packet 70
- Recorded using wireshark on Fedora 34
- Connected to the RSL10 using this asha-support package and attempted to write some audio to it
- Note that we are writing data synchronously


# Debugger session output

```
loop: Initializing
Connecting to 60_C0_BF_2F_6E_A0
Connected to 60_C0_BF_2F_6E_A0
Device: /org/bluez/hci0/dev_60_C0_BF_2F_6E_A0. /org/bluez/hci0/dev_60_C0_BF_2F_6E_A0/service0021
Size: 2
Sent
Size: 17
Data (uint8): 1 0 98 3 34 51 68 85 102 119 1 0 0 0 0 2 0 
Data (hex): 1 0 62 3 22 33 44 55 66 77 1 0 0 0 0 2 0 
Version: 1
Device Capabilities(side): 0
Device Capabilities(type): 0
HiSync ID: 144115188075951974
Feature map: 0
Render delay: 0
Reserved: 0
Supported Codecs: 0
Sent
Done.
Writing to /org/bluez/hci0/dev_60_C0_BF_2F_6E_A0/service0021/char0025
AudioControlPoint Written
L2CAP: Created socket 60_C0_BF_2F_6E_A0:168

Breakpoint 1, setopts (s=5) at ../src/bluez.c:12
12	 struct l2cap_options opts = {0};
Missing separate debuginfos, use: dnf debuginfo-install bluez-libs-5.62-1.fc34.x86_64
(gdb) c
Continuing.
Setting sockopts
Set sockopts
L2CAP: Connected to 60:C0:BF:2F:6E:A0:168

Breakpoint 3, stream_init (bd_addr_raw=0x7fffffffdd60 "60_C0_BF_2F_6E_A0", device=0x40bd90) at ../src/streaming.c:30
warning: Source file is more recent than executable.
30	 int audio_src = open("/tmp/sample.g722", O_RDONLY);
(gdb) c
Continuing.

Breakpoint 2, stream_init (bd_addr_raw=0x7fffffffdd60 "60_C0_BF_2F_6E_A0", device=0x40bd90) at ../src/streaming.c:34
34	   if (i == 0) {
(gdb) c
Continuing.

Breakpoint 2, stream_init (bd_addr_raw=0x7fffffffdd60 "60_C0_BF_2F_6E_A0", device=0x40bd90) at ../src/streaming.c:34
34	   if (i == 0) {
(gdb) print i
$1 = 1 '\001'
```

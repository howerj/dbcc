# can.md

Linux only test bench for dbcc

Using:
* <https://github.com/linux-can/can-utils/>

And socket CAN:
* <https://www.kernel.org/doc/Documentation/networking/can.txt>

## Sending values

Format is:
	cansend can0 FFF#11.22.33.44.55.66.77.88

Or:
	HH = Hexadecimal value

	cansend *device* *hex-id*#HH.HH.HH.HH.HH.HH.HH.HH

## Filtering and candump

Only show messages with ID 0x123 or ID 0x456:

	candump vcan0,0x123:0x7FF,0x456:0x7FF

# stats:

	cat /proc/net/can/version
	cat /proc/net/can/stats

# UDP Sender
## Description
A C program I made to send UDP packets to a specified destination IP and port via raw sockets. This program supports the following:

* `pthreads` (multi-threading).
* Specifying a source IP (including spoofing the IP).
* Minimum and maximum payload length (randomized for each packet).
* Randomized source ports (from 10000 to 60000).
* Wait times (intervals) between sending packets on each thread.
* Calculates both the IP Header and UDP Header checksums.

**Note** - This program does not support packet fragmentation and there's really no point to add that support since that's not what the program is made for.

## Why Did I Make This?
I am currently learning more about in-depth packet inspection along with learning how (D)DoS attacks work. I made this program and only use it on my local network. I am planning to create a UDP server application that is capable of filtering (D)DoS attacks and blocking them using [XDP](https://en.wikipedia.org/wiki/Express_Data_Path) once detected. This tool will be used to test this server application I will be making. Eventually I will be making software that'll run on my Anycast network that will be capable of dropping detected (D)DoS attacks via XDP on all POP servers.

## Compiling
I used GCC to compile this program. You must add `-lpthread` at the end of the command when compiling via GCC.

Here's an example:

```
gcc -g UDP_Sender.c -o UDP_Sender -lpthread
```

## Usage
Usage is as follows:

```
Usage: ./UDP_Sender <Source IP> <Destination IP> <Destination IP> [<Max> <Min> <Interval> <Thread Count>]
```

Please note that the interval is in *microseconds*. The Min and Max payloads are in *bytes*. If you set the interval to 0, it will not wait between sending packets on each thread.

Here's an example:

```
./UDP_Sender 192.168.80.10 10.50.0.4 27015 1000 1200 1000 3
```

The above continuously sends packets to `10.50.0.4` (port `27015`) and appears from `192.168.80.10` (in my case, spoofed). It sends anywhere from `1000` - `1200` bytes of payload data every `1000` microseconds. It will send these packets from `3` threads.

## To Do List
* Pick source IPs from a configuration file on disk and randomize it for each packet.

## Experiments
I was able to push around 300 mbps (~23K PPS) using this program on my local network until it overloaded my router and VM (I have a lower-end Edge Router). This was with no interval set and using one thread. The VM sending the information had 6 vCPUs and the processor was an older Intel Xeon clocked at 2.4 GHz. This VM was also using around 90 - 95% CPU when having this program running.

## Improvements
I am still fairly new to C and network programming. Therefore, I'm sure there are improvements that can be made. If you see anything that can be improved, please let me know :)

## Credits
* [Christian Deacon](https://www.linkedin.com/in/christian-deacon-902042186/) (AKA Roy).
* Online resources including Stack Overflow for help with calculating IP/UDP checksums.
# IPv4 Dataplane Router – Packet Forwarding in C

## 📌 Overview
This project implements the **dataplane** component of an IPv4 router in **C**, handling Ethernet frame parsing, IP packet forwarding, ARP resolution, and ICMP responses.  

The focus is on **low-level packet processing** and efficient routing decisions, simulating the behavior of a real router using a static routing table.

---

## 🚀 Features
- **Ethernet frame parsing**: Identify and handle IPv4, ARP, and broadcast frames.
- **Static routing table**: Forward packets using **Longest Prefix Match (LPM)** with support for large tables (~100k entries).
- **ARP protocol**:
  - ARP request/reply handling
  - ARP cache with timeout management
  - Packet queueing while awaiting ARP resolution
- **ICMP protocol**:
  - Echo Reply (ping response)
  - Time Exceeded (TTL expired)
  - Destination Unreachable
- **Error handling & defensive programming**: Robust against malformed packets and unexpected input.
- **Endianness management**: Correct conversion between host and network byte order.
- **Testing environment**: Compatible with **Mininet** and automated test scripts.

---

## 🛠️ Technologies & Tools
- **Languages:** C
- **Networking:** Ethernet, IPv4, ARP, ICMP
- **Tools:** Mininet, Wireshark, GCC, Make
- **Key Skills:** Low-level networking, packet parsing, routing algorithms, memory management

---

## 📂 Project Structure
common.c / common.h # Shared functions and definitions
map.c / map.h # Data structure for ARP and routing table management
server.c # Main router dataplane logic
subscriber.c # Auxiliary subscriber logic
Makefile # Build instructions


---

## ⚙️ How It Works
1. **Packet Reception** – Reads raw Ethernet frames from the network interface.
2. **Protocol Identification** – Determines if the payload is IPv4 or ARP.
3. **Forwarding Decision** – Uses Longest Prefix Match to find the next hop.
4. **ARP Resolution** – If MAC is unknown, send ARP request and queue packet.
5. **Packet Sending** – Once MAC is known, forward the packet to the correct interface.
6. **ICMP Responses** – Generate ICMP messages when necessary.

---

## ▶️ Build & Run
**Requirements:**
- GCC
- Linux environment
- Mininet (for testing)

**Build:**
make

**Run in Mininet:**

./server


**Test:**

# Run provided automated test scripts or use ping/arping

## 📊 Example Output
Received packet on interface eth0
Forwarding to eth1 (next hop: 192.168.0.2)
Sending ARP request for 192.168.0.2
ICMP Echo Reply sent to 10.0.0.1

# Layer-2-VPN

# Overview
This project simulates the behavior of a physical switch, by providing ethernet frame (L2) exchange service for devices (TAP) connected to the switch's port.

It is implemented as follows:

VSwitch: A software-based virtual switch with a MAC table and frame forwarding.

VPort: A virtual port interfacing with a TAP device, forwarding frames to/from the VSwitch.

# Prerequisite Knowledge
### Virtual Switch

A virtual network switch is a software-based switch that connects virtual machines (VMs) within a virtualized environment, enabling communication between VMs, virtual networks, and physical networks.

Operating as a multiport network bridge, it uses virtual MAC addresses to forward data at the data link layer (Layer 2) of the OSI model.

The switch maintains a forwarding table to map virtual MAC addresses to virtual ports. When a data frame is received, it forwards the frame to the correct port. If the destination is unknown, it broadcasts the frame to all ports except the source.

### Virtual Network Device (TAP Device)

A TAP device is a virtual network interface that simulates a physical network card, allowing operating systems and applications to use it like a real interface. It is commonly used in Virtual Private Network (VPN) setups to enable secure data transmission over public networks.

TAP devices are implemented in the operating system kernel and work as regular network interfaces. When packets are sent through the TAP device, they are passed to the TUN/TAP driver, which forwards them to an application for processing or sends them to other devices or networks. Similarly, incoming packets are processed by the application and delivered through the TAP device to their destination.

In this project, the TAP device connects client computers to the virtual switch, enabling packet forwarding between clients and the switch.


# How It Works
VSwitch:
* Listens on a UDP port.
* Processes Ethernet frames, learns MAC addresses, and forwards frames to the appropriate virtual ports.

VPort:
* Interfaces with a TAP device.
* Sends and receives Ethernet frames to/from the VSwitch over UDP.

# Features
* MAC address learning with LRU-based eviction.
* TAP device support for real-world Ethernet emulation.
* Batch processing for improved performance.
* Asynchronous event-driven architecture.


# Prerequisites
Python 3.8+
C++17
Linux with /dev/net/tun enabled
GCC/Clang for C++ compilation

# Setup
- Clone the project
- compile the code
  g++ -o vport vport.cpp tap_utils.cpp -lpthread


Note: Since, we are running everything locally on a single host, in the same network namespace, with both TAP devices as local interfaces, this usually prevents the kernel from sending ARP requests as it would in an actual LAN setup.

So to mimic an actual environment, we will create separate namespaces for each TAP device.


## 1) Create separate namespaces:
- sudo ip netns add ns0
- sudo ip netns add ns1

## 2) Start the vswitch.py:
- python3 vswitch.py 8000

## 3) Create TAP devices:
- sudo ./vport 127.0.0.1 8000 tapyuan0
- sudo ./vport 127.0.0.1 8000 tapyuan1

## 4) Put each TAP interface in its own namespace:
- sudo ip link set tapyuan0 netns ns0
- sudo ip link set tapyuan1 netns ns1

## 5) In namespace ns0, bring tapyuan0 up and give it an IP:
- sudo ip netns exec ns0 ip link set dev tapyuan0 up
- sudo ip netns exec ns0 ip addr add 10.0.0.2/24 dev tapyuan0

## 6) Repeat step 5 for interface ns1 and give tapyuan1 an IP:
- sudo ip netns exec ns1 ip link set dev tapyuan1 up
- sudo ip netns exec ns1 ip addr add 10.0.0.3/24 dev tapyuan1

## 7) Test the connection using ping:
- sudo ip netns exec ns0 ping -I tapyuan0 10.0.0.3
- sudo ip netns exec ns1 ping -I tapyuan1 10.0.0.2

![Screenshot From 2025-01-20 15-10-04](https://github.com/user-attachments/assets/1c36f186-7636-434d-ad7c-fc036113f6bf)



## Note: This project is only compactible with Linux.

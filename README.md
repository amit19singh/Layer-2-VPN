# Layer-2-VPN

# Overview
This project implements a Layer 2 VPN using:

VSwitch: A software-based virtual switch with a MAC table and frame forwarding.

VPort: A virtual port interfacing with a TAP device, forwarding frames to/from the VSwitch.

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
  git clone https://github.com/YourUsername/L2-VPN.git
  cd L2-VPN
- compile the code
  g++ -o vport vport.cpp tap_utils.cpp -lpthread


Note: Since, we are running everything locally on a single host, in the same network namespace, with both TAP devices as local interfaces, this usually prevents the kernel from sending ARP requests as it would in an actual LAN setup.

So to mimic an actual environment, we will create separate namespaces for each TAP device.


## 1) Create separate namespaces:
sudo ip netns add ns0
sudo ip netns add ns1

## 2) Start the vswitch.py:
python3 vswitch.py 8000

## 3) Create TAP devices:
sudo ./vport 127.0.0.1 8000 tapyuan0
sudo ./vport 127.0.0.1 8000 tapyuan1

## 4) Put each TAP interface in its own namespace:
sudo ip link set tapyuan0 netns ns0
sudo ip link set tapyuan1 netns ns1

## 5) In namespace ns0, bring tapyuan0 up and give it an IP:
sudo ip netns exec ns0 ip link set dev tapyuan0 up
sudo ip netns exec ns0 ip addr add 10.0.0.2/24 dev tapyuan0

## 6) Repeat step 5 for interface ns1 and give tapyuan1 an IP:
sudo ip netns exec ns1 ip link set dev tapyuan1 up
sudo ip netns exec ns1 ip addr add 10.0.0.3/24 dev tapyuan1

## 7) Test the connection using ping:
sudo ip netns exec ns0 ping -I tapyuan0 10.0.0.3

sudo ip netns exec ns1 ping -I tapyuan1 10.0.0.2




# Note: This project is only compactible with Linux.

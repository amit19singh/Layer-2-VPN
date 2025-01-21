#ifndef VPORT_HPP
#define VPORT_HPP

#include <string>
#include <thread>
#include <vector>
#include <netinet/in.h>

class VPort {
public:
    VPort(const std::string& vswitch_ip, int vswitch_port, const std::string& tap_name = "tapyuan");
    ~VPort();
    void start();  // Start forwarding traffic

private:
    int tap_fd;        // TAP device file descriptor
    int udp_socket;    // UDP socket for VSwitch communication
    sockaddr_in vswitch_addr;  // VSwitch address
    std::string tap_name;

    void setupTapDevice();
    void setupSocket(const std::string& vswitch_ip, int vswitch_port);

    void forwardToVSwitch();  // Forward TAP -> VSwitch
    void forwardToTAP();      // Forward VSwitch -> TAP

    std::thread upstreamThread;
    std::thread downstreamThread;
};

#endif

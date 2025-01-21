#include "vport.hpp"
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <net/ethernet.h>
#include <cstdio>



VPort::VPort(const std::string& vswitch_ip, int vswitch_port, const std::string& tap_name)
    : tap_name(tap_name) {
    setupTapDevice();
    setupSocket(vswitch_ip, vswitch_port);
}

VPort::~VPort() {
    close(tap_fd);
    close(udp_socket);
}

void VPort::setupTapDevice() {
    struct ifreq ifr{};
    tap_fd = open("/dev/net/tun", O_RDWR);
    if (tap_fd < 0) throw std::runtime_error("Failed to open /dev/net/tun");

    std::memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    // **Use the tap_name that was passed into the constructor**
    std::strncpy(ifr.ifr_name, tap_name.c_str(), IFNAMSIZ - 1);

    if (ioctl(tap_fd, TUNSETIFF, (void*)&ifr) < 0) {
        close(tap_fd);
        throw std::runtime_error("Failed to configure TAP device");
    }

    tap_name = ifr.ifr_name;
    std::cout << "[VPort] TAP device allocated: " << tap_name << std::endl;
}




void VPort::setupSocket(const std::string& vswitch_ip, int vswitch_port) {
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) throw std::runtime_error("Failed to create UDP socket");

    std::memset(&vswitch_addr, 0, sizeof(vswitch_addr));
    vswitch_addr.sin_family = AF_INET;
    vswitch_addr.sin_port = htons(vswitch_port);
    if (inet_pton(AF_INET, vswitch_ip.c_str(), &vswitch_addr.sin_addr) != 1) {
        close(udp_socket);
        throw std::runtime_error("Invalid VSwitch IP address");
    }
}

void VPort::start() {
    upstreamThread = std::thread(&VPort::forwardToVSwitch, this);
    downstreamThread = std::thread(&VPort::forwardToTAP, this);

    upstreamThread.join();
    downstreamThread.join();
}

void VPort::forwardToVSwitch() {
    std::vector<uint8_t> buffer(1500); // Ethernet frame buffer
    while (true) {
        int n = read(tap_fd, buffer.data(), buffer.size());
        if (n > 0) {
            // log the header if it's an Ethernet frame
            if (n >= 14) {
                const struct ether_header* hdr =
                    reinterpret_cast<const struct ether_header*>(buffer.data());

                // Print logs similar to your C version
                printf("[VPort] Sent to VSwitch: "
                       "dhost<%02x:%02x:%02x:%02x:%02x:%02x> "
                       "shost<%02x:%02x:%02x:%02x:%02x:%02x> "
                       "type<%04x> "
                       "datasz=<%d>\n",
                       hdr->ether_dhost[0], hdr->ether_dhost[1], hdr->ether_dhost[2],
                       hdr->ether_dhost[3], hdr->ether_dhost[4], hdr->ether_dhost[5],
                       hdr->ether_shost[0], hdr->ether_shost[1], hdr->ether_shost[2],
                       hdr->ether_shost[3], hdr->ether_shost[4], hdr->ether_shost[5],
                       ntohs(hdr->ether_type),
                       n);
            }
            // Forward frame to VSwitch via UDP socket
            sendto(udp_socket, buffer.data(), n, 0,
                   reinterpret_cast<const struct sockaddr*>(&vswitch_addr),
                   sizeof(vswitch_addr));
        }
    }
}

void VPort::forwardToTAP() {
    std::vector<uint8_t> buffer(1500); // Ethernet frame buffer
    while (true) {
        socklen_t addr_len = sizeof(vswitch_addr);
        int n = recvfrom(udp_socket, buffer.data(), buffer.size(), 0,
                         reinterpret_cast<struct sockaddr*>(&vswitch_addr), &addr_len);
        if (n > 0) {
            // If it's at least an Ethernet frame, log the header
            if (n >= 14) {
                const struct ether_header* hdr =
                    reinterpret_cast<const struct ether_header*>(buffer.data());

                printf("[VPort] Forward to TAP device: "
                       "dhost<%02x:%02x:%02x:%02x:%02x:%02x> "
                       "shost<%02x:%02x:%02x:%02x:%02x:%02x> "
                       "type<%04x> "
                       "datasz=<%d>\n",
                       hdr->ether_dhost[0], hdr->ether_dhost[1], hdr->ether_dhost[2],
                       hdr->ether_dhost[3], hdr->ether_dhost[4], hdr->ether_dhost[5],
                       hdr->ether_shost[0], hdr->ether_shost[1], hdr->ether_shost[2],
                       hdr->ether_shost[3], hdr->ether_shost[4], hdr->ether_shost[5],
                       ntohs(hdr->ether_type),
                       n);
            }
            // Write frame to TAP device
            write(tap_fd, buffer.data(), n);
        }
    }
}



int main(int argc, char const *argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: vport <VSwitch IP> <VSwitch Port> [TAP Name]" << std::endl;
        return EXIT_FAILURE;
    }

    const std::string server_ip_str = argv[1];
    int server_port = std::stoi(argv[2]);
    std::string tap_name = (argc == 4) ? argv[3] : "tapyuan"; // Use "tapyuan" as default if no name is provided

    try {
        VPort vport(server_ip_str, server_port, tap_name);
        vport.start();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

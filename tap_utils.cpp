#include "tap_utils.hpp"
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <fcntl.h>       
#include <unistd.h>      
#include <sys/ioctl.h>   
#include <linux/if.h>    
#include <linux/if_tun.h>

using namespace std;

int tap_alloc(const string& dev_name) {
    struct ifreq ifr;
    int fd;

    // Open the TAP/TUN driver
    fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        cerr << "Error: Failed to open /dev/net/tun. Error: " << strerror(errno) << endl;
        throw runtime_error("Failed to open /dev/net/tun");
    }

    // Configuring the TAP device
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI; // Set TAP device and disable packet info to exclude needless info
    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ - 1); // Copy device name (name we passed to the function)
    ifr.ifr_name[IFNAMSIZ - 1] = '\0'; 

    if (ioctl(fd, TUNSETIFF, (void*)&ifr) < 0) {
        cerr << "Error: ioctl(TUNSETIFF) failed. Error: " << strerror(errno) << endl;
        close(fd);
        throw runtime_error("Failed to configure TAP device with ioctl");
    }

    // Logging the successfully allocated TAP device
    cout << "[TAP Utils] TAP device allocated: " << ifr.ifr_name << endl;

    return fd; // Return the file descriptor for the TAP device
}

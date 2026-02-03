#include "sentinel.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/nvme_ioctl.h>
#include <scsi/sg.h>
#include <cpuid.h>
#include <openssl/sha.h>
#include <dirent.h>
#include <cstring>

namespace ENI {
    std::string get_cpu_brand() {
        uint32_t b[12];
        if (!__get_cpuid_max(0x80000004, NULL)) return "unknown_cpu";
        __get_cpuid(0x80000002, &b[0], &b[1], &b[2], &b[3]);
        __get_cpuid(0x80000003, &b[4], &b[5], &b[6], &b[7]);
        __get_cpuid(0x80000004, &b[8], &b[9], &b[10], &b[11]);
        return std::string((const char*)b, 48);
    }

    std::string get_nvme_serial() {
        int fd = open("/dev/nvme0", O_RDONLY);
        if (fd < 0) return "";
        struct nvme_admin_cmd cmd = {0};
        uint8_t buf[4096] = {0};
        cmd.opcode = 0x06; cmd.addr = (uintptr_t)buf; cmd.data_len = 4096; cmd.cdw10 = 1;
        std::string s = (ioctl(fd, NVME_IOCTL_ADMIN_CMD, &cmd) == 0) ? std::string((char*)buf + 4, 20) : "";
        close(fd); return s;
    }

    std::vector<uint8_t> derive_hardware_key() {
        std::string e = get_cpu_brand() + get_nvme_serial();
        std::vector<uint8_t> k(32);
        SHA256((const unsigned char*)e.c_str(), e.length(), k.data());
        return k;
    }
}

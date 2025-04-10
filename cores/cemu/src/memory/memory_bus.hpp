#ifndef XBAR_HPP
#define XBAR_HPP

#include "mmio_dev.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <utility>
#include <climits>
#include <queue>
#include <cstdio>

class memory_bus : public mmio_dev {
public:
    using dev_cfg_t = struct {
        mmio_dev *dev;
        bool raw_addr;
        bool trace_mem;
    };

    bool add_dev(uint64_t start_addr, uint64_t length, dev_cfg_t dev_cfg ) {
        std::pair<uint64_t, uint64_t> addr_range = std::make_pair(start_addr,start_addr+length);
        if (start_addr % length) return false;
        // check range
        auto it = devices.upper_bound(addr_range);
        if (it != devices.end()) {
            uint64_t l_max = std::max(it->first.first,addr_range.first);
            uint64_t r_min = std::min(it->first.second,addr_range.second);
            if (l_max < r_min) return false; // overleap
        }
        if (it != devices.begin()) {
            it = std::prev(it);
            uint64_t l_max = std::max(it->first.first,addr_range.first);
            uint64_t r_min = std::min(it->first.second,addr_range.second);
            if (l_max < r_min) return false; // overleap
        }
        // overleap check pass
        devices[addr_range] = dev_cfg;
        return true;
    }
    bool do_read(uint64_t start_addr, uint64_t size, unsigned char* buffer) {
        // printf("cemu: addr=%lx, size=%ld\n", start_addr, size);
        auto it = devices.upper_bound(std::make_pair(start_addr,ULONG_MAX));
        if (it == devices.begin()) return false;
        it = std::prev(it);
        uint64_t end_addr = start_addr + size;
        if (it->first.first <= start_addr && end_addr <= it->first.second) {
            uint64_t dev_size = it->first.second - it->first.first;
            const auto dev_cfg = it->second;
            auto ret = dev_cfg.dev->do_read(dev_cfg.raw_addr ? start_addr : start_addr % dev_size, size, buffer);
            return ret;
        }
        else return false;
    }
    bool do_write(uint64_t start_addr, uint64_t size, const unsigned char* buffer) {
        auto it = devices.upper_bound(std::make_pair(start_addr,ULONG_MAX));
        if (it == devices.begin()) return false;
        it = std::prev(it);
        uint64_t end_addr = start_addr + size;
        if (it->first.first <= start_addr && end_addr <= it->first.second) {
            uint64_t dev_size = it->first.second - it->first.first;
            const auto dev_cfg = it->second;
            const auto ret = dev_cfg.dev->do_write(dev_cfg.raw_addr ? start_addr : start_addr % dev_size, size, buffer);
            return ret;
        }
        else return false;
    }
private:
    std::map < std::pair<uint64_t,uint64_t>, dev_cfg_t > devices;
};

#endif
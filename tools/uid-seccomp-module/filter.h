#pragma once
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>

namespace uid_filter {
struct Result { long result; int error; int nnp_before; size_t instructions; };

// A process-local visibility experiment, not a general security sandbox.
inline Result install(std::vector<uint32_t> uids, bool other_apps = false) {
    const uint32_t self = getuid();
    if ((!other_apps && uids.empty()) || uids.size() > 256) return {-1, EINVAL, -1, 0};
    for (auto uid : uids) {
        if (uid == self || uid % 100000 < 10000 || uid % 100000 > 19999
                || uid / 100000 != self / 100000) return {-1, EINVAL, -1, 0};
    }
    std::sort(uids.begin(), uids.end());
    uids.erase(std::unique(uids.begin(), uids.end()), uids.end());
    std::vector<sock_filter> code;
    auto load = [&](uint32_t offset) { code.push_back(BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offset)); };
    auto equal = [&](uint32_t value, uint8_t yes, uint8_t no) {
        code.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, value, yes, no));
    };
    auto ret = [&](uint32_t value) { code.push_back(BPF_STMT(BPF_RET | BPF_K, value)); };
    load(offsetof(seccomp_data, arch));
    equal(AUDIT_ARCH_AARCH64, 1, 0); ret(SECCOMP_RET_ALLOW);
    load(offsetof(seccomp_data, nr));
    equal(__NR_getpriority, 1, 0); ret(SECCOMP_RET_ALLOW);
    // Kernel syscall parameters are int/id_t: compare their low 32 bits.
    load(offsetof(seccomp_data, args[0]));
    equal(PRIO_USER, 1, 0); ret(SECCOMP_RET_ALLOW);
    load(offsetof(seccomp_data, args[1]));
    if (other_apps) {
        equal(self, 0, 1); ret(SECCOMP_RET_ALLOW);
        const uint32_t first = self / 100000 * 100000 + 10000;
        code.push_back(BPF_JUMP(BPF_JMP | BPF_JGE | BPF_K, first, 1, 0)); ret(SECCOMP_RET_ALLOW);
        code.push_back(BPF_JUMP(BPF_JMP | BPF_JGT | BPF_K, first + 9999, 0, 1)); ret(SECCOMP_RET_ALLOW);
        ret(SECCOMP_RET_ERRNO | ESRCH);
    } else {
        for (auto uid : uids) { equal(uid, 0, 1); ret(SECCOMP_RET_ERRNO | ESRCH); }
    }
    ret(SECCOMP_RET_ALLOW);
    const int nnp = prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0);
    if (nnp < 0) return {-1, errno, nnp, code.size()};
    if (!nnp && prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) return {-1, errno, nnp, code.size()};
    sock_fprog program{static_cast<unsigned short>(code.size()), code.data()};
    errno = 0;
    const long result = syscall(__NR_seccomp, SECCOMP_SET_MODE_FILTER,
            SECCOMP_FILTER_FLAG_TSYNC, &program);
    // A positive TID is also TSYNC failure. Never call it an installed filter.
    return {result, result < 0 ? errno : 0, nnp, code.size()};
}
}

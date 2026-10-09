#include <sys/types.h>
#include "zygisk.hpp"
#include "filter.h"
#include <android/log.h>
#include <fcntl.h>
#include <sstream>
#include <string>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "CQUidSeccomp", __VA_ARGS__)

class UidSeccomp final : public zygisk::ModuleBase {
    zygisk::Api* api_ = nullptr;
    JNIEnv* env_ = nullptr;
    std::string package_;
    bool active_ = false;
    int expected_uid_ = -1;
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override { api_ = api; env_ = env; }
    void preServerSpecialize(zygisk::ServerSpecializeArgs*) override {
        api_->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }
    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        api_->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
        if (!args->app_data_dir || args->uid < 0 || args->uid % 100000 < 10000 || args->uid % 100000 > 19999) return;
        if (args->is_child_zygote && *args->is_child_zygote) return;
        const char* data = env_->GetStringUTFChars(args->app_data_dir, nullptr);
        if (!data) return;
        std::string path(data);
        env_->ReleaseStringUTFChars(args->app_data_dir, data);
        const auto slash = path.find_last_of('/');
        if (slash == std::string::npos || slash + 1 == path.size()) return;
        package_ = path.substr(slash + 1);
        if (package_.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.") != std::string::npos) return;
        const int dir = api_->getModuleDir();
        if (dir < 0) return;
        if (faccessat(dir, "disable", F_OK, 0) == 0 || faccessat(dir, "remove", F_OK, 0) == 0) { close(dir); return; }
        const int fd = openat(dir, "config", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        close(dir);
        if (fd < 0) return;
        char buffer[32769];
        ssize_t used = 0;
        while (used < static_cast<ssize_t>(sizeof(buffer))) {
            const ssize_t count = read(fd, buffer + used, sizeof(buffer) - size_t(used));
            if (count < 0 && errno == EINTR) continue;
            if (count < 0) { used = -1; break; }
            if (count == 0) break;
            used += count;
        }
        close(fd);
        if (used <= 0 || used >= static_cast<ssize_t>(sizeof(buffer))) return;
        std::istringstream input(std::string(buffer, size_t(used)));
        std::string line;
        if (!std::getline(input, line) || line != "enabled=1") return;
        while (std::getline(input, line)) {
            if (line == package_) { active_ = true; expected_uid_ = args->uid; break; }
        }
    }
    void postAppSpecialize(const zygisk::AppSpecializeArgs*) override {
        if (!active_ || getuid() != uint32_t(expected_uid_)) return;
        const auto result = uid_filter::install({}, true);
        LOG("version=0.2.0 package=%s uid=%u mode=other-apps install=%ld errno=%d nnp_before=%d instructions=%zu",
            package_.c_str(), getuid(), result.result, result.error, result.nnp_before, result.instructions);
    }
};
REGISTER_ZYGISK_MODULE(UidSeccomp)

#ifndef PROCESS_LAUNCHER_H
#define PROCESS_LAUNCHER_H

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>
#endif

#include <string>
#include <vector>

#ifdef _WIN32
    using ProcessHandle = HANDLE;
#else
    using ProcessHandle = pid_t;
#endif

class ProcessLauncher {
private:
    ProcessHandle process_handle;
    bool is_running;
    int exit_code;

public:
    ProcessLauncher();
    ~ProcessLauncher();

    bool launch(const std::string& program, const std::vector<std::string>& args = {});
    bool waitForFinish(int timeout = -1);
    int getExitCode() const;
    bool isRunning() const;
};

#endif

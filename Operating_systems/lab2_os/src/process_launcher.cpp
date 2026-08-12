#include "process_launcher.h"
#include <iostream>
#include <sstream>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>
    #include <cerrno>
#endif

ProcessLauncher::ProcessLauncher() : process_handle(0), is_running(false), exit_code(0) {}

ProcessLauncher::~ProcessLauncher() {
#ifdef _WIN32
    if (process_handle != NULL && process_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(process_handle);
    }
#endif
}

bool ProcessLauncher::launch(const std::string& program, const std::vector<std::string>& args) {
#ifdef _WIN32
    std::stringstream cmd_line;
    cmd_line << "\"" << program << "\"";
    for (const auto& arg : args) {
        cmd_line << " \"" << arg << "\"";
    }
    std::string cmd_str = cmd_line.str();

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    DWORD creation_flags = CREATE_NO_WINDOW;

    if (!CreateProcessA(
        NULL,
        (LPSTR)cmd_str.c_str(),
        NULL,
        NULL,
        FALSE,
        creation_flags,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        std::cerr << "Ошибка CreateProcess: " << GetLastError() << std::endl;
        return false;
    }

    CloseHandle(pi.hThread);
    process_handle = pi.hProcess;
    is_running = true;
    return true;

#else
    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Ошибка fork: " << strerror(errno) << std::endl;
        return false;
    }

    if (pid == 0) {
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(program.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(program.c_str(), argv.data());
        std::cerr << "Ошибка execvp: " << strerror(errno) << std::endl;
        _exit(127);
    } else {
        process_handle = pid;
        is_running = true;
        return true;
    }
#endif
}

bool ProcessLauncher::waitForFinish(int timeout) {
    if (!is_running) return true;

#ifdef _WIN32
    DWORD wait_result;
    if (timeout < 0) {
        wait_result = WaitForSingleObject(process_handle, INFINITE);
    } else {
        wait_result = WaitForSingleObject(process_handle, static_cast<DWORD>(timeout));
    }
if (wait_result == WAIT_OBJECT_0) {
        DWORD exit_code_win;
        if (GetExitCodeProcess(process_handle, &exit_code_win)) {
            exit_code = static_cast<int>(exit_code_win);
        }
        is_running = false;
        return true;
    } else if (wait_result == WAIT_TIMEOUT) {
        return false;
    } else {
        std::cerr << "Ошибка WaitForSingleObject: " << GetLastError() << std::endl;
        return false;
    }

#else
    if (timeout < 0) {
        int status;
        pid_t result = waitpid(process_handle, &status, 0);
        if (result == process_handle) {
            if (WIFEXITED(status)) {
                exit_code = WEXITSTATUS(status);
            } else {
                exit_code = -1;
            }
            is_running = false;
            return true;
        } else {
            std::cerr << "Ошибка waitpid: " << strerror(errno) << std::endl;
            return false;
        }
    } else {
        int elapsed = 0;
        int status;
        while (elapsed < timeout) {
            pid_t result = waitpid(process_handle, &status, WNOHANG);
            if (result == process_handle) {
                if (WIFEXITED(status)) {
                    exit_code = WEXITSTATUS(status);
                } else {
                    exit_code = -1;
                }
                is_running = false;
                return true;
            } else if (result < 0) {
                std::cerr << "Ошибка waitpid: " << strerror(errno) << std::endl;
                return false;
            }
            usleep(10000);
            elapsed += 10;
        }
        return false;
    }
#endif
}

int ProcessLauncher::getExitCode() const {
    return exit_code;
}

bool ProcessLauncher::isRunning() const {
    if (!is_running) return false;

#ifdef _WIN32
    DWORD exit_code_win;
    if (GetExitCodeProcess(process_handle, &exit_code_win) && 
        exit_code_win == STILL_ACTIVE) {
        return true;
    }
    return false;
#else
    int status;
    pid_t result = waitpid(process_handle, &status, WNOHANG);
    if (result == 0) {
        return true;
    } else if (result == process_handle) {
        const_cast<ProcessLauncher*>(this)->is_running = false;
        if (WIFEXITED(status)) {
            const_cast<ProcessLauncher*>(this)->exit_code = WEXITSTATUS(status);
        } else {
            const_cast<ProcessLauncher*>(this)->exit_code = -1;
        }
        return false;
    }
    return false;
#endif
}

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #define getpid _getpid
    #define SLEEP_MS(ms) Sleep(ms)
#else
    #include <unistd.h>
    #include <sys/mman.h>
    #include <sys/wait.h>
    #include <sys/time.h>
    #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

const char* SHM_NAME = "/lab3_counter_shm_v2";

struct SharedData {
    int counter;
    bool is_leader;
    pid_t child1_pid;
    pid_t child2_pid;
    time_t child1_start;
    time_t child2_start;
    bool leader_alive;
};

SharedData* shared = nullptr;
#ifdef _WIN32
    HANDLE shm_handle = NULL;
#else
    int shm_fd = -1;
#endif
std::atomic<bool> running(true);
int my_pid = 0;

std::string get_timestamp() {
    char buf[50];
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm* tm_info = localtime(&tv.tv_sec);
    strftime(buf, sizeof(buf), "%H:%M:%S", tm_info);
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ".%03ld", tv.tv_usec / 1000);
#endif
    return std::string(buf);
}

void log_message(const std::string& msg) {
    std::ofstream log("log.txt", std::ios::app);
    if (log.is_open()) {
        log << "[" << get_timestamp() << "] PID=" << my_pid << " | " << msg << std::endl;
        log.close();
    }
}

bool init_shared_memory() {
#ifdef _WIN32
    shm_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedData), SHM_NAME);
    if (!shm_handle) return false;
    bool first = (GetLastError() != ERROR_ALREADY_EXISTS);
    shared = (SharedData*)MapViewOfFile(shm_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    if (!shared) return false;
    if (first) {
        memset(shared, 0, sizeof(SharedData));
        shared->counter = 0;
        shared->is_leader = true;
        shared->child1_pid = -1;
        shared->child2_pid = -1;
        shared->leader_alive = true;
    } else {
        shared->is_leader = false;
    }
    return true;
#else
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) return false;
    ftruncate(shm_fd, sizeof(SharedData));
    shared = (SharedData*)mmap(0, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) return false;
    
    static bool initialized = false;
    if (!initialized) {
        memset(shared, 0, sizeof(SharedData));
        shared->counter = 0;
        shared->is_leader = true;
        shared->child1_pid = -1;
        shared->child2_pid = -1;
        shared->leader_alive = true;
        initialized = true;
    } else {
        shared->is_leader = false;
    }
    return true;
#endif
}

void launch_child(const char* child_type) {
#ifdef _WIN32
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "lab3_os.exe %s", child_type);
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        if (strcmp(child_type, "child1") == 0) {
            shared->child1_pid = pi.dwProcessId;
            shared->child1_start = time(nullptr);
        } else {
            shared->child2_pid = pi.dwProcessId;
            shared->child2_start = time(nullptr);
        }
        CloseHandle(pi.hProcess);
    }
#else
    pid_t pid = fork();
    if (pid == 0) {
        execl("./lab3_os", "./lab3_os", child_type, (char*)nullptr);
        _exit(1);
    } else if (pid > 0) {
        if (strcmp(child_type, "child1") == 0) {
            shared->child1_pid = pid;
            shared->child1_start = time(nullptr);
        } else {
            shared->child2_pid = pid;
            shared->child2_start = time(nullptr);
        }
    }
#endif
}

bool is_child_finished(pid_t pid) {
    if (pid <= 0) return true;
#ifdef _WIN32
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!h) return true;
    DWORD code;
    bool finished = (GetExitCodeProcess(h, &code) && code != STILL_ACTIVE);
    CloseHandle(h);
    return finished;
#else
    int status;
    return (waitpid(pid, &status, WNOHANG) != 0);
#endif
}

void signal_handler(int signum) {
    running = false;
#ifndef _WIN32
    if (shared) shared->leader_alive = false;
#endif
}

void input_thread() {
    std::string input;
    while (running) {
        if (std::cin >> input) {
            try {
                int value = std::stoi(input);
                if (shared) {
                    shared->counter = value;
                    log_message("Пользователь установил счётчик = " + std::to_string(value));
                }
            } catch (...) {}
        }
        SLEEP_MS(100);
    }
}

void run_child1() {
    log_message("КОПИЯ 1: запущена");
    if (shared) {
        shared->counter += 10;
        log_message("КОПИЯ 1: счётчик +10 → " + std::to_string(shared->counter));
    }
    log_message("КОПИЯ 1: завершается");
    exit(0);
}

void run_child2() {
    log_message("КОПИЯ 2: запущена");
    if (shared) {
        shared->counter *= 2;
        log_message("КОПИЯ 2: счётчик ×2 → " + std::to_string(shared->counter));
        for (int i = 0; i < 20 && running; i++) SLEEP_MS(100);
        shared->counter /= 2;
        log_message("КОПИЯ 2: счётчик ÷2 → " + std::to_string(shared->counter));
    }
    log_message("КОПИЯ 2: завершается");
    exit(0);
}

int main(int argc, char* argv[]) {
    my_pid = getpid();
    
    if (argc > 1) {
        if (!init_shared_memory()) return 1;
        if (strcmp(argv[1], "child1") == 0) run_child1();
        else if (strcmp(argv[1], "child2") == 0) run_child2();
        return 0;
    }
    
    if (!init_shared_memory()) {
        std::cerr << "Ошибка инициализации" << std::endl;
        return 1;
    }
    
    signal(SIGINT, signal_handler);
#ifndef _WIN32
    signal(SIGTERM, signal_handler);
#endif
    
    log_message(std::string("=== ЗАПУСК (") + (shared->is_leader ? "ведущий" : "обычный") + " процесс) ===");
    log_message("Счётчик = " + std::to_string(shared->counter));
    
    std::thread input_thr(input_thread);
    
    auto last_300ms = std::chrono::steady_clock::now();
    auto last_1sec = std::chrono::steady_clock::now();
    auto last_3sec = std::chrono::steady_clock::now();
    
    while (running) {
        auto now = std::chrono::steady_clock::now();
        
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_300ms).count() >= 300) {
            if (shared) shared->counter++;
            last_300ms = now;
        }
        
        if (shared && shared->is_leader && std::chrono::duration_cast<std::chrono::milliseconds>(now - last_1sec).count() >= 1000) {
            log_message("Счётчик = " + std::to_string(shared->counter));
            last_1sec = now;
        }
        
        if (shared && shared->is_leader && std::chrono::duration_cast<std::chrono::milliseconds>(now - last_3sec).count() >= 3000) {
            bool c1 = is_child_finished(shared->child1_pid);
            bool c2 = is_child_finished(shared->child2_pid);
            
            if (!c1 || !c2) {
                log_message("ПРЕДУПРЕЖДЕНИЕ: предыдущие копии ещё работают");
            } else {
                log_message("Запуск копий...");
                launch_child("child1");
                launch_child("child2");
            }
            last_3sec = now;
        }
        
        SLEEP_MS(50);
    }
    
    if (input_thr.joinable()) input_thr.join();
    
#ifdef _WIN32
    if (shared) UnmapViewOfFile(shared);
    if (shm_handle) CloseHandle(shm_handle);
#else
    if (shared) {
        if (shared->is_leader) shared->leader_alive = false;
        munmap(shared, sizeof(SharedData));
    }
    if (shm_fd != -1) close(shm_fd);
#endif
    
    log_message("=== ЗАВЕРШЕНИЕ ===");
    return 0;
}

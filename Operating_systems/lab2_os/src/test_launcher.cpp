#include "process_launcher.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== ТЕСТ БИБЛИОТЕКИ ЗАПУСКА ПРОЦЕССОВ ===\n" << std::endl;

    std::cout << "[ТЕСТ 1] Запуск 'gcc --version'..." << std::endl;
    ProcessLauncher launcher1;
    bool ok1 = launcher1.launch("gcc", {"--version"});
    if (!ok1) {
        std::cerr << "  ОШИБКА: не удалось запустить gcc!" << std::endl;
        return 1;
    }
    std::cout << "  Процесс запущен в фоне..." << std::endl;
    if (launcher1.waitForFinish(5000)) {
        std::cout << "  Завершён. Код возврата: " << launcher1.getExitCode() << std::endl;
    } else {
        std::cout << "  Таймаут 5 секунд" << std::endl;
    }
    std::cout << std::endl;
std::cout << "[ТЕСТ 2] Запуск 'sleep 2' (процесс на 2 сек)..." << std::endl;
    ProcessLauncher launcher2;
    bool ok2 = launcher2.launch("sleep", {"2"});
    if (!ok2) {
        std::cerr << "  ОШИБКА: не удалось запустить sleep!" << std::endl;
        return 1;
    }
    std::cout << "  Сразу после запуска: работает = " << (launcher2.isRunning() ? "ДА" : "НЕТ") << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "  Через 1 сек: работает = " << (launcher2.isRunning() ? "ДА" : "НЕТ") << std::endl;
    launcher2.waitForFinish();
    std::cout << "  После ожидания: работает = " << (launcher2.isRunning() ? "ДА" : "НЕТ") << std::endl;
    std::cout << "  Код возврата: " << launcher2.getExitCode() << std::endl;
    std::cout << std::endl;

    std::cout << "=== ВСЁ ГОТОВО! БИБЛИОТЕКА РАБОТАЕТ ===" << std::endl;
    return 0;
}

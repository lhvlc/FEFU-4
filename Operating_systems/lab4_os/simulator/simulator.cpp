#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <random>
#include <csignal>
#include <iomanip>

volatile sig_atomic_t keep_running = 1;

void signal_handler(int signal) {
    keep_running = 0;
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Создаём файл В КОРНЕ lab4_os (на уровень выше)
    std::ofstream port("../virtual_port.txt");
    if (!port.is_open()) {
        std::cerr << "ОШИБКА: не могу создать ../virtual_port.txt\n";
        std::cerr << "Запускайте программу из папки simulator/\n";
        return 1;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> temp_dist(22.0, 3.0); // 22°C ±3°

    std::cout << "========================================\n";
    std::cout << " СИМУЛЯТОР ТЕРМОМЕТРА ЗАПУЩЕН\n";
    std::cout << "========================================\n";
    std::cout << "Пишет температуру в ../virtual_port.txt\n";
    std::cout << "Измерения каждые 5 секунд\n";
    std::cout << "Для остановки нажмите Ctrl+C\n";
    std::cout << "========================================\n\n";

    while (keep_running) {
        double temp = temp_dist(gen);
        port << std::fixed << std::setprecision(1) << temp << "\n";
        port.flush(); // Сразу на диск

        std::cout << "Температура: " << std::fixed << std::setprecision(1) 
                  << temp << " °C\r" << std::flush;

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    port.close();
    std::cout << "\n\n Симулятор остановлен. Данные сохранены.\n";
    return 0;
}

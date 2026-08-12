#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>
#include <string>
#include <algorithm>
#ifdef _WIN32
    #include <io.h>
    #define FILE_EXISTS _access
#else
    #include <unistd.h>
    #define FILE_EXISTS access
#endif

// Текущее время как строка
std::string current_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    #ifdef _WIN32
        std::tm tm;
        localtime_s(&tm, &t);
    #else
        std::tm tm = *std::localtime(&t);
    #endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Прочитать все строки из файла
std::vector<std::string> read_lines(const std::string& filename) {
    std::vector<std::string> lines;
    std::ifstream f(filename);
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

// Добавить строку в конец файла
void append_line(const std::string& filename, const std::string& line) {
    std::ofstream f(filename, std::ios::app);
    f << line << "\n";
    f.close();
}

int main() {
    std::cout << "========================================\n";
    std::cout << " ЛОГГЕР ТЕМПЕРАТУРЫ ЗАПУЩЕН\n";
    std::cout << "========================================\n";
    std::cout << "Читаю из ../virtual_port.txt\n";
    std::cout << "raw.log      → все измерения (24ч)\n";
    std::cout << "hourly.log   → средние за час (30дн)\n";
    std::cout << "daily.log    → средние за день (год)\n";
    std::cout << "Для остановки нажмите Ctrl+C\n";
    std::cout << "========================================\n\n";

    std::string last_read;
    std::vector<double> hour_temps;
    std::vector<double> day_temps;

    auto last_hour = std::chrono::steady_clock::now();
    auto last_day = std::chrono::steady_clock::now();

    // Для тестирования можно уменьшить интервалы (раскомментировать):
    // const int HOUR_INTERVAL = 60;   // 1 минута вместо часа
    // const int DAY_INTERVAL = 300;    // 5 минут вместо дня
    const int HOUR_INTERVAL = 3600;   // настоящий час (сек)
    const int DAY_INTERVAL = 86400;   // настоящий день (сек)

    while (true) {
        // Проверяем, существует ли файл порта
        if (FILE_EXISTS("../virtual_port.txt", 0) != 0) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        // Читаем последнюю строку из файла-порта
        std::ifstream port_file("../virtual_port.txt");
        std::string line, last_line;
        while (std::getline(port_file, line)) {
            if (!line.empty()) last_line = line;
        }

        // Пропускаем, если ничего нового
        if (last_line.empty() || last_line == last_read) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        last_read = last_line;

        // Парсим температуру
        double temp = 0.0;
        try {
            temp = std::stod(last_line);
        } catch (...) {
            continue; // битые данные — пропускаем
        }

        std::string ts = current_time();

        // 1. Записываем в сырые данные
        append_line("raw.log", ts + " " + std::to_string(temp));

        // 2. Накапливаем для расчётов
        hour_temps.push_back(temp);
        day_temps.push_back(temp);

        // 3. Проверка часа
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_hour).count() >= HOUR_INTERVAL) {
            if (!hour_temps.empty()) {
                double sum = 0.0;
                for (double t : hour_temps) sum += t;
                double avg = sum / hour_temps.size();
                append_line("hourly.log", ts + " " + std::to_string(avg));
                std::cout << "[ЧАС] Средняя: " << std::fixed << std::setprecision(1) 
                          << avg << " °C\n";
                hour_temps.clear();
            }
            last_hour = now;
        }

        // 4. Проверка дня
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_day).count() >= DAY_INTERVAL) {
            if (!day_temps.empty()) {
                double sum = 0.0;
                for (double t : day_temps) sum += t;
                double avg = sum / day_temps.size();
                append_line("daily.log", ts + " " + std::to_string(avg));
                std::cout << "[ДЕНЬ] Средняя: " << std::fixed << std::setprecision(1) 
                          << avg << " °C\n";
                day_temps.clear();
            }
            last_day = now;
        }

        // 5. Ротация логов (удаление старых записей)
        // raw.log — оставляем ~1750 записей (24ч * 3600сек / 5сек ≈ 1728)
        {
            auto raw = read_lines("raw.log");
            if (raw.size() > 1800) {
                std::ofstream f("raw.log");
                for (size_t i = raw.size() - 1750; i < raw.size(); ++i) {
                    f << raw[i] << "\n";
                }
                f.close();
            }
        }

        // hourly.log — оставляем ~730 записей (30 дней * 24 часа)
        {
            auto hourly = read_lines("hourly.log");
            if (hourly.size() > 750) {
                std::ofstream f("hourly.log");
                for (size_t i = hourly.size() - 730; i < hourly.size(); ++i) {
                    f << hourly[i] << "\n";
                }
                f.close();
            }
        }

        // daily.log — не чистим (хранит год)

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}

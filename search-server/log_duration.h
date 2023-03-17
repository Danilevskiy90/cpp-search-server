//Класс для выполнения профилирования кода

#pragma once

#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
//макрос для вывода в cerr - нужно указать только имя функции, которая подлежит профилированию
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
//макрос для вывода в cout - требуется указать поток, куда осуществляется вывод, сопровождающийся подписью "Operation time"
#define LOG_DURATION_STREAM(out_stream) LogDuration UNIQUE_VAR_NAME_PROFILE("Operation time"s, out_stream)

class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;

    LogDuration(std::string code_block_name, std::ostream& out_stream_ = std::cerr)
        : code_block_name_(code_block_name), out_stream(out_stream_) {}

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        out_stream << code_block_name_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const std::string code_block_name_;
    const Clock::time_point start_time_ = Clock::now();
    std::ostream& out_stream;
};
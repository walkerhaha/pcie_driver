#include "simlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"



spdlog::logger *f_get_logger() {
    static spdlog::logger *g_logger = NULL;
    static int g_logger_init = 0;
    if(!g_logger_init) {
        auto console_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
        console_sink->set_level(LOG_LEVEL);
        console_sink->set_pattern("[MT] [%H:%M:%S %e] [%^%l%$] %v");

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("./logs.txt", true);
        file_sink->set_level(LOG_LEVEL);

        static spdlog::logger logger("MT", {console_sink, file_sink});
        g_logger = &logger;
        g_logger_init = 1;
    }
    return g_logger;
}


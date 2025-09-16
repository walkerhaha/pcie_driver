#ifndef _SIMLOG_H_
#define _SIMLOG_H_

#include <spdlog/spdlog.h>

#define LOG_LEVEL spdlog::level::debug

spdlog::logger *f_get_logger();

#define LTrace(msg,...)  f_get_logger()->trace(SUFFIX(msg),__VA_ARGS__)
#define LDebug(...)  f_get_logger()->debug(__VA_ARGS__)
#define LInfo(...)  f_get_logger()->info(__VA_ARGS__)
#define LWarn(...) f_get_logger()->warn(__VA_ARGS__)
#define LError(...)  f_get_logger()->error(__VA_ARGS__)
#define LCritical(...)  f_get_logger()->critical(__VA_ARGS__)


#endif //_SIMLOG_H_

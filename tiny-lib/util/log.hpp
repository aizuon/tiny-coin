#pragma once
#include <memory>
#ifndef SPDLOG_FMT_EXTERNAL
#define SPDLOG_FMT_EXTERNAL
#endif
#include <spdlog/spdlog.h>

class Log
{
public:
	static void start_log(bool async = true);
	static void stop_log();

	static inline std::shared_ptr<spdlog::logger>& get_logger() { return logger; }

private:
	static std::shared_ptr<spdlog::logger> logger;
};

#define LOG_TRACE(...)    do { if (Log::get_logger()) Log::get_logger()->trace(__VA_ARGS__); } while(0)
#define LOG_INFO(...)     do { if (Log::get_logger()) Log::get_logger()->info(__VA_ARGS__); } while(0)
#define LOG_WARN(...)     do { if (Log::get_logger()) Log::get_logger()->warn(__VA_ARGS__); } while(0)
#define LOG_ERROR(...)    do { if (Log::get_logger()) Log::get_logger()->error(__VA_ARGS__); } while(0)
#define LOG_CRITICAL(...) do { if (Log::get_logger()) Log::get_logger()->critical(__VA_ARGS__); } while(0)

#pragma once
#include <memory>
#ifndef SPDLOG_WCHAR_TO_UTF8_SUPPORT
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#endif
#include <spdlog/spdlog.h>

class Log
{
public:
	static void StartLog(bool async = true);
	static void StopLog();

	static inline std::shared_ptr<spdlog::logger>& GetLogger() { return Logger; }

private:
	static std::shared_ptr<spdlog::logger> Logger;
};

#define LOG_TRACE(...)    Log::GetLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...)     Log::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)     Log::GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    Log::GetLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) Log::GetLogger()->critical(__VA_ARGS__)

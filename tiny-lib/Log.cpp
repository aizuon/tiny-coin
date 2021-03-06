#include "pch.hpp"
#include "Log.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Log::Logger;

void Log::StartLog(bool async /*= true*/)
{
	std::vector<spdlog::sink_ptr> logSinks;

	char logFile[MAX_PATH];
	const std::string logFolder = "log\\";
	if (GetFileAttributes(logFolder.c_str()) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(logFolder.c_str(), nullptr);
	SYSTEMTIME t;
	GetSystemTime(&t);
	sprintf_s(logFile, MAX_PATH, "TinyCoin_%4d%02d%02d_%02d%02d%02d.log", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute,
	          t.wSecond);
	std::string path = logFolder + std::string(logFile);
	logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path));
	logSinks[0]->set_pattern("[ %T ] [ %l ] [ %n ] %v");

	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks[1]->set_pattern("%^[ %T ] [ %n ] %v%$");

	if (async)
	{
		spdlog::init_thread_pool(128, 1);

		Logger = std::make_shared<spdlog::async_logger>("tc", logSinks.begin(), logSinks.end(), spdlog::thread_pool(),
			spdlog::async_overflow_policy::block);
	}
	else
	{
		Logger = std::make_shared<spdlog::logger>("tc", logSinks.begin(), logSinks.end());
	}
	
	Logger->set_level(spdlog::level::trace);
	Logger->flush_on(spdlog::level::trace);
	spdlog::register_logger(Logger);
	spdlog::set_default_logger(Logger);
}

void Log::StopLog()
{
	Logger->flush();
	spdlog::shutdown();
}

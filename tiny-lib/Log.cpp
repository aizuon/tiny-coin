#include "pch.hpp"
#include "Log.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Log::Logger;

void Log::StartLog(bool async /*= true*/)
{
	std::vector<spdlog::sink_ptr> log_sinks;

	char log_file[MAX_PATH];
	const std::string log_folder = "log\\";
	if (GetFileAttributes(log_folder.c_str()) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(log_folder.c_str(), nullptr);
	SYSTEMTIME t;
	GetSystemTime(&t);
	sprintf_s(log_file, MAX_PATH, "TinyCoin_%4d%02d%02d_%02d%02d%02d.log", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute,
	          t.wSecond);
	std::string path = log_folder + std::string(log_file);
	log_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path));
	log_sinks[0]->set_pattern("[ %T ] [ %l ] [ %n ] %v");

	log_sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	log_sinks[1]->set_pattern("%^[ %T ] [ %n ] %v%$");

	if (async)
	{
		spdlog::init_thread_pool(128, 1);

		Logger = std::make_shared<spdlog::async_logger>("tc", log_sinks.begin(), log_sinks.end(), spdlog::thread_pool(),
			spdlog::async_overflow_policy::block);
	}
	else
	{
		Logger = std::make_shared<spdlog::logger>("tc", log_sinks.begin(), log_sinks.end());
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

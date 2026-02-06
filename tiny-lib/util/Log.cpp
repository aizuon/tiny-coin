#include "util/log.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Log::logger;

void Log::start_log(bool async /*= true*/)
{
	std::vector<spdlog::sink_ptr> log_sinks;

	const std::filesystem::path log_folder = "log";
	std::filesystem::create_directories(log_folder);

	const auto now = std::chrono::system_clock::now();
	const auto time = std::chrono::system_clock::to_time_t(now);
	std::tm tm{};
#ifdef _WIN32
	gmtime_s(&tm, &time);
#else
	gmtime_r(&time, &tm);
#endif
	char log_file[256];
	std::snprintf(log_file, sizeof(log_file), "TinyCoin_%4d%02d%02d_%02d%02d%02d.log",
	              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	              tm.tm_hour, tm.tm_min, tm.tm_sec);
	const auto path = (log_folder / log_file).string();
	log_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path));
	log_sinks[0]->set_pattern("[ %T ] [ %l ] [ %n ] %v");

	log_sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	log_sinks[1]->set_pattern("%^[ %T ] [ %n ] %v%$");

	if (async)
	{
		spdlog::init_thread_pool(128, 1);

		logger = std::make_shared<spdlog::async_logger>("tc", log_sinks.begin(), log_sinks.end(), spdlog::thread_pool(),
			spdlog::async_overflow_policy::block);
	}
	else
	{
		logger = std::make_shared<spdlog::logger>("tc", log_sinks.begin(), log_sinks.end());
	}
	
	logger->set_level(spdlog::level::trace);
	logger->flush_on(spdlog::level::trace);
	spdlog::register_logger(logger);
	spdlog::set_default_logger(logger);
}

void Log::stop_log()
{
	logger->flush();
	spdlog::shutdown();
}

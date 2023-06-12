#include "pch.hpp"
#include "Log.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>
#include <boost/date_time/posix_time/posix_time.hpp>

std::shared_ptr<spdlog::logger> Log::Logger;

void Log::StartLog(bool async /*= true*/)
{
	std::vector<spdlog::sink_ptr> log_sinks;

	const std::filesystem::path log_folder("./log/");

	if (!std::filesystem::exists(log_folder))
		std::filesystem::create_directory(log_folder);

	auto now = boost::posix_time::second_clock::local_time();
	std::stringstream log_file_ss;
	log_file_ss << "TinyCoin_" << boost::posix_time::to_iso_string(now) << ".log";
	std::string path = log_folder.string() + log_file_ss.str();
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

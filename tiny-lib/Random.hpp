#pragma once
#include <random>
#include <mutex>

class Random
{
public:
	static int64_t GetInt(int64_t start, int64_t end);

private:
	static std::random_device rd;
	static std::mt19937 gen;
	static std::mutex mtx;
};

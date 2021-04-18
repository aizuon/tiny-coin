#include "pch.hpp"

#include "Random.hpp"

std::random_device Random::rd;
std::mt19937 Random::gen(Random::rd());

int64_t Random::GetInt(int64_t start, int64_t end)
{
	std::uniform_int_distribution<int64_t> dist(start, end);

	return dist(rd);
}

#pragma once
#include <memory>

#include "mining/i_mining_backend.hpp"

class MiningBackendFactory
{
public:
    static std::unique_ptr<IMiningBackend> create();
    static std::unique_ptr<IMiningBackend> create_cpu();
};

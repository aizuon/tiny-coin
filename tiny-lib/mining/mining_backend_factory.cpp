#include "mining/mining_backend_factory.hpp"

#include "mining/cpu_mining_backend.hpp"
#include "util/log.hpp"

#ifdef __APPLE__
#include "mining/metal/metal_mining_backend.hpp"
#endif

std::unique_ptr<IMiningBackend> MiningBackendFactory::create()
{
#ifdef __APPLE__
    auto metal_backend = std::make_unique<MetalMiningBackend>();
    if (metal_backend->is_available())
    {
        LOG_INFO("Mining backend: Metal GPU");
        return metal_backend;
    }
    LOG_INFO("Metal GPU not available, falling back to CPU mining");
#endif

    LOG_INFO("Mining backend: CPU");
    return std::make_unique<CpuMiningBackend>();
}

std::unique_ptr<IMiningBackend> MiningBackendFactory::create_cpu()
{
    return std::make_unique<CpuMiningBackend>();
}

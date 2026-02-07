#ifdef __APPLE__

#include "mining/metal/metal_mining_backend.hpp"

#include <cstring>
#include <string>

#import <Metal/Metal.h>

#include "util/log.hpp"

static const char* MINING_KERNEL_SOURCE =
#include "mining/metal/mining_kernel_source.inc"
;

struct MetalContext
{
	id<MTLDevice> device = nil;
	id<MTLCommandQueue> command_queue = nil;
	id<MTLComputePipelineState> pipeline_state = nil;
	id<MTLLibrary> library = nil;

	~MetalContext()
	{
		pipeline_state = nil;
		command_queue = nil;
		library = nil;
		device = nil;
	}
};

MetalMiningBackend::MetalMiningBackend()
	: ctx_(std::make_unique<MetalContext>())
{
	@autoreleasepool
	{
		ctx_->device = MTLCreateSystemDefaultDevice();
		if (!ctx_->device)
		{
			LOG_WARN("Metal: No GPU device found");
			return;
		}

		NSError* error = nil;
		NSString* source = [NSString stringWithUTF8String:MINING_KERNEL_SOURCE];
		ctx_->library = [ctx_->device newLibraryWithSource:source options:nil error:&error];
		if (!ctx_->library)
		{
			LOG_WARN("Metal: Failed to compile shader library: {}",
				error ? [[error localizedDescription] UTF8String] : "unknown error");
			ctx_.reset();
			ctx_ = std::make_unique<MetalContext>();
			return;
		}

		id<MTLFunction> function = [ctx_->library newFunctionWithName:@"mine_kernel"];
		if (!function)
		{
			LOG_WARN("Metal: Failed to find mine_kernel function");
			ctx_.reset();
			ctx_ = std::make_unique<MetalContext>();
			return;
		}

		ctx_->pipeline_state = [ctx_->device newComputePipelineStateWithFunction:function error:&error];
		if (!ctx_->pipeline_state)
		{
			LOG_WARN("Metal: Failed to create compute pipeline: {}",
				error ? [[error localizedDescription] UTF8String] : "unknown error");
			ctx_.reset();
			ctx_ = std::make_unique<MetalContext>();
			return;
		}

		ctx_->command_queue = [ctx_->device newCommandQueue];
		if (!ctx_->command_queue)
		{
			LOG_WARN("Metal: Failed to create command queue");
			ctx_.reset();
			ctx_ = std::make_unique<MetalContext>();
			return;
		}

		available_ = true;
		LOG_INFO("Metal mining backend initialized: {}", [[ctx_->device name] UTF8String]);
	}
}

MetalMiningBackend::~MetalMiningBackend() = default;

std::string MetalMiningBackend::name() const
{
	return "metal";
}

bool MetalMiningBackend::is_available() const
{
	return available_;
}

MineResult MetalMiningBackend::mine(const std::vector<uint8_t>& header_prefix,
	const std::vector<uint8_t>& target_bytes, std::atomic_bool& interrupt)
{
	MineResult result;
	if (!available_)
		return result;

	@autoreleasepool
	{
		const uint32_t prefix_len = static_cast<uint32_t>(header_prefix.size());

		id<MTLBuffer> prefix_buf = [ctx_->device newBufferWithBytes:header_prefix.data()
			length:prefix_len options:MTLResourceStorageModeShared];
		id<MTLBuffer> prefix_len_buf = [ctx_->device newBufferWithBytes:&prefix_len
			length:sizeof(uint32_t) options:MTLResourceStorageModeShared];
		id<MTLBuffer> target_buf = [ctx_->device newBufferWithBytes:target_bytes.data()
			length:32 options:MTLResourceStorageModeShared];

		uint32_t result_found_init = 0;
		id<MTLBuffer> result_found_buf = [ctx_->device newBufferWithBytes:&result_found_init
			length:sizeof(uint32_t) options:MTLResourceStorageModeShared];

		uint64_t result_nonce_init = 0;
		id<MTLBuffer> result_nonce_buf = [ctx_->device newBufferWithBytes:&result_nonce_init
			length:sizeof(uint64_t) options:MTLResourceStorageModeShared];

		id<MTLBuffer> nonce_offset_buf = [ctx_->device newBufferWithLength:sizeof(uint64_t)
			options:MTLResourceStorageModeShared];

		NSUInteger max_threads = [ctx_->pipeline_state maxTotalThreadsPerThreadgroup];
		NSUInteger threadgroup_size = max_threads > 256 ? 256 : max_threads;

		MTLSize grid_size = MTLSizeMake(BATCH_SIZE, 1, 1);
		MTLSize tg_size = MTLSizeMake(threadgroup_size, 1, 1);

		uint64_t nonce_offset = 0;
		uint64_t total_hash_count = 0;

		while (!interrupt)
		{
			std::memcpy([nonce_offset_buf contents], &nonce_offset, sizeof(uint64_t));

			uint32_t zero = 0;
			std::memcpy([result_found_buf contents], &zero, sizeof(uint32_t));

			id<MTLCommandBuffer> command_buffer = [ctx_->command_queue commandBuffer];
			id<MTLComputeCommandEncoder> encoder = [command_buffer computeCommandEncoder];

			[encoder setComputePipelineState:ctx_->pipeline_state];
			[encoder setBuffer:prefix_buf offset:0 atIndex:0];
			[encoder setBuffer:prefix_len_buf offset:0 atIndex:1];
			[encoder setBuffer:target_buf offset:0 atIndex:2];
			[encoder setBuffer:result_found_buf offset:0 atIndex:3];
			[encoder setBuffer:result_nonce_buf offset:0 atIndex:4];
			[encoder setBuffer:nonce_offset_buf offset:0 atIndex:5];

			[encoder dispatchThreads:grid_size threadsPerThreadgroup:tg_size];
			[encoder endEncoding];

			[command_buffer commit];
			[command_buffer waitUntilCompleted];

			total_hash_count += BATCH_SIZE;

			uint32_t found_flag = 0;
			std::memcpy(&found_flag, [result_found_buf contents], sizeof(uint32_t));
			if (found_flag != 0)
			{
				uint64_t found_nonce = 0;
				std::memcpy(&found_nonce, [result_nonce_buf contents], sizeof(uint64_t));
				result.found = true;
				result.nonce = found_nonce;
				result.hash_count = total_hash_count;
				break;
			}

			nonce_offset += BATCH_SIZE;

			if (nonce_offset < BATCH_SIZE)
				break;
		}

		result.hash_count = total_hash_count;
	}

	return result;
}

#endif // __APPLE__

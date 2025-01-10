#pragma once
#include "../Support/WinInclude.h"
#include "../Support/ComPointer.h"
#include <stdexcept>
#include <array>

#define NUM_CMDLISTS 55
enum CommandListID {
    OBJECT_RENDER_WIRE_ID,
    OBJECT_RENDER_SOLID_ID,

    PBMPM_RENDER_ID,
    PBMPM_G2P2G_COMPUTE_ID,
    PBMPM_BUKKITCOUNT_COMPUTE_ID,
    PBMPM_BUKKITALLOCATE_COMPUTE_ID,
    PBMPM_BUKKITINSERT_COMPUTE_ID,
    PBMPM_BUFFERCLEAR_COMPUTE_ID,
    PBMPM_EMISSION_COMPUTE_ID,
    PBMPM_SET_INDIRECT_ARGS_COMPUTE_ID,

    FLUID_RENDER_ID,
    FLUID_BILEVEL_UNIFORM_GRID_COMPUTE_ID,
    FLUID_SURFACE_BLOCK_DETECTION_COMPUTE_ID,
    FLUID_SURFACE_CELL_DETECTION_COMPUTE_ID,
    FLUID_SURFACE_VERTEX_COMPACTION_COMPUTE_ID,
    FLUID_SURFACE_VERTEX_DENSITY_COMPUTE_ID,
    FLUID_SURFACE_VERTEX_NORMAL_COMPUTE_ID,
    FLUID_BUFFER_CLEAR_COMPUTE_ID,
    FLUID_MESH_ID,
    SCREEN_QUAD_RENDER_ID,

    ELASTIC_RENDER_ID,
    ELASTIC_BILEVEL_UNIFORM_GRID_COMPUTE_ID,
    ELASTIC_SURFACE_BLOCK_DETECTION_COMPUTE_ID,
    ELASTIC_SURFACE_CELL_DETECTION_COMPUTE_ID,
    ELASTIC_SURFACE_VERTEX_COMPACTION_COMPUTE_ID,
    ELASTIC_SURFACE_VERTEX_DENSITY_COMPUTE_ID,
    ELASTIC_SURFACE_VERTEX_NORMAL_COMPUTE_ID,
    ELASTIC_BUFFER_CLEAR_COMPUTE_ID,
    ELASTIC_MESH_ID,

    VISCO_RENDER_ID,
    VISCO_BILEVEL_UNIFORM_GRID_COMPUTE_ID,
    VISCO_SURFACE_BLOCK_DETECTION_COMPUTE_ID,
    VISCO_SURFACE_CELL_DETECTION_COMPUTE_ID,
    VISCO_SURFACE_VERTEX_COMPACTION_COMPUTE_ID,
    VISCO_SURFACE_VERTEX_DENSITY_COMPUTE_ID,
    VISCO_SURFACE_VERTEX_NORMAL_COMPUTE_ID,
    VISCO_BUFFER_CLEAR_COMPUTE_ID,
    VISCO_MESH_ID,

    SAND_RENDER_ID,
    SAND_BILEVEL_UNIFORM_GRID_COMPUTE_ID,
    SAND_SURFACE_BLOCK_DETECTION_COMPUTE_ID,
    SAND_SURFACE_CELL_DETECTION_COMPUTE_ID,
    SAND_SURFACE_VERTEX_COMPACTION_COMPUTE_ID,
    SAND_SURFACE_VERTEX_DENSITY_COMPUTE_ID,
    SAND_SURFACE_VERTEX_NORMAL_COMPUTE_ID,
    SAND_BUFFER_CLEAR_COMPUTE_ID,
    SAND_MESH_ID,

    SNOW_RENDER_ID,
    SNOW_BILEVEL_UNIFORM_GRID_COMPUTE_ID,
    SNOW_SURFACE_BLOCK_DETECTION_COMPUTE_ID,
    SNOW_SURFACE_CELL_DETECTION_COMPUTE_ID,
    SNOW_SURFACE_VERTEX_COMPACTION_COMPUTE_ID,
    SNOW_SURFACE_VERTEX_DENSITY_COMPUTE_ID,
    SNOW_SURFACE_VERTEX_NORMAL_COMPUTE_ID,
    SNOW_BUFFER_CLEAR_COMPUTE_ID,
    SNOW_MESH_ID
};

class DXContext
{
public:
    DXContext();
    ~DXContext();

    void signalAndWait();
    void resetCommandList(CommandListID id);
	void executeCommandList(CommandListID id);

    void flush(size_t count);
    void signalAndWaitForFence(ComPointer<ID3D12Fence>& fence, UINT64& fenceValue);

    ComPointer<IDXGIFactory7>& getFactory();
    ComPointer<ID3D12Device6>& getDevice();
    ComPointer<ID3D12CommandQueue>& getCommandQueue();
    ComPointer<ID3D12CommandAllocator>& getCommandAllocator(CommandListID id) { return cmdAllocators[id]; };
    ID3D12GraphicsCommandList6* createCommandList(CommandListID id);
    double readTimingQueryData();
    void startTimingQuery(ID3D12GraphicsCommandList6* cmdList);
    void endTimingQuery(ID3D12GraphicsCommandList6* cmdList);

private:
    void initTimingResources();

    ComPointer<ID3D12QueryHeap> queryHeap;
    ComPointer<ID3D12Resource> queryResultBuffer;

    ComPointer<IDXGIFactory7> dxgiFactory;

    ComPointer<ID3D12Device6> device;

    ComPointer<ID3D12CommandQueue> cmdQueue;
    std::array<ComPointer<ID3D12CommandAllocator>, NUM_CMDLISTS> cmdAllocators{};
    std::array<ComPointer<ID3D12GraphicsCommandList6>, NUM_CMDLISTS> cmdLists{};

    ComPointer<ID3D12Fence1> fence;
    UINT64 fenceValue = 0;
    HANDLE fenceEvent = nullptr;

};

// Support functions used in main.h and MeshShadingScene.cpp

static uint32_t packBytes(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(c) << 8) | uint32_t(d);
}

static uint32_t packValues(uint32_t packed, int index, uint8_t value) {
    // Ensure the index is valid
    if (index < 0 || index > 3) {
        throw std::invalid_argument("Index must be between 0 and 3");
    }

    // Clear the target byte in the packed value
    packed &= ~(0xFF << (index * 8));

    // Set the new value in the target byte
    packed |= (uint32_t(value) << (index * 8));

    return packed;
}
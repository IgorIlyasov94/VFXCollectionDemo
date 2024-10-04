#pragma once

#include "../DirectX12Includes.h"
#include "../Resources/IResource.h"
#include "../BufferManager.h"
#include "../Resources/ResourceManager.h"
#include "Raytracing/RaytracingShaderTable.h"
#include "Mesh.h"
#include "RaytracingObject.h"

namespace Graphics::Assets
{
	enum class RootSignatureUsing : uint32_t
	{
		GLOBAL = 0u,
		RAY_GENERATION = 1u,
		MISS = 2u,
		TRIANGLE_HIT_GROUP = 3u,
		AABB_HIT_GROUP = 4u
	};

	struct RaytracingHitGroup
	{
		LPCWSTR hitGroupName;

		LPCWSTR anyHitShaderName;
		LPCWSTR closestHitShaderName;
		LPCWSTR intersectionShaderName;
	};

	struct RaytracingLibraryDesc
	{
		uint32_t payloadStride;
		uint32_t attributeStride;
		uint32_t maxRecursionLevel;

		D3D12_SHADER_BYTECODE dxilLibrary;

		LPCWSTR rayGenerationShaderName;
		std::vector<LPCWSTR> missShaderNames;
		std::vector<RaytracingHitGroup> triangleHitGroups;
		std::vector<RaytracingHitGroup> aabbHitGroups;
	};

	struct AccelerationStructureDesc
	{
		D3D12_GPU_VIRTUAL_ADDRESS vertexBufferAddress;
		D3D12_GPU_VIRTUAL_ADDRESS indexBufferAddress;
		D3D12_GPU_VIRTUAL_ADDRESS transformsAddress;

		uint32_t vertexStride;
		uint32_t indexStride;
		uint32_t verticesNumber;
		uint32_t indicesNumber;

		bool isOpaque;
	};

	struct AccelerationStructureAABBDesc
	{
		D3D12_GPU_VIRTUAL_ADDRESS aabbBufferAddress;
		uint32_t aabbBufferSize;

		uint32_t instancesNumber;

		bool isOpaque;
	};

	class RaytracingObjectBuilder
	{
	public:
		RaytracingObjectBuilder();
		~RaytracingObjectBuilder();

		void AddAccelerationStructure(const AccelerationStructureDesc& desc);
		void AddAccelerationStructure(const AccelerationStructureAABBDesc& desc);

		void SetRootConstants(uint32_t registerIndex, uint32_t constantsNumber);
		void SetConstantBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
		void SetTexture(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor);
		void SetBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
		void SetRWTexture(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor);
		void SetRWBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
		void SetSampler(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor);

		void SetLocalRootConstants(uint32_t registerIndex, uint32_t constantsNumber,
			void* constants, RootSignatureUsing signatureUsing, uint32_t variantIndex);
		void SetLocalConstantBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
			RootSignatureUsing signatureUsing, uint32_t variantIndex);
		void SetLocalTexture(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor,
			RootSignatureUsing signatureUsing, uint32_t variantIndex);
		void SetLocalBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
			RootSignatureUsing signatureUsing, uint32_t variantIndex);
		void SetLocalRWTexture(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor,
			RootSignatureUsing signatureUsing, uint32_t variantIndex);
		void SetLocalRWBuffer(uint32_t registerIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
			RootSignatureUsing signatureUsing, uint32_t variantIndex);
		void SetLocalSampler(uint32_t registerIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor,
			RootSignatureUsing signatureUsing, uint32_t variantIndex);

		void SetShader(const RaytracingLibraryDesc& desc);

		RaytracingObject* Compose(ID3D12Device5* device, ID3D12GraphicsCommandList5* commandList,
			BufferManager* bufferManager);

		void Reset();

	private:
		RaytracingObjectBuilder(const RaytracingObjectBuilder&) = delete;
		RaytracingObjectBuilder(RaytracingObjectBuilder&&) = delete;
		RaytracingObjectBuilder& operator=(const RaytracingObjectBuilder&) = delete;
		RaytracingObjectBuilder& operator=(RaytracingObjectBuilder&&) = delete;

		struct RootSignatureVariant
		{
		public:
			uint32_t variantIndex;
			RootSignatureUsing signatureUsing;

			uint64_t GetHash()
			{
				return (static_cast<uint64_t>(variantIndex) << 32) | static_cast<uint64_t>(signatureUsing);
			}

			static RootSignatureVariant GetVariantFromHash(uint64_t hash)
			{
				return { static_cast<uint32_t>(hash >> 32), static_cast<RootSignatureUsing>(hash & std::numeric_limits<uint32_t>::max()) };
			}

			static bool Compare(uint64_t hash, uint32_t _variantIndex)
			{
				return (hash >> 32) == _variantIndex;
			}

			static bool Compare(uint64_t hash, RootSignatureUsing _signatureUsing)
			{
				return (hash & std::numeric_limits<uint32_t>::max()) == static_cast<uint64_t>(_signatureUsing);
			}
		};

		void SetDescriptorParameter(uint32_t registerIndex, D3D12_ROOT_PARAMETER_TYPE parameterType,
			D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, std::vector<DescriptorSlot>& slots,
			RootSignatureVariant signatureVariant);
		void SetDescriptorTableParameter(uint32_t registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
			D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor, std::vector<DescriptorTableSlot>& slots,
			RootSignatureVariant signatureVariant);

		D3D12_ROOT_SIGNATURE_FLAGS SetRootSignatureFlags();
		RaytracingObjectDesc SetRaytracingObjectDesc(ID3D12Device5* device, ID3D12GraphicsCommandList5* commandList,
			BufferManager* bufferManager, ID3D12StateObject* stateObject, ID3D12RootSignature* globalRootSignature,
			const std::map<uint64_t, ID3D12RootSignature*>& localRootSignatures);

		ID3D12RootSignature* CreateRootSignature(ID3D12Device5* device, D3D12_ROOT_SIGNATURE_FLAGS flags,
			D3D12_ROOT_PARAMETER* rootParameterArray, uint32_t rootParameterNumber);
		ID3D12StateObject* CreateStateObject(ID3D12Device5* device, ID3D12RootSignature* globalRootSignature,
			const std::map<uint64_t, ID3D12RootSignature*>& localRootSignatures);
		
		D3D12_STATE_SUBOBJECT CreateDXILLibrarySubobject();
		D3D12_STATE_SUBOBJECT CreateHitGroupSubobject(const RaytracingHitGroup& hitGroup, bool isTriangle);
		D3D12_STATE_SUBOBJECT CreateRaytracingConfigSubobject();

		void CreateLocalRootSignatureSubobject(ID3D12RootSignature* rootSignature, RootSignatureVariant signatureVariant,
			D3D12_STATE_SUBOBJECT& localRootSubobject, D3D12_STATE_SUBOBJECT& assotiationSubobject);

		D3D12_STATE_SUBOBJECT CreateGlobalRootSignatureSubobject(ID3D12RootSignature* rootSignature);
		D3D12_STATE_SUBOBJECT CreatePipelineConfigSubobject();

		void AddLibraryExport(const LPCWSTR& name);
		
		Graphics::Assets::Raytracing::RaytracingShaderRecord CreateShaderRecord(RootSignatureVariant signatureVariant,
			const void* shaderIdentifier);

		void CreateBottomLevelAccelerationStructure(ID3D12Device5* device, ID3D12GraphicsCommandList5* commandList,
			BufferManager* bufferManager, const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDescArray,
			BufferAllocation& bottomLevelStructure);

		void CreateTopLevelAccelerationStructure(ID3D12Device5* device,
			ID3D12GraphicsCommandList5* commandList, BufferManager* bufferManager,
			const BufferAllocation& bottomLevelStructure, const BufferAllocation& bottomLevelStructureAABB,
			BufferAllocation& topLevelStructure);

		bool HitGroupIsEmpty(const RaytracingHitGroup& hitGroup);

		RaytracingLibraryDesc raytracingLibraryDesc;
		D3D12_DXIL_LIBRARY_DESC libraryDesc;
		std::vector<D3D12_HIT_GROUP_DESC> hitGroupDescs;
		D3D12_RAYTRACING_SHADER_CONFIG raytracingConfig;
		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
		std::vector<D3D12_LOCAL_ROOT_SIGNATURE> localSignatures;
		D3D12_GLOBAL_ROOT_SIGNATURE globalSignature;
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION assotiation;
		
		uint32_t rootSignatureIndexCounter;

		std::map<uint32_t, uint32_t> rootConstantIndices;
		std::vector<DescriptorSlot> constantBufferSlots;
		std::vector<DescriptorSlot> bufferSlots;
		std::vector<DescriptorSlot> rwBufferSlots;
		std::vector<DescriptorTableSlot> textureSlots;

		enum class RootParameterOrder : uint32_t
		{
			ROOT_CONSTANTS = 0u,
			CONSTANT_BUFFER = 1u,
			BUFFER = 2u,
			RW_BUFFER = 3u,
			TEXTURE = 4u
		};

		struct LocalRootConstants
		{
		public:
			LocalRootConstants()
				: rootParameterIndex(0u)
			{

			}

			uint32_t rootParameterIndex;
			std::vector<uint32_t> data;
		};

		struct RootParameterOrderIndex
		{
		public:
			uint32_t index;
			uint32_t variantIndex;
			RootParameterOrder order;
		};

		struct LocalRootData
		{
		public:
			std::unordered_map<uint32_t, LocalRootConstants> rootConstants;
			std::vector<DescriptorSlot> constantBufferSlots;
			std::vector<DescriptorSlot> bufferSlots;
			std::vector<DescriptorSlot> rwBufferSlots;
			std::vector<DescriptorTableSlot> textureSlots;
			std::vector<RootParameterOrderIndex> localRootParametersOrder;
		};

		std::unordered_map<uint64_t, std::vector<D3D12_ROOT_PARAMETER>> rootParameters;

		std::unordered_map<uint64_t, LocalRootData> localRootData;
		
		std::vector<D3D12_EXPORT_DESC> libraryExports;
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> aabbGeometryDescs;
	};
}

#pragma once
#include "ShaderResourceBindings.h"
#include "../../../OS/IO/Path.h"


namespace Jimara {
	namespace Graphics {
		class ShaderClass : public virtual Object {
		public:
			ShaderClass(const OS::Path& shaderPath);

			const OS::Path& ShaderPath()const;

			typedef ShaderResourceBindings::ConstantBufferBinding ConstantBufferBinding;

			typedef ShaderResourceBindings::StructuredBufferBinding StructuredBufferBinding;

			typedef ShaderResourceBindings::TextureSamplerBinding TextureSamplerBinding;

			virtual inline Reference<const ConstantBufferBinding> DefaultConstantBufferBinding(const std::string& name, GraphicsDevice* device)const { return nullptr; }

			virtual inline Reference<const StructuredBufferBinding> DefaultStructuredBufferBinding(const std::string& name, GraphicsDevice* device)const { return nullptr; }
			
			virtual inline Reference<const TextureSamplerBinding> DefaultTextureSamplerBinding(const std::string& name, GraphicsDevice* device)const { return nullptr; }

			class RegistryEntry : public virtual ObjectCache<std::string>::StoredObject {
			public:
				static Reference<RegistryEntry> Create(const std::string& shaderId, Reference<const ShaderClass> shaderClass);

				~RegistryEntry();

				static RegistryEntry* Find(const std::string& shaderId);

				const std::string& ShaderId()const;

				const ShaderClass* Class()const;

			private:
				const std::string m_shaderId;
				const Reference<const ShaderClass> m_shaderClass;

				RegistryEntry(const std::string& shaderId, Reference<const ShaderClass> shaderClass);
			};


		private:
			const OS::Path m_shaderPath;
		};
	}
}

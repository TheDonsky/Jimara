#include "../GtestHeaders.h"
#include "OS/Window/Window.h"
#include "OS/Logging/StreamLogger.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "Core/Stopwatch.h"
#include "Math/Algorithms/PolygonTools.h"
#include "Math/Random.h"


namespace Jimara {
	TEST(PolygonTriangulationTest, Manual) {
		const Reference<OS::StreamLogger> logger = Object::Instantiate<OS::StreamLogger>();
		const Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>();
		const Reference<OS::Window> window = OS::Window::Create(logger, "PolygonTriangulationTest");
		ASSERT_NE(window, nullptr);
		const Reference<OS::Input> input = window->CreateInputModule();
		ASSERT_NE(input, nullptr);
		const Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, appInfo);
		ASSERT_NE(graphicsInstance, nullptr);
		const Reference<Graphics::RenderSurface> renderSurface = graphicsInstance->CreateRenderSurface(window);
		ASSERT_NE(renderSurface, nullptr);
		Graphics::PhysicalDevice* physicalDevice = renderSurface->PrefferedDevice();
		ASSERT_NE(physicalDevice, nullptr);
		const Reference<Graphics::GraphicsDevice> graphicsDevice = physicalDevice->CreateLogicalDevice();
		ASSERT_NE(graphicsDevice, nullptr);
		const Reference<Graphics::RenderEngine> renderEngine = graphicsDevice->CreateRenderEngine(renderSurface);
		ASSERT_NE(renderEngine, nullptr);
		const Reference<Graphics::ShaderLoader> shaderLoader = Graphics::ShaderDirectoryLoader::Create("Shaders/", logger);
		ASSERT_NE(shaderLoader, nullptr);
		const Reference<Graphics::ShaderSet> shaderSet = shaderLoader->LoadShaderSet("");
		ASSERT_NE(shaderSet, nullptr);
		static const Graphics::ShaderClass shaderClass(OS::Path("Jimara-Tests/Math/PolygonTriangulationTest"));
		const Reference<Graphics::SPIRV_Binary> vertexShader = shaderSet->GetShaderModule(&shaderClass, Graphics::PipelineStage::VERTEX);
		ASSERT_NE(vertexShader, nullptr);
		const Reference<Graphics::SPIRV_Binary> fragmentShader = shaderSet->GetShaderModule(&shaderClass, Graphics::PipelineStage::FRAGMENT);
		ASSERT_NE(fragmentShader, nullptr);
		
		struct VertexData {
			alignas(8) Vector2 vertPosition = {};
			alignas(16) Vector4 vertColor = {};
		};

		struct RendererData : public virtual Object {
			Reference<Graphics::GraphicsDevice> device;
			Reference<Graphics::RenderPass> renderPass;
			Stacktor<Reference<Graphics::FrameBuffer>, 4u> frameBuffers;
			Reference<Graphics::GraphicsPipeline> edgePipeline;
			Reference<Graphics::GraphicsPipeline> trianglePipeline;
			Reference<Graphics::VertexInput> triangleInput;
			Reference<Graphics::VertexInput> edgeInput;
			Reference<Graphics::VertexInput> pointerInput;
			Vector2 frameBufferSize = {};
		};

		class Renderer : public virtual Graphics::ImageRenderer {
		private:
			const Reference<OS::Input> m_input;
			const Reference<Graphics::SPIRV_Binary> m_vertexShader;
			const Reference<Graphics::SPIRV_Binary> m_fragmentShader;
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_triangleBuffer = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_edgeBuffer = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_pointerBuffer = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
			std::vector<Vector2> m_points;
			std::vector<Vector4> m_triangleColors;

			inline void UndoIfRequested() {
				if ((m_input->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL) && m_input->KeyDown(OS::Input::KeyCode::Z)) ||
					(m_input->KeyDown(OS::Input::KeyCode::LEFT_CONTROL) && m_input->KeyPressed(OS::Input::KeyCode::Z)) ||
					(m_input->KeyDown(OS::Input::KeyCode::LEFT_CONTROL) && m_input->KeyDown(OS::Input::KeyCode::Z)))
					if (m_points.size() > 0u)
						m_points.pop_back();
			}

			inline void DrawTriangles(RendererData* data, const Graphics::InFlightBufferInfo& bufferInfo) {
				const std::vector<size_t> triangles = PolygonTools::Triangulate(m_points.data(), m_points.size());
				while (m_triangleColors.size() < (triangles.size() / 3u))
					m_triangleColors.push_back(Vector4(Random::Float(), Random::Float(), Random::Float(), 0.25f));
				if (m_triangleBuffer->BoundObject() == nullptr || m_triangleBuffer->BoundObject()->ObjectCount() < triangles.size())
					m_triangleBuffer->BoundObject() = data->device->CreateArrayBuffer<VertexData>(Math::Max(triangles.size(), size_t(3u)));
				VertexData* vertPtr = (VertexData*)m_triangleBuffer->BoundObject()->Map();
				for (size_t i = 0u; i < triangles.size(); i += 3) {
					auto add = [&](size_t index) {
						vertPtr->vertPosition = m_points[triangles[index]];
						vertPtr->vertColor = m_triangleColors[index / 3u];
						vertPtr++;
					};
					add(i + 2u);
					add(i + 1u);
					add(i);
				}
				m_triangleBuffer->BoundObject()->Unmap(true);
				data->triangleInput->Bind(bufferInfo);
				data->trianglePipeline->Draw(bufferInfo, triangles.size(), 1u);
			}

			inline void DrawEdges(RendererData* data, const Graphics::InFlightBufferInfo& bufferInfo) const {
				if (m_edgeBuffer->BoundObject() == nullptr || m_edgeBuffer->BoundObject()->ObjectCount() < (m_points.size() * 2u))
					m_edgeBuffer->BoundObject() = data->device->CreateArrayBuffer<VertexData>(m_points.size() * 2u + 1u);
				VertexData* verts = (VertexData*)m_edgeBuffer->BoundObject()->Map();
				for (size_t i = 0u; i < m_points.size(); i++) {
					VertexData& a = verts[i << 1u];
					VertexData& b = verts[(i << 1u) + 1u];
					a.vertPosition = m_points[i];
					b.vertPosition = m_points[(i + 1u) % m_points.size()];
					a.vertColor = b.vertColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
				}
				m_edgeBuffer->BoundObject()->Unmap(true);
				data->edgeInput->Bind(bufferInfo);
				data->edgePipeline->Draw(bufferInfo, m_points.size() * 2u, 1u);
			}

			inline void DrawPointer(RendererData* data, const Graphics::InFlightBufferInfo& bufferInfo) {
				if (m_pointerBuffer->BoundObject() == nullptr)
					m_pointerBuffer->BoundObject() = data->device->CreateArrayBuffer<VertexData>(6u);
				const Vector2 rawPosition(
					m_input->GetAxis(OS::Input::Axis::MOUSE_POSITION_X), 
					m_input->GetAxis(OS::Input::Axis::MOUSE_POSITION_Y));
				const Vector2 snappedPosition =
					(!m_input->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)) ? rawPosition :
					Vector2(uint32_t(rawPosition.x / 16.0f) * 16.0f, uint32_t(rawPosition.y / 16.0f) * 16.0f);
				const Vector2 pointerPosition = (snappedPosition - (data->frameBufferSize * 0.5f)) / data->frameBufferSize * Vector2(2.0f, -2.0f);
				const Vector2 pointerRadius = Vector2(16.0f) / data->frameBufferSize;
				VertexData* vertPtr = (VertexData*)m_pointerBuffer->BoundObject()->Map();
				auto addVert = [&](Vector2 offset) {
					vertPtr->vertPosition = pointerPosition + offset;
					vertPtr->vertColor = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
					vertPtr++;
				};
				addVert(Vector2(-pointerRadius.x, -pointerRadius.y));
				addVert(Vector2(pointerRadius.x, pointerRadius.y));
				addVert(Vector2(-pointerRadius.x, pointerRadius.y));
				addVert(Vector2(-pointerRadius.x, -pointerRadius.y));
				addVert(Vector2(pointerRadius.x, -pointerRadius.y));
				addVert(Vector2(pointerRadius.x, pointerRadius.y));
				m_pointerBuffer->BoundObject()->Unmap(true);
				data->pointerInput->Bind(bufferInfo);
				data->trianglePipeline->Draw(bufferInfo, m_pointerBuffer->BoundObject()->ObjectCount(), 1u);
				if (pointerPosition.x > -1.0f && pointerPosition.x < 1.0f &&
					pointerPosition.y > -1.0f && pointerPosition.y < 1.0f &&
					m_input->KeyUp(OS::Input::KeyCode::MOUSE_LEFT_BUTTON))
					m_points.push_back(pointerPosition);
			}

		public:
			inline Renderer(
				OS::Input* input,
				Graphics::SPIRV_Binary* vertexShader, Graphics::SPIRV_Binary* fragmentShader)
				: m_input(input)
				, m_vertexShader(vertexShader), m_fragmentShader(fragmentShader) {}

			inline virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) {
				Reference<RendererData> data = Object::Instantiate<RendererData>();
				data->device = engineInfo->Device();
				Graphics::Texture::PixelFormat format = engineInfo->ImageFormat();
				data->renderPass = data->device->GetRenderPass(
					Graphics::Texture::Multisampling::SAMPLE_COUNT_1, 1u, &format, 
					Graphics::Texture::PixelFormat::OTHER, Graphics::RenderPass::Flags::CLEAR_COLOR);
				if (data->renderPass == nullptr)
					return nullptr;
				for (size_t i = 0u; i < engineInfo->ImageCount(); i++) {
					Reference<Graphics::TextureView> view = engineInfo->Image(i)->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
					if (view == nullptr)
						return nullptr;
					const Reference<Graphics::FrameBuffer> frameBuffer = data->renderPass->CreateFrameBuffer(&view, nullptr, nullptr, nullptr);
					if (frameBuffer == nullptr)
						return nullptr;
					data->frameBuffers.Push(frameBuffer);
				}
				Graphics::GraphicsPipeline::Descriptor desc = {};
				{
					desc.blendMode = Graphics::GraphicsPipeline::BlendMode::ALPHA_BLEND;
					desc.flags = Graphics::GraphicsPipeline::Flags::DEFAULT;
					desc.vertexShader = m_vertexShader;
					desc.fragmentShader = m_fragmentShader;
					desc.indexType = Graphics::GraphicsPipeline::IndexType::EDGE;
				}
				{
					desc.vertexInput.Resize(1u);
					Graphics::GraphicsPipeline::VertexInputInfo& info = desc.vertexInput[0];
					info.bufferElementSize = sizeof(VertexData);
					info.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX;
					info.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo("vertPosition", offsetof(VertexData, vertPosition)));
					info.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo("vertColor", offsetof(VertexData, vertColor)));
				}
				data->edgePipeline = data->renderPass->GetGraphicsPipeline(desc);
				if (data->edgePipeline == nullptr)
					return nullptr;
				desc.indexType = Graphics::GraphicsPipeline::IndexType::TRIANGLE;
				data->trianglePipeline = data->renderPass->GetGraphicsPipeline(desc);
				if (data->trianglePipeline == nullptr)
					return nullptr;
				{
					auto buff = m_triangleBuffer.operator->();
					data->triangleInput = data->trianglePipeline->CreateVertexInput(&buff, nullptr);
					if (data->triangleInput == nullptr)
						return nullptr;
				}
				{
					auto buff = m_edgeBuffer.operator->();
					data->edgeInput = data->edgePipeline->CreateVertexInput(&buff, nullptr);
					if (data->edgeInput == nullptr)
						return nullptr;
				}
				{
					auto buff = m_pointerBuffer.operator->();
					data->pointerInput = data->trianglePipeline->CreateVertexInput(&buff, nullptr);
					if (data->pointerInput == nullptr)
						return nullptr;
				}
				data->frameBufferSize = engineInfo->ImageSize();
				return data;
			}

			inline virtual void Render(Object* engineData, Graphics::InFlightBufferInfo bufferInfo) {
				RendererData* data = dynamic_cast<RendererData*>(engineData);
				if (data == nullptr)
					return;
				const Vector4 clearColor = Vector4(0.25f, 0.25f, 0.25f, 1.0f);
				data->renderPass->BeginPass(bufferInfo, data->frameBuffers[bufferInfo], &clearColor);
				UndoIfRequested();
				DrawTriangles(data, bufferInfo);
				DrawEdges(data, bufferInfo);
				DrawPointer(data, bufferInfo);
				data->renderPass->EndPass(bufferInfo);
			}
		};

		Stopwatch frameTimer;
		auto onWindowUpdate = [&](auto* wnd) { 
			input->Update(frameTimer.Reset());
			renderEngine->Update(); 
		};
		window->OnUpdate() += Callback<OS::Window*>::FromCall(&onWindowUpdate);

		const Reference<Renderer> renderer = Object::Instantiate<Renderer>(input, vertexShader, fragmentShader);
		renderEngine->AddRenderer(renderer);
		const constexpr float timeOut = 5.0f;
		Stopwatch timer;
		const Size2 initialWindowSize = window->FrameBufferSize();
		while (true) {
			const float elapsed = timer.Elapsed();
			if (elapsed >= timeOut || window->Closed())
				break;
			if (initialWindowSize != window->FrameBufferSize()) {
				window->SetName("PolygonTriangulationTest");
				window->WaitTillClosed();
			}
			else {
				std::stringstream stream;
				stream << "PolygonTriangulationTest [Closing in " << (float(int((timeOut - elapsed) * 4.0f)) / 4.0f) << " seconds unless the window gets resized]";
				window->SetName(stream.str());
			}
		}
		window->OnUpdate() -= Callback<OS::Window*>::FromCall(&onWindowUpdate);
	}
}

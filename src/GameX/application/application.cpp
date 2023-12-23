#include "GameX/application/application.h"

#include "GameX/renderer/renderer.h"

namespace GameX {
Application::Application(const GameX::ApplicationSettings &settings)
    : settings_(settings) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  if (settings_.width == -1 || settings_.height == -1) {
    if (settings_.fullscreen) {
      // Get the primary monitor
      GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();

      // Get the video mode of the primary monitor
      const GLFWvidmode *mode = glfwGetVideoMode(primaryMonitor);

      settings_.width = mode->width;
      settings_.height = mode->height;
    } else {
      settings_.width = 1280;
      settings_.height = 720;
    }
  }

  if (settings_.fullscreen) {
    // Get the primary monitor
    GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();

    // Get the video mode of the primary monitor
    const GLFWvidmode *mode = glfwGetVideoMode(primaryMonitor);

    // Set the window to be full screen borderless window
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    // Create a borderless windowed mode window
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  }

  window_ = glfwCreateWindow(settings_.width, settings_.height, "GameX",
                             nullptr, nullptr);

  if (settings_.fullscreen) {
    glfwSetWindowPos(window_, 0, 0);
  }

  grassland::vulkan::CoreSettings core_settings;
  core_settings.window = window_;

  core_ = std::make_unique<grassland::vulkan::Core>(core_settings);

  renderer_ = std::make_unique<class Renderer>(this);
}

Application::~Application() {
  renderer_.reset();
  core_.reset();
  glfwDestroyWindow(window_);
  glfwTerminate();
}

void Application::Run() {
  Init();

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();
    Update();
    Render();
  }

  core_->Device()->WaitIdle();
  Cleanup();
}

void Application::Init() {
  CreateCube();
}

void Application::Update() {
}

void Application::Render() {
  renderer_->SyncObjects();
  core_->BeginFrame();

  auto cmd_buffer = core_->CommandBuffer();

  auto frame_image = core_->SwapChain()->Images()[core_->ImageIndex()];

  // Transition frame_image_ to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
  grassland::vulkan::TransitImageLayout(
      cmd_buffer->Handle(), frame_image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_READ_BIT,
      VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

  // Clear frame_image_ to black
  VkClearColorValue clear_color{0.6f, 0.7f, 0.8f, 1.0f};
  VkImageSubresourceRange subresource_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
                                            1};

  vkCmdClearColorImage(cmd_buffer->Handle(), frame_image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1,
                       &subresource_range);

  // Transition frame_image_ to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  grassland::vulkan::TransitImageLayout(
      cmd_buffer->Handle(), frame_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

  core_->EndFrame();
}

void Application::Cleanup() {
  static_cube_.reset();
  dynamic_cube_.reset();
}

void Application::CreateCube() {
  std::vector<Vertex> vertices = {
      Vertex{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}},
      Vertex{{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
      Vertex{{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
      Vertex{{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
      Vertex{{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
      Vertex{{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
      Vertex{{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}},
      Vertex{{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
  };

  std::vector<uint32_t> indices = {
      0, 2, 1, 1, 2, 3, 4, 5, 6, 5, 7, 6, 0, 1, 5, 0, 5, 4,
      2, 6, 7, 2, 7, 3, 0, 4, 6, 0, 6, 2, 1, 3, 7, 1, 7, 5,
  };

  cube_ = Mesh(vertices, indices);

  static_cube_ = std::make_unique<StaticModel>(Renderer(), cube_);
  dynamic_cube_ = std::make_unique<DynamicModel>(Renderer(), &cube_);
}
}  // namespace GameX
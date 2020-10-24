// Copyright(c) 2016 Ruoyu Fan (Windy Darian), Xueyin Wan
// MIT License.

// We are mixing up Vulkan C binding (vulkan.h) and C++ binding
//  (vulkan.hpp), because we are trying to use C++ binding for
// new codes; cleaning up will be done at some point

#include "VulkanRenderer.h"

#include "../scene.h"
#include "model.h"
#include "raii.h"
#include "../util.h"
#include "vulkan_util.h"
#include "context.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <functional>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <fstream>
#include <chrono>

using util::Vertex;

const int MAX_POINT_LIGHT_COUNT = 20000; //TODO: change it back smaller
//const int MAX_POINT_LIGHT_PER_TILE = 63;
const int MAX_POINT_LIGHT_PER_TILE = 1023;
// const int TILE_SIZE = 16;
const int TILE_SIZE = 16;

struct PointLight
{
public:
    //glm::vec3 pos = { 0.0f, 1.0f, 0.0f };
    glm::vec3 pos;
    float radius = { 5.0f };
    glm::vec3 intensity = { 1.0f, 1.0f, 1.0f };
    float padding;

    PointLight() {}
    PointLight(glm::vec3 pos, float radius, glm::vec3 intensity)
        : pos(pos), radius(radius), intensity(intensity)
    {};
};


// uniform buffer object for model transformation
struct SceneObjectUbo
{
    glm::mat4 model;
};

// uniform buffer object for camera
struct CameraUbo
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 projview;
    glm::vec3 cam_pos;
};

struct PushConstantObject
{
    glm::ivec2 viewport_size;
    glm::ivec2 tile_nums;
    int debugview_index; // TODO: separate this and only have it in debug mode?

    PushConstantObject(int viewport_size_x, int viewport_size_y, int tile_num_x, int tile_num_y, int debugview_index = 0)
        : viewport_size(viewport_size_x, viewport_size_y),
        tile_nums(tile_num_x, tile_num_y),
        debugview_index(debugview_index)
    {}
};



class _VulkanRenderer_Impl
{
public:
    _VulkanRenderer_Impl(GLFWwindow* window);

    void resize(int width, int height);
    void requestDraw(float deltatime);
    void cleanUp();

    void setCamera(const glm::mat4 & view, const glm::vec3 campos);


    int getDebugViewIndex() const
    {
        return debug_view_index;
    }

    /**
    *  0: render 1: heat map with render 2: heat map 3: depth 4: normal
    */
    void changeDebugViewIndex(int target_view)
    {
        debug_view_index = target_view % 5;
        recreateSwapChain(); // TODO: change this to a state modification and handle the recreation before update
    }

private:

    VContext vulkan_context;

    VUtility utility {vulkan_context};

    // TODO: remove those
    QueueFamilyIndices queue_family_indices;
    VkPhysicalDevice physical_device;
    VkDevice graphics_device;
    vk::Device device;
    vk::Queue graphics_queue;
    vk::Queue present_queue;
    vk::Queue compute_queue;
    vk::CommandPool graphics_command_pool;
    vk::CommandPool compute_command_pool;

    VRaii<vk::SwapchainKHR> swap_chain;
    std::vector<VkImage> swap_chain_images;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    std::vector<VRaii<vk::ImageView>> swap_chain_imageviews;
    std::vector<VRaii<vk::Framebuffer>> swap_chain_framebuffers;
    VRaii<vk::Framebuffer> depth_pre_pass_framebuffer;

    VRaii<vk::RenderPass> render_pass;
    VRaii<vk::RenderPass> depth_pre_pass; // the depth prepass which happens before formal render pass

    VRaii<vk::DescriptorSetLayout> object_descriptor_set_layout;
    VRaii<vk::DescriptorSetLayout> camera_descriptor_set_layout;
    VRaii<vk::DescriptorSetLayout> material_descriptor_set_layout;
    VRaii<VkPipelineLayout> pipeline_layout;
    VRaii<VkPipeline> graphics_pipeline;
    VRaii<vk::PipelineLayout> depth_pipeline_layout;
    VRaii<vk::Pipeline> depth_pipeline;

    VRaii<vk::DescriptorSetLayout> light_culling_descriptor_set_layout;  // shared between compute queue and graphics queue
    VRaii<vk::DescriptorSetLayout> intermediate_descriptor_set_layout; // which is exclusive to compute queue
    VRaii<VkPipelineLayout> compute_pipeline_layout;
    VRaii<VkPipeline> compute_pipeline;
    vk::CommandBuffer light_culling_command_buffer = {};
    //VRaii<vk::PipelineLayout> compute_pipeline_layout;
    //VRaii<vk::Pipeline> compute_pipeline;

    std::vector<VkCommandBuffer> command_buffers; // buffers will be released when pool destroyed
    vk::CommandBuffer depth_prepass_command_buffer;

    VRaii<vk::Semaphore> image_available_semaphore;
    VRaii<vk::Semaphore> render_finished_semaphore;
    VRaii<vk::Semaphore> lightculling_completed_semaphore;
    VRaii<vk::Semaphore> depth_prepass_finished_semaphore;

    // for depth
    VRaii<VkImage> depth_image;
    VRaii<VkDeviceMemory> depth_image_memory;
    VRaii<VkImageView> depth_image_view;

    // texture image
    VRaii<VkImage> texture_image;
    VRaii<VkDeviceMemory> texture_image_memory;
    VRaii<VkImageView> texture_image_view;
    VRaii<VkImage> normalmap_image;
    VRaii<VkDeviceMemory> normalmap_image_memory;
    VRaii<VkImageView> normalmap_image_view;
    VRaii<VkSampler> texture_sampler;

    // uniform buffers
    VRaii<VkBuffer> object_staging_buffer;
    VRaii<VkDeviceMemory> object_staging_buffer_memory;
    VRaii<VkBuffer> object_uniform_buffer;
    VRaii<VkDeviceMemory> object_uniform_buffer_memory;
    VRaii<VkBuffer> camera_staging_buffer;
    VRaii<VkDeviceMemory> camera_staging_buffer_memory;
    VRaii<VkBuffer> camera_uniform_buffer;
    VRaii<VkDeviceMemory> camera_uniform_buffer_memory;

    VRaii<VkDescriptorPool> descriptor_pool;
    VkDescriptorSet object_descriptor_set;
    vk::DescriptorSet camera_descriptor_set;
    VkDescriptorSet light_culling_descriptor_set;
    vk::DescriptorSet intermediate_descriptor_set;

    // vertex buffer
    VModel model;
    //VRaii<VkBuffer> vertex_buffer;
    //VRaii<VkDeviceMemory> vertex_buffer_memory;
    //VRaii<VkBuffer> index_buffer;
    //VRaii<VkDeviceMemory> index_buffer_memory;

    VRaii<VkBuffer> pointlight_buffer;
    VRaii<VkDeviceMemory> pointlight_buffer_memory;
    VRaii<VkBuffer> lights_staging_buffer;
    VRaii<VkDeviceMemory> lights_staging_buffer_memory;
    VkDeviceSize pointlight_buffer_size;

    std::vector<util::Vertex> vertices;
    std::vector<uint32_t> vertex_indices;

    std::vector<PointLight> pointlights;

    // This storage buffer stores visible lights for each tile
    // which is output from the light culling compute shader
    // max MAX_POINT_LIGHT_PER_TILE point lights per tile
    VRaii<VkBuffer> light_visibility_buffer;
    VRaii<VkDeviceMemory> light_visibility_buffer_memory;
    VkDeviceSize light_visibility_buffer_size = 0;

    int window_framebuffer_width;
    int window_framebuffer_height;

    glm::mat4 view_matrix;
    glm::vec3 cam_pos;
    int tile_count_per_row;
    int tile_count_per_col;
    int debug_view_index = 0;

    void initialize()
    {
        util::writeLog("Init vulkan render.");
        createSwapChain();
        createSwapChainImageViews();
        createRenderPasses();
        createDescriptorSetLayouts();
        createGraphicsPipelines();
        createComputePipeline();
        createDepthResources();
        createFrameBuffers();
        createTextureSampler();
        createUniformBuffers();
        createLights();
        createDescriptorPool();
        model = VModel::loadModelFromFile(vulkan_context, getGlobalTestSceneConfiguration().model_file, texture_sampler.get(), descriptor_pool.get(), material_descriptor_set_layout.get());
        createSceneObjectDescriptorSet();
        createCameraDescriptorSet();
        createIntermediateDescriptorSet();
        updateIntermediateDescriptorSet();
        createLigutCullingDescriptorSet();
        createLightVisibilityBuffer(); // create a light visiblity buffer and update descriptor sets, need to rerun after changing size
        createGraphicsCommandBuffers();
        createLightCullingCommandBuffer();
        createDepthPrePassCommandBuffer();
        createSemaphores();
    }

    void recreateSwapChain()
    {
        util::writeLog("====================begin reset swapchain====================");
        vkDeviceWaitIdle(graphics_device);

        createSwapChain();
        createSwapChainImageViews();
        createRenderPasses();
        createGraphicsPipelines();
        createDepthResources();
        createFrameBuffers();
        createLightVisibilityBuffer(); // since it's size will scale with window;
        updateIntermediateDescriptorSet();
        createGraphicsCommandBuffers();
        createLightCullingCommandBuffer(); // it needs light_visibility_buffer_size, which is changed on resize
        createDepthPrePassCommandBuffer();
        util::writeLog("====================end reset swapchain====================\n");
    }

    void createSwapChain();
    void createSwapChainImageViews();
    void createRenderPasses();
    void createDescriptorSetLayouts();
    void createGraphicsPipelines();
    void createDepthResources();
    void createFrameBuffers();
    void createTextureSampler();
    void createUniformBuffers();
    void createLights();
    void createDescriptorPool();
    void createSceneObjectDescriptorSet();
    void createCameraDescriptorSet();
    void createIntermediateDescriptorSet();
    void updateIntermediateDescriptorSet();
    void createGraphicsCommandBuffers();
    void createSemaphores();

    void createComputePipeline();
    void createLigutCullingDescriptorSet();
    void createLightVisibilityBuffer();
    void createLightCullingCommandBuffer();

    void createDepthPrePassCommandBuffer();

    void updateUniformBuffers(float deltatime);
    void drawFrame();

    VRaii<VkShaderModule> createShaderModule(const std::vector<char>& code);
};



_VulkanRenderer_Impl::_VulkanRenderer_Impl(GLFWwindow* window)
    :vulkan_context(window)
{

    queue_family_indices = vulkan_context.getQueueFamilyIndices();
    physical_device = vulkan_context.getPhysicalDevice();
    graphics_device = vulkan_context.getDevice();
    device = vulkan_context.getDevice();
    graphics_queue = vulkan_context.getGraphicsQueue();
    present_queue = vulkan_context.getPresentQueue();
    compute_queue = vulkan_context.getComputeQueue();
    graphics_command_pool = vulkan_context.getGraphicsCommandPool();
    compute_command_pool = vulkan_context.getComputeCommandPool();

    std::tie(window_framebuffer_width, window_framebuffer_height) = vulkan_context.getWindowFrameBufferSize();

    initialize(); //TODO: allow multiple calls to initialize().... currently I am having problems with something like command buffers
}

void _VulkanRenderer_Impl::resize(int width, int height)
{
    if (width == 0 || height == 0) return;

    std::tie(window_framebuffer_width, window_framebuffer_height) = vulkan_context.getWindowFrameBufferSize();

    recreateSwapChain();
}

void _VulkanRenderer_Impl::requestDraw(float deltatime)
{
    updateUniformBuffers(deltatime); // TODO: there is graphics queue waiting in utility.copyBuffer() called by this so I don't need to sync CPU and GPU elsewhere... but someday I will make the copy command able to use multiple times and I need to sync on writing the staging buffer
    drawFrame();
}

void _VulkanRenderer_Impl::cleanUp()
{
    vkDeviceWaitIdle(graphics_device);
}

void _VulkanRenderer_Impl::setCamera(const glm::mat4 & view, const glm::vec3 campos)
{
    view_matrix = view;
    this->cam_pos = campos;
}

void _VulkanRenderer_Impl::createSwapChain()
{
    auto support_details = SwapChainSupportDetails::querySwapChainSupport(physical_device, vulkan_context.getWindowSurface());

    VkSurfaceFormatKHR surface_format = utility.chooseSwapSurfaceFormat(support_details.formats);
    VkPresentModeKHR present_mode = utility.chooseSwapPresentMode(support_details.present_modes);
    VkExtent2D extent = utility.chooseSwapExtent(support_details.capabilities);

    uint32_t queue_length = support_details.capabilities.minImageCount + 1;
    if (support_details.capabilities.maxImageCount > 0 && queue_length > support_details.capabilities.maxImageCount)
    {
        // 0 for maxImageCount means no limit
        queue_length = support_details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = vulkan_context.getWindowSurface();
    create_info.minImageCount = queue_length;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1; // >1 when developing stereoscopic application
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // render directly
    // VK_IMAGE_USAGE_TRANSFER_DST_BIT and memory operation to enable post processing

    QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(physical_device, vulkan_context.getWindowSurface());
    uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphics_family, (uint32_t)indices.present_family };

    if (indices.graphics_family != indices.present_family)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0; // Optional
        create_info.pQueueFamilyIndices = nullptr; // Optional
    }

    create_info.preTransform = support_details.capabilities.currentTransform; // not doing any transformation
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore alpha channel (for blending with other windows)

    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE; // ignore pixels obscured

    auto old_swap_chain = std::move(swap_chain); //which will be destroyed when out of scope
    create_info.oldSwapchain = old_swap_chain.get(); // required when recreating a swap chain (like resizing windows)

    VkSwapchainKHR tmp_swap_chain;
    vulkan_util::checkResult(vkCreateSwapchainKHR(graphics_device, &create_info, nullptr, &tmp_swap_chain), "Failed to create swap chain!");

    swap_chain = VRaii<vk::SwapchainKHR>(
            tmp_swap_chain,
            [device = this->device](auto & obj){device.destroySwapchainKHR(obj); }
    );

    uint32_t image_count;
    vkGetSwapchainImagesKHR(graphics_device, swap_chain.get(), &image_count, nullptr);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(graphics_device, swap_chain.get(), &image_count, swap_chain_images.data());

    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extent;

    util::writeLog("create swapchain :  %d images size[%dx%d].", image_count,extent.width,extent.height);
}

void _VulkanRenderer_Impl::createSwapChainImageViews()
{
    auto raii_deleter = [device = this->device](auto& obj)
    {
        device.destroyImageView(obj);
    };

    swap_chain_imageviews.clear(); // VRaii will delete old objects
    swap_chain_imageviews.reserve(swap_chain_images.size());

    for (uint32_t i = 0; i < swap_chain_images.size(); i++)
    {
        VkImageView tmp_imageview;
        utility.createImageView(swap_chain_images[i], swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, &tmp_imageview);
        swap_chain_imageviews.emplace_back(tmp_imageview, raii_deleter);
        util::writeLog("create image view for image %d in swapchain.", i);
    }
}

void _VulkanRenderer_Impl::createRenderPasses()
{
    auto renderpass_deletef = [device = this->device](auto & obj)
    {
        device.destroyRenderPass(obj);
    };

    // depth pre pass // TODO: should I merge this as a
    {
        VkAttachmentDescription depth_attachment = {};
        depth_attachment.format = utility.findDepthFormat();
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //TODO?
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        //depth_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // to be read in compute shader?
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;  // to be read in compute shader?


        VkAttachmentReference depth_attachment_ref = {};
        depth_attachment_ref.attachment = 0;
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pDepthStencilAttachment = &depth_attachment_ref;

        // overwrite subpass dependency to make it wait until VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0; // 0  refers to the subpass
        dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 1> attachments = {  depth_attachment };

        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = (uint32_t)attachments.size();
        render_pass_info.pAttachments = attachments.data();
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &dependency;

        VkRenderPass pass;
        if (vkCreateRenderPass(graphics_device, &render_pass_info, nullptr, &pass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create depth pre-pass!");
        }
        depth_pre_pass = VRaii<vk::RenderPass>(pass,renderpass_deletef);

        util::writeLog("create render pass --> early depth pass: \n"
            "\t depth attachment{format, samples, load/store action , initial and fianal layout.} count = 1\n"
            "\t subpass(VK_PIPELINE_BIND_POINT_GRAPHICS), count = 1\n"
            "\t subpass dependency, count=1\n");

    }
    // the render pass
    {
        VkAttachmentDescription color_attachment = {};
        color_attachment.format = swap_chain_image_format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // before rendering
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // after rendering
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // no stencil
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // image layout when pass begins
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // image layout when pass ends

        VkAttachmentDescription depth_attachment = {};
        depth_attachment.format = utility.findDepthFormat();
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_attachment_ref = {};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_attachment_ref = {};
        depth_attachment_ref.attachment = 1;
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;
        subpass.pDepthStencilAttachment = &depth_attachment_ref;

        // overwrite subpass dependency to make it wait until VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0; // 0  refers to the subpass
        dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = (uint32_t)attachments.size();
        render_pass_info.pAttachments = attachments.data();
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &dependency;

        VkRenderPass tmp_render_pass;
        if (vkCreateRenderPass(graphics_device, &render_pass_info, nullptr, &tmp_render_pass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
        render_pass = VRaii<vk::RenderPass>(tmp_render_pass, renderpass_deletef);

        util::writeLog("create render pass --> main pass: \n"
            "\t depth & color attachment{format, samples, load/store action, initial and fianal layout.} count = 2\n"
            "\t subpass(VK_PIPELINE_BIND_POINT_GRAPHICS), count = 1\n"
            "\t subpass dependency, count=1\n");
    }
}

void _VulkanRenderer_Impl::createDescriptorSetLayouts()
{
    auto raii_layout_deleter = [device = this->device](auto & layout)
    {
        device.destroyDescriptorSetLayout(layout);
    };

    {
        // Transform information for each object in scene
        VkDescriptorSetLayoutBinding ubo_layout_binding = {};
        ubo_layout_binding.binding = 0;
        ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_layout_binding.descriptorCount = 1;
        ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // only referencing from vertex shader
        ubo_layout_binding.pImmutableSamplers = nullptr; // Optional


        std::array<VkDescriptorSetLayoutBinding, 1> bindings = { ubo_layout_binding};
        VkDescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = (uint32_t)bindings.size();
        layout_info.pBindings = bindings.data();


        object_descriptor_set_layout = VRaii<vk::DescriptorSetLayout>(
            device.createDescriptorSetLayout(layout_info, nullptr),
            raii_layout_deleter
        );
        util::writeLog("create descriptor set layout: scene object\n"
            "\t binding:\n"
            "\t\t {uniform buffer,\n"
            "\t\t descriptor count=1\n"
            "\t\t vertex|fragment\n"
            "\t\t binding=0},\n"
        );
    }

    // camera_descriptor_set_layout
    {
        vk::DescriptorSetLayoutBinding ubo_layout_binding = {
            0,                                   // binding
            vk::DescriptorType::eStorageBuffer,  // descriptorType // FIXME: change back to uniform
            1,                                   // descriptorCount
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute, // stagFlags
            nullptr,                             // pImmutableSamplers
        };

        vk::DescriptorSetLayoutCreateInfo create_info = {
            vk::DescriptorSetLayoutCreateFlags(), // flags
            1,
            &ubo_layout_binding,
        };

        camera_descriptor_set_layout = VRaii<vk::DescriptorSetLayout>(
            device.createDescriptorSetLayout(create_info, nullptr),
            raii_layout_deleter
        );
        util::writeLog("create descriptor set layout: camera object\n"
            "\t binding:\n"
            "\t\t {uniform buffer,\n"
            "\t\t descriptor count=1\n"
            "\t\t vertex|fragment|compute\n"
            "\t\t binding=0},\n"
        );
    }

    // light_culling_descriptor_set_layout, shared between compute pipeline and graphics pipeline
    {
        std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {};

        {
            // create descriptor for storage buffer for light culling results
            VkDescriptorSetLayoutBinding lb = {};
            lb.binding = 0;
            lb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            lb.descriptorCount = 1;
            lb.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            lb.pImmutableSamplers = nullptr;
            set_layout_bindings.push_back(lb);
        }

        {
            // uniform buffer for point lights
            VkDescriptorSetLayoutBinding lb = {};
            lb.binding = 1;
            lb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; // FIXME: change back to uniform
            lb.descriptorCount = 1;  // maybe we can use this for different types of lights
            lb.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            lb.pImmutableSamplers = nullptr;
            set_layout_bindings.push_back(lb);
        }

        VkDescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(set_layout_bindings.size());
        layout_info.pBindings = set_layout_bindings.data();

        light_culling_descriptor_set_layout = VRaii<vk::DescriptorSetLayout>(
            device.createDescriptorSetLayout(layout_info, nullptr),
            raii_layout_deleter
        );

        util::writeLog("create descriptor set layout: light culling parameters\n"
            "\t binding:\n"
            "\t\t {storage buffer,\n"
            "\t\t descriptor count=1\n"
            "\t\t compute|fragment\n"
            "\t\t binding=0},\n"
            "\t binding:\n"
            "\t\t {storage buffer,\n"
            "\t\t descriptor count=1\n"
            "\t\t compute|fragment\n"
            "\t\t binding=1},\n"
        );
    }

    // descriptor set layout for intermediate objects during render passes, such as z-buffer
    {
        // reads from depth attachment of previous frame
        // descriptor for texture sampler
        vk::DescriptorSetLayoutBinding sampler_layout_binding = {
            0,                                         // binding
            vk::DescriptorType::eCombinedImageSampler, // descriptorType
            1,                                         // descriptoCount
            vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eFragment ,  //stageFlags
            nullptr,                                   // pImmutableSamplers
        };

        vk::DescriptorSetLayoutCreateInfo create_info = {
            vk::DescriptorSetLayoutCreateFlags(), // flags
            1,
            &sampler_layout_binding,
        };

        intermediate_descriptor_set_layout = VRaii<vk::DescriptorSetLayout>(
            device.createDescriptorSetLayout(create_info, nullptr),
            raii_layout_deleter
        );

        util::writeLog("create descriptor set layout: depth map in previous frame\n"
            "\t binding:\n"
            "\t\t {combined image sampler,\n"
            "\t\t descriptor count=1\n"
            "\t\t compute|fragment\n"
            "\t\t binding=0},\n"
        );
    }

    // material_descriptror_layout // TODO: maybe I still need to do for each instance
    {
        // reads from depth attachment of previous frame
        // descriptor for texture sampler

        vk::DescriptorSetLayoutBinding uniform_layout_binding = {
            0, // binding
            vk::DescriptorType::eUniformBuffer, // descriptorType
            1, // descriptorCount
            vk::ShaderStageFlagBits::eFragment ,  //stageFlags
            nullptr, // pImmutableSamplers
        };

        vk::DescriptorSetLayoutBinding albedo_map_layout_binding = {
            1, // binding
            vk::DescriptorType::eCombinedImageSampler, // descriptorType
            1, // descriptorCount
            vk::ShaderStageFlagBits::eFragment ,  //stageFlags
            nullptr, // pImmutableSamplers
        };

        vk::DescriptorSetLayoutBinding normap_layout_binding = {
            2, // binding
            vk::DescriptorType::eCombinedImageSampler, // descriptorType
            1, // descriptorCount
            vk::ShaderStageFlagBits::eFragment ,  //stageFlags
            nullptr, // pImmutableSamplers
        };

        std::array<vk::DescriptorSetLayoutBinding, 3> bindings = { uniform_layout_binding, albedo_map_layout_binding , normap_layout_binding };

        vk::DescriptorSetLayoutCreateInfo create_info = {
            vk::DescriptorSetLayoutCreateFlags(), // flags
            static_cast<uint32_t>(bindings.size()),
            bindings.data()
        };

        material_descriptor_set_layout = VRaii<vk::DescriptorSetLayout>(
            device.createDescriptorSetLayout(create_info, nullptr),
            raii_layout_deleter
        );

        util::writeLog("create descriptor set layout: material\n"
            "\t binding:\n"
            "\t\t {uniform buffer,\n"
            "\t\t descriptor count=1\n"
            "\t\t fragment stage\n"
            "\t\t binding=0},\n"
            "\t binding:\n"
            "\t\t {combined image sampler,\n"
            "\t\t descriptor count=1\n"
            "\t\t fragment stage\n"
            "\t\t binding=1},\n"
            "\t binding:\n"
            "\t\t {combined image sampler,\n"
            "\t\t descriptor count=1\n"
            "\t\t fragment stage\n"
            "\t\t binding=2},\n"
        );
    }
}


void _VulkanRenderer_Impl::createGraphicsPipelines()
{

    auto raii_pipeline_layout_deleter = [device = this->device](auto & obj)
    {
        device.destroyPipelineLayout(obj);
    };
    auto raii_pipeline_deleter = [device = this->device](auto & obj)
    {
        device.destroyPipeline(obj);
    };

    // create main pipeline
    {
        auto vert_shader_code = util::readFile(util::getContentPath("forwardplus_vert.spv"));
        auto frag_shader_code = util::readFile(util::getContentPath("forwardplus_frag.spv"));
        // auto light_culling_comp_shader_code = util::readFile(util::getContentPath("light_culling.comp.spv"));



        auto vert_shader_module = createShaderModule(vert_shader_code);
        auto frag_shader_module = createShaderModule(frag_shader_code);


        VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
        vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_info.module = vert_shader_module.get();
        vert_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
        frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_stage_info.module = frag_shader_module.get();
        frag_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vert_shader_stage_info, frag_shader_stage_info };

        // vertex data info
        VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto binding_description = vulkan_util::getVertexBindingDesciption();
        auto attr_description = vulkan_util::getVertexAttributeDescriptions();

        vertex_input_info.vertexBindingDescriptionCount = 1;
        vertex_input_info.pVertexBindingDescriptions = &binding_description;
        vertex_input_info.vertexAttributeDescriptionCount = (uint32_t)attr_description.size();
        vertex_input_info.pVertexAttributeDescriptions = attr_description.data(); // Optional

        // input assembler
        VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
        input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_info.primitiveRestartEnable = VK_FALSE;

        // viewport
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swap_chain_extent.width;
        viewport.height = (float)swap_chain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = swap_chain_extent;
        VkPipelineViewportStateCreateInfo viewport_state_info = {};
        viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_info.viewportCount = 1;
        viewport_state_info.pViewports = &viewport;
        viewport_state_info.scissorCount = 1;
        viewport_state_info.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f; // requires wideLines feature enabled when larger than one
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        //rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // what
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // inverted Y during projection matrix
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // no multisampling
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; /// Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional


        // depth and stencil
        VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
        depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil.depthTestEnable = VK_TRUE;
        depth_stencil.depthWriteEnable = VK_FALSE; // not VK_TRUE since we have a depth prepass
        depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; //not VK_COMPARE_OP_LESS since we have a depth prepass;
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState color_blend_attachment = {};
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        // Use alpha blending
        color_blend_attachment.blendEnable = VK_TRUE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo color_blending_info = {};
        color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending_info.logicOpEnable = VK_FALSE;
        color_blending_info.logicOp = VK_LOGIC_OP_COPY; // Optional
        color_blending_info.attachmentCount = 1;
        color_blending_info.pAttachments = &color_blend_attachment;
        color_blending_info.blendConstants[0] = 0.0f; // Optional
        color_blending_info.blendConstants[1] = 0.0f; // Optional
        color_blending_info.blendConstants[2] = 0.0f; // Optional
        color_blending_info.blendConstants[3] = 0.0f; // Optional

        // parameters allowed to be changed without recreating a pipeline
        VkDynamicState dynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };
        VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
        dynamic_state_info.dynamicStateCount = 2;
        dynamic_state_info.pDynamicStates = dynamicStates;

        VkPushConstantRange push_constant_range = {};
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(PushConstantObject);
        push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // no uniform variables or push constants
        std::vector<VkDescriptorSetLayout> set_layouts = {
            object_descriptor_set_layout.get(), camera_descriptor_set_layout.get(),
            light_culling_descriptor_set_layout.get(), intermediate_descriptor_set_layout.get(),
            material_descriptor_set_layout.get()
        };
        VkPipelineLayoutCreateInfo pipeline_layout_info = {};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(set_layouts.size()); // Optional
        pipeline_layout_info.pSetLayouts = set_layouts.data(); // Optional
        pipeline_layout_info.pushConstantRangeCount = 1; // Optional
        pipeline_layout_info.pPushConstantRanges = &push_constant_range; // Optional


        VkPipelineLayout temp_layout;
        auto pipeline_layout_result = vkCreatePipelineLayout(graphics_device, &pipeline_layout_info, nullptr, &temp_layout);
        if (pipeline_layout_result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }
        pipeline_layout = VRaii<VkPipelineLayout>(temp_layout, raii_pipeline_layout_deleter);

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        pipelineInfo.pVertexInputState = &vertex_input_info;
        pipelineInfo.pInputAssemblyState = &input_assembly_info;
        pipelineInfo.pViewportState = &viewport_state_info;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depth_stencil;
        pipelineInfo.pColorBlendState = &color_blending_info;
        pipelineInfo.pDynamicState = nullptr; // Optional
        pipelineInfo.layout = pipeline_layout.get();
        pipelineInfo.renderPass = render_pass.get();
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // not deriving from existing pipeline
        pipelineInfo.basePipelineIndex = -1; // Optional
        pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

        VkPipeline temp_pipeline;
        auto pipeline_result = vkCreateGraphicsPipelines(graphics_device, VK_NULL_HANDLE, 1 , &pipelineInfo, nullptr, &temp_pipeline);
        graphics_pipeline = VRaii<VkPipeline>(temp_pipeline, raii_pipeline_deleter);

        if (pipeline_result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        util::writeLog("create main pass pipeline state object:\n"
            "\t reading shader code: forwardplus_vert.spv forwardplus_frag.spv\n"
            "\t creating shader modules\n"
            "\t creating shader stage : {shader stage, shader module,entry point}\n"
            "\t creating vertex input: vertex binding description\n"
            "\t creating vertex input: vertex attribute description\n"
            "\t creating input assembly state: {topology, enable primitive restart}\n"
            "\t creating viewport state info : {viewports, viewport count, scissors, scissors count}\n"
            "\t creating multiple sample state info: {enable, samples, alpha to coverage, alpha to one}\n"
            "\t creating depth stencil state info: {depth test enable, depth test enable, depth compare operation}\n"
            "\t creating color blend state info: {}\n"
            "\t creating color blend attachment state : {}\n"
            "\t creating dynamic state info: {dynamic states, state count}\n"
            "\t creating push constant range: {offset, size, flag}\n"
            "\t creating pipeline layout info: {<descriptor sets,count>, <push constant,count> } //< it contains all shader resources bindings\n"
            "\t\t [scene object descriptor set layout]\n"
            "\t\t [camera object descriptor set layout]\n"
            "\t\t [lighting culling parameter descriptor set layout]\n"
            "\t\t [intermediate descriptor set layout]\n"
            "\t\t [material descriptor set layout]\n"
        );
        //-------------------------------------depth prepass pipeline ------------------------------------------------

        {

            VkPipelineDepthStencilStateCreateInfo pre_pass_depth_stencil = { depth_stencil };
            pre_pass_depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
            pre_pass_depth_stencil.depthWriteEnable = VK_TRUE;

            auto depth_vert_shader_code = util::readFile(util::getContentPath("depth_vert.spv"));
            // auto light_culling_comp_shader_code = util::readFile(util::getContentPath("light_culling.comp.spv"));
            auto depth_vert_shader_module = createShaderModule(depth_vert_shader_code);
            VkPipelineShaderStageCreateInfo depth_vert_shader_stage_info = {};
            depth_vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            depth_vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
            depth_vert_shader_stage_info.module = depth_vert_shader_module.get();
            depth_vert_shader_stage_info.pName = "main";
            VkPipelineShaderStageCreateInfo depth_shader_stages[] = { depth_vert_shader_stage_info };

            std::array<vk::DescriptorSetLayout, 2> depth_set_layouts = { object_descriptor_set_layout.get(), camera_descriptor_set_layout.get() };

            vk::PipelineLayoutCreateInfo depth_layout_info =
            {
                vk::PipelineLayoutCreateFlags(),  // flags
                static_cast<uint32_t>(depth_set_layouts.size()),  // setLayoutCount
                depth_set_layouts.data(),                         // setlayouts
                0,      // pushConstantRangeCount
                nullptr // pushConstantRanges
            };
            depth_pipeline_layout = VRaii<vk::PipelineLayout>(
                device.createPipelineLayout(depth_layout_info, nullptr),
                raii_pipeline_layout_deleter
                );

            VkGraphicsPipelineCreateInfo depth_pipeline_info = {};
            depth_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            depth_pipeline_info.stageCount = 1;
            depth_pipeline_info.pStages = depth_shader_stages;

            depth_pipeline_info.pVertexInputState = &vertex_input_info;
            depth_pipeline_info.pInputAssemblyState = &input_assembly_info;
            depth_pipeline_info.pViewportState = &viewport_state_info;
            depth_pipeline_info.pRasterizationState = &rasterizer;
            depth_pipeline_info.pMultisampleState = &multisampling;
            depth_pipeline_info.pDepthStencilState = &pre_pass_depth_stencil;
            depth_pipeline_info.pColorBlendState = nullptr;
            depth_pipeline_info.pDynamicState = nullptr; // Optional
            depth_pipeline_info.layout = depth_pipeline_layout.get();
            depth_pipeline_info.renderPass = depth_pre_pass.get();
            depth_pipeline_info.subpass = 0;
            depth_pipeline_info.basePipelineHandle = graphics_pipeline.get(); // not deriving from existing pipeline
            depth_pipeline_info.basePipelineIndex = -1; // Optional
            depth_pipeline_info.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;

            depth_pipeline = VRaii<vk::Pipeline>(
                device.createGraphicsPipeline(vk::PipelineCache(), depth_pipeline_info, nullptr),
                raii_pipeline_deleter
                );
        util::writeLog("create early depth pass pipeline state object:\n"
            "\t reading shader code: depth_vert.spv\n"
            "\t creating shader modules\n"
            "\t creating shader stage : {shader stage, shader module,entry point}\n"
            "\t creating vertex input: vertex binding description\n"
            "\t creating vertex input: vertex attribute description\n"
            "\t creating input assembly state: {topology, enable primitive restart}\n"
            "\t creating viewport state info : {viewports, viewport count, scissors, scissors count}\n"
            "\t creating multiple sample state info: {enable, samples, alpha to coverage, alpha to one}\n"
            "\t creating depth stencil state info: {depth test enable, depth test enable, depth compare operation}\n"
            "\t creating color blend state info: {}\n"
            "\t creating color blend attachment state : {}\n"
            "\t creating dynamic state info: {dynamic states, state count}\n"
            "\t creating push constant range: {offset, size, flag}\n"
            "\t creating pipeline layout info: {descriptor sets, descriptor set count, push constant range}\n"
            "\t\t [scene object descriptor set layout]\n"
            "\t\t [camera object descriptor set layout]\n"
        );
        }
    }
}

void _VulkanRenderer_Impl::createFrameBuffers()
{
    auto raii_framebuffer_deleter = [device = this->device](auto & obj)
    {
        device.destroyFramebuffer(obj);
    };

    // swap chain frame buffers
    {
        swap_chain_framebuffers.clear(); // VDeleter will delete old objects
        swap_chain_framebuffers.reserve(swap_chain_imageviews.size());

        for (size_t i = 0; i < swap_chain_imageviews.size(); i++)
        {
            std::array<VkImageView, 2> attachments = { swap_chain_imageviews[i].get(), depth_image_view.get() };

            VkFramebufferCreateInfo framebuffer_info = {};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = render_pass.get();
            framebuffer_info.attachmentCount = (uint32_t)attachments.size();
            framebuffer_info.pAttachments = attachments.data();
            framebuffer_info.width = swap_chain_extent.width;
            framebuffer_info.height = swap_chain_extent.height;
            framebuffer_info.layers = 1;

            swap_chain_framebuffers.emplace_back(
                device.createFramebuffer(framebuffer_info, nullptr),
                raii_framebuffer_deleter
            );

            util::writeLog("create frame buffer %d for main pass:\n"
                "\t attachment: color image view, depth image view\n"
                "\t size: %dx%d\n"
                "\t layer: 1\n"
                ,i, swap_chain_extent.width,swap_chain_extent.height
            );
        }
    }

    // depth pass frame buffer
    {
        std::array<VkImageView, 1> attachments = { depth_image_view.get() };

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = depth_pre_pass.get();
        framebuffer_info.attachmentCount = (uint32_t)attachments.size();
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = swap_chain_extent.width;
        framebuffer_info.height = swap_chain_extent.height;
        framebuffer_info.layers = 1;

        depth_pre_pass_framebuffer = VRaii<vk::Framebuffer>(
            device.createFramebuffer(framebuffer_info, nullptr),
            raii_framebuffer_deleter
        );
            util::writeLog("create frame buffer for early depth pass:\n"
                "\t attachment: color image view, depth image view\n"
                "\t size: %dx%d\n"
                "\t layer: 1\n"
                , swap_chain_extent.width,swap_chain_extent.height
            );
    }
}


void _VulkanRenderer_Impl::createDepthResources()
{
    VkFormat depth_format = utility.findDepthFormat();

    // for depth pre pass and output as texture
    std::tie(depth_image, depth_image_memory) = utility.createImage(swap_chain_extent.width, swap_chain_extent.height
        , depth_format
        , VK_IMAGE_TILING_OPTIMAL
        //, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT  // TODO: if creating another depth image for prepass use, use this only for rendering depth image
        , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    depth_image_view = utility.createImageView(depth_image.get(), depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
    utility.transitImageLayout(depth_image.get(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    util::writeLog("create depth buffer resources & insert a resource barrier :{image & image view}\n" );
}

void _VulkanRenderer_Impl::createTextureSampler()
{
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;

    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16;

    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;

    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    VkSampler sampler;
    if (vkCreateSampler(graphics_device, &sampler_info, nullptr, &sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture sampler!");
    }

    texture_sampler = VRaii<VkSampler>(
        sampler,
        [device = this->device](auto& obj)
        {
            device.destroySampler(obj);
        }
    );

    util::writeLog("create image sampler.\n" );
}

void _VulkanRenderer_Impl::createUniformBuffers()
{
    // create buffers for scene object
    {
        VkDeviceSize bufferSize = sizeof(SceneObjectUbo);

        std::tie(object_staging_buffer, object_staging_buffer_memory) = utility.createBuffer(bufferSize
            , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        std::tie(object_uniform_buffer, object_uniform_buffer_memory) = utility.createBuffer(bufferSize
            , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
    // Adding data to scene object buffer
    {
        SceneObjectUbo ubo = {};
        ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(getGlobalTestSceneConfiguration().scale));;

        void* data;
        vkMapMemory(graphics_device, object_staging_buffer_memory.get(), 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(graphics_device, object_staging_buffer_memory.get());
        utility.copyBuffer(object_staging_buffer.get(), object_uniform_buffer.get(), sizeof(ubo));
    }
    util::writeLog("create shader parameter @ uniform buffer:\n"
        "\t SceneObjectUbo{model transform} staging buffer & uniform buffer\n"
        "\t updating uniform buffer by staging buffer\n"
    );

    // create buffers for camera
    {
        VkDeviceSize bufferSize = sizeof(CameraUbo);

        std::tie(camera_staging_buffer, camera_staging_buffer_memory) = utility.createBuffer(bufferSize
            , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        std::tie(camera_uniform_buffer, camera_uniform_buffer_memory) = utility.createBuffer(bufferSize
            , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  // FIXME: change back to uniform
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            , queue_family_indices.graphics_family
            , 0);//        , queue_family_indices.compute_family);
    }
    util::writeLog("create shader parameter @ uniform buffer:\n"
        "\t SceneObjectUbo{model transform} staging buffer & uniform buffer\n"
        "\t CameraUbo{model transform} staging buffer & uniform buffer\n"
    );

}

void _VulkanRenderer_Impl::createLights()
{
    for (int i = 0; i < getGlobalTestSceneConfiguration().light_num; i++) {
        glm::vec3 color;
        do { color = { glm::linearRand(glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)) }; }
        while (color.length() < 0.8f);
        pointlights.emplace_back(glm::linearRand(getGlobalTestSceneConfiguration().min_light_pos, getGlobalTestSceneConfiguration().max_light_pos), getGlobalTestSceneConfiguration().light_radius, color);
    }
    util::writeLog("configured point light number = %d\n",getGlobalTestSceneConfiguration().light_num);

    // TODO: choose between memory mapping and staging buffer
    //  (given that the lights are moving)
    auto light_num = static_cast<int>(pointlights.size());

    pointlight_buffer_size = sizeof(PointLight) * MAX_POINT_LIGHT_COUNT + sizeof(glm::vec4); // vec4 rather than int for padding

    std::tie(lights_staging_buffer, lights_staging_buffer_memory) = utility.createBuffer(pointlight_buffer_size
        , VK_BUFFER_USAGE_TRANSFER_SRC_BIT // to be transfered from
        , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    std::tie(pointlight_buffer, pointlight_buffer_memory) = utility.createBuffer(pointlight_buffer_size
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT  // FIXME: change back to uniform
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // using barrier to sync
    util::writeLog("create point light uniform buffer.\n"
        "\t Point light parameters staging buffer & uniform buffer\n"
    );
}

void _VulkanRenderer_Impl::createDescriptorPool()
{
    // Create descriptor pool for uniform buffer
    std::array<VkDescriptorPoolSize, 3> pool_sizes = {};
    //std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = 100; // transform buffer & light buffer & camera buffer & light buffer in compute pipeline
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = 100; // sampler for color map and normal map and depth map from depth prepass... and so many from scene materials
    pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_sizes[2].descriptorCount = 3; // light visiblity buffer in graphics pipeline and compute pipeline

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = (uint32_t)pool_sizes.size();
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = 200;
    pool_info.flags = 0;
    //poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    // TODO: use VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT so I can create a VKGemoetryClass

    util::writeLog("create desciptor pool.\n"
        "\t 100 uniform buffer\n"
        "\t 100 combined image sampler\n"
        "\t 3   storage buffer\n"
    );

    VkDescriptorPool pool;
    if (vkCreateDescriptorPool(graphics_device, &pool_info, nullptr, &pool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool!");
    }

    descriptor_pool = VRaii<VkDescriptorPool>(
        pool,
        [device = this->device](auto& obj)
        {
            device.destroyDescriptorPool(obj);
        }
    );
}

void _VulkanRenderer_Impl::createSceneObjectDescriptorSet()
{

    VkDescriptorSetLayout layouts[] = { object_descriptor_set_layout.get() };
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool.get();
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(graphics_device, &alloc_info, &object_descriptor_set) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor set!");
    }


    // refer to the uniform object buffer
    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = object_uniform_buffer.get();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(SceneObjectUbo);

    //std::array<VkWriteDescriptorSet, 4> descriptor_writes = {};
    std::array<VkWriteDescriptorSet, 1> descriptor_writes = {};

    // ubo
    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].dstSet = object_descriptor_set;
    descriptor_writes[0].dstBinding = 0;
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].pBufferInfo = &buffer_info;
    descriptor_writes[0].pImageInfo = nullptr; // Optional
    descriptor_writes[0].pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(graphics_device, (uint32_t)descriptor_writes.size()
        , descriptor_writes.data(), 0, nullptr);

    util::writeLog("create scene object desciptor set.\n"
        "\t update descriptor set by a write descriptor set.\n"
    );
}

void _VulkanRenderer_Impl::createCameraDescriptorSet()
{
    // Create descriptor set
    {
        vk::DescriptorSetAllocateInfo alloc_info = {
            descriptor_pool.get(),  // descriptorPool
            1,  // descriptorSetCount
            camera_descriptor_set_layout.data(), // pSetLayouts
        };

        camera_descriptor_set = device.allocateDescriptorSets(alloc_info)[0];
    }

    // Write desciptor set
    {
        // refer to the uniform object buffer
        vk::DescriptorBufferInfo camera_uniform_buffer_info{
            camera_uniform_buffer.get(), // buffer_
            0, //offset_
            sizeof(CameraUbo) // range_
        };

        std::vector<vk::WriteDescriptorSet> descriptor_writes = {};

        descriptor_writes.emplace_back(
            camera_descriptor_set, // dstSet
            0, // dstBinding
            0, // distArrayElement
            1, // descriptorCount
            vk::DescriptorType::eStorageBuffer, //descriptorType // FIXME: change back to uniform
            nullptr, //pImageInfo
            &camera_uniform_buffer_info, //pBufferInfo
            nullptr //pTexBufferView
        );

        std::array<vk::CopyDescriptorSet, 0> descriptor_copies;
        device.updateDescriptorSets(descriptor_writes, descriptor_copies);
    }

    util::writeLog("create camera object desciptor set.\n"
        "\t update descriptor set by a write descriptor set.\n"
    );
}

void _VulkanRenderer_Impl::createIntermediateDescriptorSet()
{
    // Create descriptor set
    {
        vk::DescriptorSetAllocateInfo alloc_info = {
            descriptor_pool.get(),  // descriptorPool
            1,  // descriptorSetCount
            intermediate_descriptor_set_layout.data(), // pSetLayouts
        };

        intermediate_descriptor_set = device.allocateDescriptorSets(alloc_info)[0];
    }

    util::writeLog("create intermediate desciptor set.\n"
        "\t Descriptor sets are allocated from descriptor pool, in GPU & CPU accessible memory.\n"
        "\t State is uninit after creation.\n"
    );
}

void _VulkanRenderer_Impl::updateIntermediateDescriptorSet()
{
    // Write desciptor set

        vk::DescriptorImageInfo depth_image_info = {
            texture_sampler.get(),
            depth_image_view.get(),
            vk::ImageLayout::eShaderReadOnlyOptimal
        };

        std::vector<vk::WriteDescriptorSet> descriptor_writes = {};

        descriptor_writes.emplace_back(
            intermediate_descriptor_set, // dstSet
            0, // dstBinding
            0, // distArrayElement
            1, // descriptorCount
            vk::DescriptorType::eCombinedImageSampler, //descriptorType // FIXME: change back to uniform
            &depth_image_info, //pImageInfo
            nullptr, //pBufferInfo
            nullptr //pTexBufferView
        );

        std::array<vk::CopyDescriptorSet, 0> descriptor_copies;
        device.updateDescriptorSets(descriptor_writes, descriptor_copies);

        util::writeLog( "update intermediate descriptor set by a write descriptor set.\n"
            "\t dstSet = intermediate_descriptor_set, dstbinding = 0, dstelement = 0, count = 1\n"
            "\t type = combined image sampler\n"
            "\t descriptor image info = {texture sampler, depth image, }\n"
        );
}

void _VulkanRenderer_Impl::createDepthPrePassCommandBuffer()
{
    if (depth_prepass_command_buffer)
    {
        device.freeCommandBuffers(graphics_command_pool, 1, &depth_prepass_command_buffer);
        depth_prepass_command_buffer = nullptr;
    }

    // Create depth pre-pass command buffer
    {
        vk::CommandBufferAllocateInfo alloc_info = {
            graphics_command_pool, // command pool
            vk::CommandBufferLevel::ePrimary, // level
            1 // commandBufferCount
        };

        depth_prepass_command_buffer = device.allocateCommandBuffers(alloc_info)[0];
    }

    util::writeLog( "pre-depth pass --> create primary command buffer.\n" );
    // Begin command
    {
        vk::CommandBufferBeginInfo begin_info =
        {
            vk::CommandBufferUsageFlagBits::eSimultaneousUse,
            nullptr
        };

        auto command = depth_prepass_command_buffer;

        command.begin(begin_info);

        std::array<vk::ClearValue, 1> clear_values = {};
        clear_values[0].depthStencil = vk::ClearDepthStencilValue( 1.0f, 0 ); // 1.0 is far view plane
        vk::RenderPassBeginInfo depth_pass_info = {
            depth_pre_pass.get(),
            depth_pre_pass_framebuffer.get(),
            vk::Rect2D({ 0,0 }, swap_chain_extent),
            static_cast<uint32_t>(clear_values.size()),
            clear_values.data()
        };
        command.beginRenderPass(&depth_pass_info, vk::SubpassContents::eInline);

        util::writeLog( "pre-depth pass --> begin render pass.\n" );
        for (const auto& part: model.getMeshParts() )
        {
            command.bindPipeline(vk::PipelineBindPoint::eGraphics, depth_pipeline.get());

            std::array<vk::DescriptorSet, 2> depth_descriptor_sets = { object_descriptor_set, camera_descriptor_set };
            std::array<uint32_t, 0> depth_dynamic_offsets;
            command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, depth_pipeline_layout.get(), 0, depth_descriptor_sets, depth_dynamic_offsets);

            std::array<vk::Buffer, 1> depth_vertex_buffers = { part.vertex_buffer_section.buffer };
            std::array<vk::DeviceSize, 1> depth_offsets = { part.vertex_buffer_section.offset };
            command.bindVertexBuffers(0, depth_vertex_buffers, depth_offsets);
            command.bindIndexBuffer(part.index_buffer_section.buffer, part.index_buffer_section.offset, vk::IndexType::eUint32);

            command.drawIndexed(static_cast<uint32_t>(part.index_count), 1, 0, 0, 0);
            util::writeLog( "\t pre-depth pass --> bind graphic render pipeline object.\n"
                "\t pre-depth pass --> bind graphic descriptor sets{objetct's, camera's}\n"
                "\t pre-depth pass --> bind VB.\n"
                "\t pre-depth pass --> bind IB.\n"
                "\t pre-depth pass --> draw Indexed.\n"
            );
        }
        command.endRenderPass();

        command.end();

        util::writeLog( "pre-depth pass --> end render pass.\n" );
        util::writeLog( "pre-depth pass --> end command buffer.\n" );
    }

}

void _VulkanRenderer_Impl::createGraphicsCommandBuffers()
{
    // Free old command buffers, if any
    if (command_buffers.size() > 0)
    {
        vkFreeCommandBuffers(graphics_device, graphics_command_pool, (uint32_t)command_buffers.size(), command_buffers.data());
    }
    command_buffers.clear();

    command_buffers.resize(swap_chain_framebuffers.size());

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = graphics_command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // primary: can be submitted to a queue but cannot be called from other command buffers
    // secondary: can be called by others but cannot be submitted to a queue
    alloc_info.commandBufferCount = (uint32_t)command_buffers.size();

    auto alloc_result = vkAllocateCommandBuffers(graphics_device, &alloc_info, command_buffers.data());
    if (alloc_result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    util::writeLog( "main pass create PRIMARY command buffer. count=%d\n", command_buffers.size());

    // record command buffers
    for (size_t i = 0; i < command_buffers.size(); i++)
    {
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        begin_info.pInheritanceInfo = nullptr; // Optional

        vkBeginCommandBuffer(command_buffers[i], &begin_info);

        util::writeLog( "main pass begin command buffer. index=%d\n", i);
        // render pass
        {
            VkRenderPassBeginInfo render_pass_info = {};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass = render_pass.get();
            render_pass_info.framebuffer = swap_chain_framebuffers[i].get();
            render_pass_info.renderArea.offset = { 0, 0 };
            render_pass_info.renderArea.extent = swap_chain_extent;

            std::array<VkClearValue, 1> clear_values = {};
            clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
            //clear_values[1].depthStencil = { 1.0f, 0 }; // don't clear with depth prepass
            render_pass_info.clearValueCount = (uint32_t)clear_values.size();
            render_pass_info.pClearValues = clear_values.data();

            vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

            util::writeLog( "main pass --> begin render pass.\n", i);
            PushConstantObject pco = {
                static_cast<int>(swap_chain_extent.width),
                static_cast<int>(swap_chain_extent.height),
                tile_count_per_row, tile_count_per_col,
                debug_view_index
            };
            vkCmdPushConstants(command_buffers[i], pipeline_layout.get(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pco), &pco);
            util::writeLog( "main pass --> push constant.\n", i);


            vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.get());
            util::writeLog( "main pass --> bind graphics pipeline object.\n", i);

            std::array<VkDescriptorSet, 4> descriptor_sets = { object_descriptor_set, camera_descriptor_set, light_culling_descriptor_set, intermediate_descriptor_set };
            vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS
                , pipeline_layout.get(), 0, static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), 0, nullptr);
            util::writeLog( "main pass --> bind graphics descriptor sets {object's, camera's, light culling's, intermediate's}.\n", i);

            for (const auto& part : model.getMeshParts())
            {
                // bind vertex buffer
                VkBuffer vertex_buffers[] = { part.vertex_buffer_section.buffer };
                VkDeviceSize offsets[] = { part.vertex_buffer_section.offset };
                vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);
                //vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT16);
                vkCmdBindIndexBuffer(command_buffers[i], part.index_buffer_section.buffer, part.index_buffer_section.offset, VK_INDEX_TYPE_UINT32);

                std::array<VkDescriptorSet, 1> mesh_descriptor_sets = { part.material_descriptor_set };
                vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS
                    , pipeline_layout.get(), static_cast<uint32_t>(descriptor_sets.size()), static_cast<uint32_t>(mesh_descriptor_sets.size()), mesh_descriptor_sets.data(), 0, nullptr);

                //vkCmdDraw(command_buffers[i], VERTICES.size(), 1, 0, 0);
                vkCmdDrawIndexed(command_buffers[i], static_cast<uint32_t>(part.index_count), 1, 0, 0, 0);
                util::writeLog( "\t main pass --> bind graphic descriptor set.{material}\n"
                "\t main pass --> bind VB.\n"
                "\t main pass --> bind IB.\n"
                "\t main pass --> draw Indexed.\n"
            );
            }
            vkCmdEndRenderPass(command_buffers[i]);
            //utility.recordTransitImageLayout(command_buffers[i], pre_pass_depth_image.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        
            util::writeLog( "main pass --> end render pass.\n", i);
            util::writeLog( "main pass end command buffer. index=%d\n", i);
        }

        auto record_result = vkEndCommandBuffer(command_buffers[i]);
        if (record_result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}

void _VulkanRenderer_Impl::createSemaphores()
{
    vk::SemaphoreCreateInfo semaphore_info = { vk::SemaphoreCreateFlags() };

    auto destroy_func = [&device = this->device](auto & obj)
    {
        device.destroySemaphore(obj);
    };

    render_finished_semaphore = VRaii<vk::Semaphore>(
        device.createSemaphore(semaphore_info, nullptr),
        destroy_func
    );
    image_available_semaphore = VRaii<vk::Semaphore>(
        device.createSemaphore(semaphore_info, nullptr),
        destroy_func
    );
    lightculling_completed_semaphore = VRaii<vk::Semaphore>(
        device.createSemaphore(semaphore_info, nullptr),
        destroy_func
    );
    depth_prepass_finished_semaphore = VRaii<vk::Semaphore>(
        device.createSemaphore(semaphore_info, nullptr),
        destroy_func
    );
   util::writeLog( "create semaphore{render finished, image available, light culling done, early depth done}.\n");
}


/**
* Create compute pipeline for light culling
*/
void _VulkanRenderer_Impl::createComputePipeline()
{
    // Step 1: Create Pipeline
    {
        auto raii_pipeline_layout_deleter = [device = this->device](auto & obj)
        {
            device.destroyPipelineLayout(obj);
        };
        auto raii_pipeline_deleter = [device = this->device](auto & obj)
        {
            device.destroyPipeline(obj);
        };

        VkPushConstantRange push_constant_range = {};
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(PushConstantObject);
        push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkPipelineLayoutCreateInfo pipeline_layout_info = {};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        std::array<VkDescriptorSetLayout, 3> set_layouts = { light_culling_descriptor_set_layout.get(), camera_descriptor_set_layout.get(), intermediate_descriptor_set_layout.get()};
        pipeline_layout_info.setLayoutCount = static_cast<int>(set_layouts.size());
        pipeline_layout_info.pSetLayouts = set_layouts.data();
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &push_constant_range;

        VkPipelineLayout temp_layout;
        vulkan_util::checkResult(vkCreatePipelineLayout(graphics_device, &pipeline_layout_info, nullptr, &temp_layout));
        compute_pipeline_layout = VRaii<VkPipelineLayout>(temp_layout, raii_pipeline_layout_deleter);

        auto light_culling_comp_shader_code = util::readFile(util::getContentPath("light_culling_comp.spv"));

        auto comp_shader_module = createShaderModule(light_culling_comp_shader_code);
        VkPipelineShaderStageCreateInfo comp_shader_stage_info = {};
        comp_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        comp_shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        comp_shader_stage_info.module = comp_shader_module.get();
        comp_shader_stage_info.pName = "main";

        VkComputePipelineCreateInfo pipeline_create_info;
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_create_info.stage = comp_shader_stage_info;
        pipeline_create_info.layout = compute_pipeline_layout.get();
        pipeline_create_info.pNext = nullptr;
        pipeline_create_info.flags = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE; // not deriving from existing pipeline
        pipeline_create_info.basePipelineIndex = -1; // Optional

        VkPipeline temp_pipeline;
        vulkan_util::checkResult(vkCreateComputePipelines(graphics_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &temp_pipeline));
        compute_pipeline = VRaii<VkPipeline>(temp_pipeline, raii_pipeline_deleter);
    };
}

/**
* creating light visiblity descriptor sets for both passes
*/
void _VulkanRenderer_Impl::createLigutCullingDescriptorSet()
{
    // create shared dercriptor set between compute pipeline and rendering pipeline
    {
        // todo: reduce code duplication with createDescriptorSet()
        VkDescriptorSetLayout layouts[] = { light_culling_descriptor_set_layout.get() };
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = descriptor_pool.get();
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = layouts;

        light_culling_descriptor_set = device.allocateDescriptorSets(alloc_info)[0];
    }

}

// just for sizing information
struct _Dummy_VisibleLightsForTile
{
    uint32_t count;
    std::array<uint32_t, MAX_POINT_LIGHT_PER_TILE> lightindices;
};

/**
* Create or recreate light visibility buffer and its descriptor
*/
void _VulkanRenderer_Impl::createLightVisibilityBuffer()
{
    assert(sizeof(_Dummy_VisibleLightsForTile) == sizeof(int) * (MAX_POINT_LIGHT_PER_TILE + 1));

    tile_count_per_row = (swap_chain_extent.width - 1) / TILE_SIZE + 1;
    tile_count_per_col = (swap_chain_extent.height - 1) / TILE_SIZE + 1;

    light_visibility_buffer_size = sizeof(_Dummy_VisibleLightsForTile) * tile_count_per_row * tile_count_per_col;

    std::tie(light_visibility_buffer, light_visibility_buffer_memory) = utility.createBuffer(
        light_visibility_buffer_size
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    ); // using barrier to sync

    // Write desciptor set in compute shader
    {
        // refer to the uniform object buffer
        vk::DescriptorBufferInfo light_visibility_buffer_info{
            light_visibility_buffer.get(), // buffer_
            0, //offset_
            light_visibility_buffer_size // range_
        };

        // refer to the uniform object buffer
        vk::DescriptorBufferInfo pointlight_buffer_info = {
            pointlight_buffer.get(), // buffer_
            0, //offset_
            pointlight_buffer_size // range_
        };

        std::vector<vk::WriteDescriptorSet> descriptor_writes = {};

        descriptor_writes.emplace_back(
            light_culling_descriptor_set, // dstSet
            0, // dstBinding
            0, // distArrayElement
            1, // descriptorCount
            vk::DescriptorType::eStorageBuffer, //descriptorType
            nullptr, //pImageInfo
            &light_visibility_buffer_info, //pBufferInfo
            nullptr //pTexBufferView
        );

        descriptor_writes.emplace_back(
            light_culling_descriptor_set, // dstSet
            1, // dstBinding
            0, // distArrayElement
            1, // descriptorCount
            vk::DescriptorType::eStorageBuffer, //descriptorType // FIXME: change back to uniform
            nullptr, //pImageInfo
            &pointlight_buffer_info, //pBufferInfo
            nullptr //pTexBufferView
        );

        std::array<vk::CopyDescriptorSet, 0> descriptor_copies;
        device.updateDescriptorSets(descriptor_writes, descriptor_copies);
        util::writeLog("create visibility buffer & point light buffer, update descriptor set.\n");
    }

}

void _VulkanRenderer_Impl::createLightCullingCommandBuffer()
{

    if (light_culling_command_buffer)
    {
        device.freeCommandBuffers(compute_command_pool, 1, &light_culling_command_buffer);
        light_culling_command_buffer = nullptr;
    }

    // Create light culling command buffer
    {
        vk::CommandBufferAllocateInfo alloc_info = {
            compute_command_pool, // command pool
            vk::CommandBufferLevel::ePrimary, // level
            1 // commandBufferCount
        };

        light_culling_command_buffer = device.allocateCommandBuffers(alloc_info)[0];
    }

    util::writeLog("light culling pass create command buffer.\n");
    // Record command buffer
    {
        vk::CommandBufferBeginInfo begin_info =
        {
            vk::CommandBufferUsageFlagBits::eSimultaneousUse,
            nullptr
        };

        vk::CommandBuffer command(light_culling_command_buffer);

        command.begin(begin_info);

        util::writeLog("light culling pass --> begin command buffer.\n");
        // using barrier since the sharing mode when allocating memory is exclusive
        // begin after fragment shader finished reading from storage buffer

        std::vector<vk::BufferMemoryBarrier> barriers_before;
        barriers_before.emplace_back
        (
            vk::AccessFlagBits::eShaderRead,  // srcAccessMask
            vk::AccessFlagBits::eShaderWrite,  // dstAccessMask
            0, //static_cast<uint32_t>(queue_family_indices.graphics_family),  // srcQueueFamilyIndex
            0, //static_cast<uint32_t>(queue_family_indices.compute_family),  // dstQueueFamilyIndex
            static_cast<vk::Buffer>(light_visibility_buffer.get()),  // buffer
            0,  // offset
            light_visibility_buffer_size  // size
        );
        barriers_before.emplace_back
        (
            vk::AccessFlagBits::eShaderRead,  // srcAccessMask // FIXME: change back to uniform
            vk::AccessFlagBits::eShaderWrite,  // dstAccessMask
            0, //static_cast<uint32_t>(queue_family_indices.graphics_family),  // srcQueueFamilyIndex
            0, //static_cast<uint32_t>(queue_family_indices.compute_family),  // dstQueueFamilyIndex
            static_cast<vk::Buffer>(pointlight_buffer.get()),  // buffer
            0,  // offset
            pointlight_buffer_size  // size
        );

        command.pipelineBarrier(
            vk::PipelineStageFlagBits::eFragmentShader,  // srcStageMask
            vk::PipelineStageFlagBits::eComputeShader,  // dstStageMask
            vk::DependencyFlags(),  // dependencyFlags
            0,  // memoryBarrierCount
            nullptr,  // pBUfferMemoryBarriers
            static_cast<uint32_t>(barriers_before.size()),  // bufferMemoryBarrierCount
            barriers_before.data(),  // pBUfferMemoryBarriers
            0,  // imageMemoryBarrierCount
            nullptr // pImageMemoryBarriers
        );

        util::writeLog("light culling pass --> set memory barrier{visibility buffer 'ps read'-'cs write' barrier,point light 'ps read'-'cs write' barrier}.\n");

        command.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute, // pipelineBindPoint
            compute_pipeline_layout.get(), // layout
            0, // firstSet
            std::array<vk::DescriptorSet, 3>{light_culling_descriptor_set, camera_descriptor_set, intermediate_descriptor_set}, // descriptorSets
            std::array<uint32_t, 0>() // pDynamicOffsets
        );

        util::writeLog("light culling pass --> set descriptor sets{light culling's, camera's, intermediate's}.\n");

        PushConstantObject pco = { static_cast<int>(swap_chain_extent.width), static_cast<int>(swap_chain_extent.height), tile_count_per_row, tile_count_per_col };
        command.pushConstants(compute_pipeline_layout.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pco), &pco);
        util::writeLog("light culling pass --> push constants.\n");

        command.bindPipeline(vk::PipelineBindPoint::eCompute, static_cast<VkPipeline>(compute_pipeline.get()));
        util::writeLog("light culling pass --> bind compute pipeline object.\n");
        command.dispatch(tile_count_per_row, tile_count_per_col, 1);
        util::writeLog("light culling pass --> dispatch compute shader(%d,%d,%d).\n", tile_count_per_row, tile_count_per_col, 1);

        std::vector<vk::BufferMemoryBarrier> barriers_after;
        barriers_after.emplace_back
        (
            vk::AccessFlagBits::eShaderWrite,  // srcAccessMask
            vk::AccessFlagBits::eShaderRead,  // dstAccessMask
            0,//static_cast<uint32_t>(queue_family_indices.compute_family), // srcQueueFamilyIndex
            0,//static_cast<uint32_t>(queue_family_indices.graphics_family),  // dstQueueFamilyIndex
            static_cast<vk::Buffer>(light_visibility_buffer.get()),  // buffer
            0,  // offset
            light_visibility_buffer_size  // size
        );
        barriers_after.emplace_back
        (
            vk::AccessFlagBits::eShaderWrite,  // srcAccessMask // TODO: change back to uniform
            vk::AccessFlagBits::eShaderRead,  // dstAccessMask
            0, //static_cast<uint32_t>(queue_family_indices.compute_family), // srcQueueFamilyIndex
            0, //static_cast<uint32_t>(queue_family_indices.graphics_family),  // dstQueueFamilyIndex
            static_cast<vk::Buffer>(pointlight_buffer.get()),  // buffer
            0,  // offset
            pointlight_buffer_size  // size
        );

        command.pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlags(),
            0, nullptr,
            static_cast<uint32_t>(barriers_after.size()), barriers_after.data(), // TODO
            0, nullptr
        );

        util::writeLog("light culling pass --> set memory barrier{visibility buffer 'cs write'-'ps read' barrier,point light 'cs write'-'ps read' barrier}.\n");
        command.end();
        util::writeLog("light culling pass --> end command buffer.\n");
    }
}


void _VulkanRenderer_Impl::updateUniformBuffers(float deltatime)
{
    static auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() / 1000.0f;

    // update camera ubo
    {
        CameraUbo ubo = {};
        ubo.view = view_matrix;
        ubo.proj = glm::perspective(glm::radians(45.0f), swap_chain_extent.width / (float)swap_chain_extent.height, 0.5f, 100.0f);
        ubo.proj[1][1] *= -1; //since the Y axis of Vulkan NDC points down
        ubo.projview = ubo.proj * ubo.view;
        ubo.cam_pos = cam_pos;

        void* data;
        vkMapMemory(graphics_device, camera_staging_buffer_memory.get(), 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(graphics_device, camera_staging_buffer_memory.get());

        // TODO: maybe I shouldn't use single time buffer
        utility.copyBuffer(camera_staging_buffer.get(), camera_uniform_buffer.get(), sizeof(ubo));
    }

    // update light ubo
    {
        auto light_num = static_cast<int>(pointlights.size());
        VkDeviceSize bufferSize = sizeof(PointLight) * MAX_POINT_LIGHT_COUNT + sizeof(glm::vec4);

        for (int i = 0; i < light_num; i++)
        {
            pointlights[i].pos += glm::vec3(0, 3.0f, 0) * deltatime;
            if (pointlights[i].pos.y > getGlobalTestSceneConfiguration().max_light_pos.y)
            {
                pointlights[i].pos.y -= (getGlobalTestSceneConfiguration().max_light_pos.y - getGlobalTestSceneConfiguration().min_light_pos.y);
            }
        }

        auto pointlights_size = sizeof(PointLight) * pointlights.size();
        void* data;
        vkMapMemory(graphics_device, lights_staging_buffer_memory.get(), 0, pointlight_buffer_size, 0, &data);
        memcpy(data, &light_num, sizeof(int));
        memcpy((char*)data + sizeof(glm::vec4), pointlights.data(), pointlights_size); //< alignment
        vkUnmapMemory(graphics_device, lights_staging_buffer_memory.get());
        utility.copyBuffer(lights_staging_buffer.get(), pointlight_buffer.get(), pointlight_buffer_size);
    }
    util::writeLog("update ubo --> camera ubo: {map => copy => unmap, upload to GPU by a staging buffer}\n"
            "update ubo --> point light ubo: update light position randomly. {map => copy => unmap, upload to GPU by a staging buffer}\n");
}

const uint64_t ACQUIRE_NEXT_IMAGE_TIMEOUT{ std::numeric_limits<uint64_t>::max() };

void _VulkanRenderer_Impl::drawFrame()
{
    // 1. Acquiring an image from the swap chain
    uint32_t image_index;
    {
        auto aquiring_result = vkAcquireNextImageKHR(graphics_device, swap_chain.get()
            , ACQUIRE_NEXT_IMAGE_TIMEOUT, image_available_semaphore.get(), VK_NULL_HANDLE, &image_index);

        if (aquiring_result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // when swap chain needs recreation
            recreateSwapChain();
            return;
        }
        else if (aquiring_result != VK_SUCCESS && aquiring_result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }
    }

    // submit depth pre-pass command buffer
    {
        vk::SubmitInfo submit_info = {
            0, // waitSemaphoreCount
            nullptr, // pWaitSemaphores
            nullptr, // pwaitDstStageMask
            1, // commandBufferCount
            &depth_prepass_command_buffer, // pCommandBuffers
            1, // singalSemaphoreCount
            depth_prepass_finished_semaphore.data() // pSingalSemaphores
        };
        graphics_queue.submit(1, &submit_info, nullptr);
    }

    // submit light culling command buffer
    {
        vk::Semaphore wait_semaphores[] = { depth_prepass_finished_semaphore.get() }; // which semaphore to wait
        vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eComputeShader }; // which stage to execute
        vk::SubmitInfo submit_info = {
            1, // waitSemaphoreCount
            wait_semaphores, // pWaitSemaphores
            wait_stages, // pwaitDstStageMask
            1, // commandBufferCount
            &light_culling_command_buffer, // pCommandBuffers
            1, // singalSemaphoreCount
            lightculling_completed_semaphore.data() // pSingalSemaphores
        };
        compute_queue.submit(1, &submit_info, nullptr);
    }

    // 2. Submitting the command buffer
    {
        VkSemaphore wait_semaphores[]   = { image_available_semaphore.get() , lightculling_completed_semaphore.get() }; // which semaphore to wait
        VkSemaphore signal_semaphores[] = { render_finished_semaphore.get() };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT }; // which stage to execute

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 2;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffers[image_index];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        auto submit_result = vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
        if (submit_result != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }
    }
    // TODO: use Fence and we can have cpu start working at a earlier time

    // 3. Submitting the result back to the swap chain to show it on screen
    {
        VkSwapchainKHR swapChains[] = { swap_chain.get() };
        VkSemaphore present_wait_semaphores[] = { render_finished_semaphore.get() };

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = present_wait_semaphores;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapChains;
        present_info.pImageIndices = &image_index;
        present_info.pResults = nullptr; // Optional, check for if every single chains is successful

        VkResult present_result = vkQueuePresentKHR(present_queue, &present_info);

        if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
        {
            recreateSwapChain();
        }
        else if (present_result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swap chain image!");
        }
    }
    static uint64_t numFrames = 0;
    util::writeLog("frame %llu:\n"
        "\t draw a frame --> acquire an image from swap chain.[wait swapchain image available]\n"
        "\t draw a frame --> submit early depth pass.[signal early depth pass done]\n"
        "\t draw a frame --> submit light culling pass(compute). [wait early depth pass done]\n"
        "\t draw a frame --> submit main pass. [wait swapchain image done, light culling done][signal main pass done]\n"
        "\t draw a frame --> present. [wait main pass done]\n"
        , numFrames++
    );

}

VRaii<VkShaderModule> _VulkanRenderer_Impl::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = (uint32_t*)code.data();

    VkShaderModule temp_sm;
    auto result = vkCreateShaderModule(graphics_device, &create_info, nullptr, &temp_sm);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }
    return VRaii<VkShaderModule>(
        temp_sm,
        [device = this->device](auto& obj)
        {
            device.destroyShaderModule(obj);
        }
    );
}


VulkanRenderer::VulkanRenderer(GLFWwindow * window)
    :p_impl(std::make_unique<_VulkanRenderer_Impl>(window))
{}

VulkanRenderer::~VulkanRenderer() = default;

int VulkanRenderer::getDebugViewIndex() const
{
    return p_impl->getDebugViewIndex();
}

void VulkanRenderer::resize(int width, int height)
{
    p_impl->resize(width, height);
}

void VulkanRenderer::changeDebugViewIndex(int target_view)
{
    p_impl->changeDebugViewIndex(target_view);
}

void VulkanRenderer::requestDraw(float deltatime)
{
    p_impl->requestDraw(deltatime);
}

void VulkanRenderer::cleanUp()
{
    p_impl->cleanUp();
}

void VulkanRenderer::setCamera(const glm::mat4 & view, const glm::vec3 campos)
{
    p_impl->setCamera(view, campos);
}

/* Exercise instance setup without a display or physical GPU. The intercepted
 * creation call captures the real request and fails before surface creation. */
#define SDL_MAIN_HANDLED
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <string.h>

static int advertised, duplicate, retry, enumeration_error, create_calls;
static VkInstanceCreateFlags captured_flags;
static unsigned captured_portability;
static int device_present, device_retry, device_error;

static VkResult VKAPI_CALL mock_device_extensions(VkPhysicalDevice device, const char *layer,
    uint32_t *count, VkExtensionProperties *properties)
{
    (void)device;
    assert(!layer);
    if (device_error) return VK_ERROR_OUT_OF_HOST_MEMORY;
    *count = device_present ? 1u : 0u;
    if (properties && device_retry) { device_retry = 0; return VK_INCOMPLETE; }
    if (properties && device_present)
        strcpy(properties[0].extensionName, "VK_KHR_portability_subset");
    return VK_SUCCESS;
}

static SDL_bool mock_sdl_extensions(SDL_Window *window, unsigned *count, const char **names)
{
    (void)window;
    if (names) {
        names[0] = VK_KHR_SURFACE_EXTENSION_NAME;
        if (duplicate) names[1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    }
    *count = duplicate ? 2u : 1u;
    return SDL_TRUE;
}

static VkResult VKAPI_CALL mock_instance_extensions(const char *layer, uint32_t *count,
                                                    VkExtensionProperties *properties)
{
    assert(!layer);
    if (enumeration_error) return VK_ERROR_OUT_OF_HOST_MEMORY;
    *count = advertised ? 1u : 0u;
    if (properties && retry) { retry = 0; return VK_INCOMPLETE; }
    if (properties && advertised)
        strcpy(properties[0].extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL mock_create_instance(const VkInstanceCreateInfo *info,
    const VkAllocationCallbacks *allocator, VkInstance *instance)
{
    (void)allocator; (void)instance;
    create_calls++;
    captured_flags = info->flags;
    captured_portability = 0;
    for (uint32_t i = 0; i < info->enabledExtensionCount; i++)
        if (!strcmp(info->ppEnabledExtensionNames[i], VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
            captured_portability++;
    return VK_ERROR_INITIALIZATION_FAILED;
}

#define SDL_Vulkan_GetInstanceExtensions mock_sdl_extensions
#define vkEnumerateInstanceExtensionProperties mock_instance_extensions
#define vkCreateInstance mock_create_instance
#define vkEnumerateDeviceExtensionProperties mock_device_extensions
#include "../runner/src/vulkan_renderer.c"
#undef SDL_Vulkan_GetInstanceExtensions
#undef vkEnumerateInstanceExtensionProperties
#undef vkCreateInstance
#undef vkEnumerateDeviceExtensionProperties

static void check_request(int present, int already_present, int incomplete)
{
    char error[256];
    advertised = present; duplicate = already_present; retry = incomplete;
    enumeration_error = 0; create_calls = 0;
    assert(!saturn_vk_create(NULL, NULL, error, sizeof error));
    assert(create_calls == 1);
    assert(captured_portability == (unsigned)present);
    assert(!!(captured_flags & VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR) == present);
    assert(strstr(error, "vkCreateInstance"));
    assert(!retry);
}

int main(void)
{
    check_request(0, 0, 0);
    check_request(1, 0, 0);
    check_request(1, 1, 0);
    check_request(1, 0, 1);
    char error[256];
    enumeration_error = 1; create_calls = 0;
    assert(!saturn_vk_create(NULL, NULL, error, sizeof error));
    assert(!create_calls);
    assert(strstr(error, "extension count failed"));
    assert(!has_device_extension(VK_NULL_HANDLE, "VK_KHR_portability_subset"));
    device_present = 1; device_retry = 1;
    assert(has_device_extension(VK_NULL_HANDLE, "VK_KHR_portability_subset"));
    assert(!device_retry);
    assert(!has_device_extension(VK_NULL_HANDLE, VK_KHR_SWAPCHAIN_EXTENSION_NAME));
    device_error = 1;
    assert(!has_device_extension(VK_NULL_HANDLE, "VK_KHR_portability_subset"));
    puts("Vulkan portability instance setup passed");
    return 0;
}

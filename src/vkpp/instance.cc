#include <vkpp/instance.hh>

#include <vkpp/exception.hh>

namespace vkpp {
    Instance::Instance(const Application& application_information,
                       const std::vector<Layer> required_layers,
                       const std::vector<Extension> required_extensions,
                       DebugMessenger::Callback debug_callback)
                      : application_info { application_information },
                        enabled_layers { required_layers },
                        enabled_extensions { required_extensions } {
        VkInstanceCreateInfo create_info;
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pNext = nullptr;
        create_info.flags = 0;

        VkApplicationInfo app_info;
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext = nullptr;
        app_info.pApplicationName = application_info.name.c_str();
        app_info.applicationVersion = application_info.app_version;
        app_info.pEngineName = application_info.engine_name.c_str();
        app_info.engineVersion = application_info.engine_version;
        app_info.apiVersion = application_info.api_version;

        auto missing_layers = find(required_layers);

        if (missing_layers.size() != 0) {
            auto missing = collapse(missing_layers);
            throw Exception { "couldn't create instance!",
            "the layers(s): " + missing + "are missing!"};
        }

        auto missing_extensions = find(required_extensions);

        if (missing_extensions.size() != 0) {
            auto missing = collapse(missing_extensions);
            throw Exception { "couldn't create instance!",
            "extension(s): " + missing + "are missing!"};
        }

        // Copy over the layer/extension names to packed name arrays.

        std::vector<const char*> layer_names(required_layers.size());
        for (std::size_t i { 0 }; i < layer_names.size(); ++i)
            layer_names[i] = required_layers[i].name.c_str();

        std::vector<const char*> extension_names(required_extensions.size());
        for (std::size_t i { 0 }; i < extension_names.size(); ++i)
            extension_names[i] = required_extensions[i].name.c_str();

        auto extension_count = (std::uint32_t) extension_names.size();
        auto layer_count = (std::uint32_t) layer_names.size();

        create_info.enabledExtensionCount = extension_count;
        create_info.ppEnabledExtensionNames = extension_names.data();
        create_info.enabledLayerCount = layer_count;
        create_info.ppEnabledLayerNames = layer_names.data();
        create_info.pApplicationInfo = &app_info;

        if (VkResult error = vkCreateInstance(&create_info, nullptr, &handle))
            throw Exception { error, "couldn't create instance!" };

        if (find({ "VK_EXT_debug_utils" }, required_extensions).size() == 0)
        if (find({ "VK_LAYER_LUNARG_standard_validation" }, required_layers).size() == 0)
            debug_utils_messenger = std::move(DebugMessenger { handle, debug_callback });
    }

    Instance::~Instance() noexcept {
        debug_utils_messenger.destroy();
        if (handle != VK_NULL_HANDLE) {
            vkDestroyInstance(handle, nullptr);
        }
    }

    Instance::Instance(Instance&& instance) noexcept {
        swap(*this, instance);
    }

    Instance& Instance::operator=(Instance&& instance) noexcept {
        swap(*this, instance);
        return *this;
    }

    void swap(Instance& lhs, Instance& rhs) {
        using std::swap;
        swap(lhs.application_info, rhs.application_info);
        swap(lhs.enabled_layers, rhs.enabled_layers);
        swap(lhs.enabled_extensions, rhs.enabled_extensions);
        swap(lhs.handle, rhs.handle);
        swap(lhs.debug_utils_messenger,
             rhs.debug_utils_messenger);
    }

    VkInstance& Instance::get_handle() {
        return handle;
    }

    std::vector<Layer> Instance::find(const std::vector<Layer>& layers) const {
        return find(layers, get_available_layers());
    }

    std::vector<Extension> Instance::find(const std::vector<Extension>& extensions) const {
        return find(extensions, get_available_extensions());
    }

    std::vector<Extension> Instance::find(const std::vector<Extension>& extensions,
                                          const std::vector<Extension>& available) const {
        std::vector<Extension> missing_extensions;

        for (const auto extension : extensions) {
            bool extension_found { false };
            for (const auto available_extension : available) {
                if (extension == available_extension)
                    extension_found = true;
            }

            if (!extension_found)
                missing_extensions.push_back(extension);
        }

        return missing_extensions;
    }

    std::vector<Layer> Instance::find(const std::vector<Layer>& layers,
                                      const std::vector<Layer>& available) const {
        std::vector<Layer> missing_layers;

        for (const auto layer : layers) {
            bool layer_found { false };
            for (const auto available_layer : available) {
                if (layer == available_layer)
                    layer_found = true;
            }

            if (!layer_found)
                missing_layers.push_back(layer);
        }

        return missing_layers;
    }

    const Application& Instance::get_application() const {
        return application_info;
    }

    const std::vector<Layer>& Instance::get_enabled_layers() const {
        return enabled_layers;
    }

    const std::vector<Extension>& Instance::get_enabled_extensions() const {
        return enabled_extensions;
    }

    DebugMessenger& Instance::get_debug_messenger() {
        return debug_utils_messenger;
    }

    Version Instance::get_api_version() {
        std::uint32_t api_version;
        vkEnumerateInstanceVersion(&api_version);
        return api_version;
    }

    const std::vector<Layer>& Instance::get_available_layers() {
        if (!available_layers.empty())
            return available_layers;

        std::uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> layer_properties(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, layer_properties.data());

        available_layers.resize(layer_properties.size());
        for (std::size_t i { 0 }; i < available_layers.size(); ++i)
            available_layers[i] = layer_properties[i];

        return available_layers;
    }

    const std::vector<Extension>& Instance::get_available_extensions() {
        if (!available_extensions.empty())
            return available_extensions;

        std::uint32_t extension_count;
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> extension_properties(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                               extension_properties.data());

        available_extensions.resize(extension_properties.size());
        for (std::size_t i { 0 }; i < available_extensions.size(); ++i)
            available_extensions[i] = extension_properties[i];

        return available_extensions;
    }

    std::string Instance::collapse(const std::vector<Extension>& exts) {
        std::string extension_string;
        for (const auto extension : exts)
            extension_string.append(extension.name + " ");
        return extension_string;
    }

    std::string Instance::collapse(const std::vector<Layer>& layers) {
        std::string layer_string;
        for (const auto layer : layers)
            layer_string.append(layer.name + " ");
        return layer_string;
    }

    // Cache the available layers plus extensions.
    std::vector<Layer> Instance::available_layers;
    std::vector<Extension> Instance::available_extensions;
}

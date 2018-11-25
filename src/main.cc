#include <vkhr/arg_parser.hh>
#include <vkhr/scene_graph.hh>
#include <vkhr/paths.hh>
#include <vkhr/image.hh>
#include <vkhr/window.hh>
#include <vkhr/input_map.hh>
#include <vkhr/rasterizer.hh>
#include <vkhr/ray_tracer.hh>

#include <glm/glm.hpp>

int main(int argc, char** argv) {
    vkhr::ArgParser argp { vkhr::arguments };
    auto scene_file = argp.parse(argc, argv);

    if (scene_file.empty()) scene_file = SCENE("ponytail.vkhr");

    vkhr::SceneGraph scene_graph { scene_file };
    auto& camera { (scene_graph.get_camera()) };

    int width  = argp["x"].value.integer,
        height = argp["y"].value.integer;

    camera.set_resolution(width, height);

    vkhr::Raytracer ray_tracer { scene_graph };

    const vkhr::Image vulkan_icon { IMAGE("vulkan-icon.png") };
    vkhr::Window window { width, height, "VKHR", vulkan_icon };

    if (argp["fullscreen"].value.boolean)
        window.toggle_fullscreen();

    vkhr::InputMap input_map { window };

    input_map.bind("quit", vkhr::Input::Key::Escape);
    input_map.bind("grab", vkhr::Input::MouseButton::Left);
    input_map.bind("switch_renderer", vkhr::Input::Key::Tab);
    input_map.bind("take_screenshot", vkhr::Input::Key::S);
    input_map.bind("toggle_ui", vkhr::Input::Key::U);
    input_map.bind("recompile", vkhr::Input::Key::R);

    bool vsync_enabled = argp["vsync"].value.boolean;

    vkhr::Rasterizer rasterizer { window, scene_graph, vsync_enabled };

    if (argp["ui"].value.boolean == 0)
        rasterizer.get_imgui().hide();

    window.show();

    while (window.is_open()) {
        if (input_map.just_pressed("quit")) {
            window.close();
        } else if (input_map.just_pressed("toggle_ui")) {
            rasterizer.get_imgui().toggle_visibility();
        } else if (input_map.just_pressed("switch_renderer")) {
            rasterizer.get_imgui().toggle_raytracing();
        } else if (input_map.just_pressed("take_screenshot")) {
            rasterizer.get_screenshot().save("render.png");
        } else if (input_map.just_pressed("recompile")) {
            rasterizer.recompile_spirv();
        }

        camera.control(input_map, window.update_delta_time(),
                       rasterizer.get_imgui().wants_focus());

        scene_graph.traverse_nodes();

        rasterizer.get_imgui().update(scene_graph);

        if (rasterizer.get_imgui().do_raytrace()) {
            ray_tracer.draw(scene_graph);
            auto& framebuffer = ray_tracer.get_framebuffer();
            rasterizer.draw(framebuffer);
        } else {
            rasterizer.draw(scene_graph);
        }

        window.poll_events();
    }

    return 0;
}

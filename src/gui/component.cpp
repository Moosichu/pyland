#include "component.hpp"
#include <mutex>
#include <vector>
int Component::get_new_id() {
    static int next_component_id = 1;

    //Lock on the shared variable
    std::lock_guard<std::mutex> lock(component_mutex);

    return next_component_id++;
}

Component::Component() : vertex_data(nullptr), texture_data(nullptr),
                         size_vertex_data(0), size_texture_data(0),
                         id(0), width(0.0f), height(0.0f),
                         width_pixels(0),height_pixels(0),
                         x_offset(0.0f), y_offset(0.0f),
                         x_offset_pixels(0), y_offset_pixels(0),
                         clickable(false), parent(nullptr)
{
    id =  get_new_id();
}

Component::~Component() {
    delete []vertex_data;
    delete []texture_data;
}

void Component::set_on_click(std::function<void (void)> func) {
    clickable = true;
    on_click_func= func;
}

void Component::clear_on_click() {
    auto function = [] () {return; };
    on_click_func = function;
    clickable = false;
}

void Component::set_width(float _width) {
    width = _width;
}

void Component::set_height(float _height) {
    height = _height;
}

const std::map<int, std::shared_ptr<Component>>& Component::get_components() {
    component_no_children_exception exception;
    throw exception;
}
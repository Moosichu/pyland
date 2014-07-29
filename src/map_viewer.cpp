#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "game_window.hpp"
#include "map.hpp"
#include "map_viewer.hpp"
#include "object.hpp"
#include "object_manager.hpp"
#include "renderable_component.hpp"

MapViewer::MapViewer(GameWindow* new_window) {
    if(new_window == nullptr) {
        LOG(ERROR) << "MapViewer::MapViewer: GameWindow should not be null";
    }
    window = new_window;

    // Set background color and clear buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    // Disable writing to the alpha channel.
    // Fixes a bug where EGL layer becomes transparent.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    // L./eave this here!!!
    //Disable back face culling.
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


MapViewer::~MapViewer() {

}

void MapViewer::render_map() {
    if(map == nullptr) {
        LOG(ERROR) << "MapViewer::render_map: Map should not be null";
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderableComponent* map_render_component = map->get_renderable_component();
    if(map_render_component == nullptr) {
        LOG(ERROR) << "MapViewer::render_map: RenderComponent should not be null ";
        return;
    }

    Shader* map_shader = map_render_component->get_shader();
    if(map_shader == nullptr) {
        LOG(ERROR) << "MapViewer::render_map: Shader should not be null";
        return;
    }

    std::pair<int, int> size = window->get_size();
    //TODO, set the map view correctly
    glScissor(0, 0, size.first, size.second);
    map->set_display_width(float(size.first) / 32.0f);
    map->set_display_height(float(size.second) / 32.0f);
    glViewport(0, 0, size.first, size.second);
    refocus_map();
    glm::mat4 projection_matrix = glm::ortho(0.0f, float(size.first), 0.0f, float(size.second), 0.0f, 1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 translate = glm::vec3(-map->get_display_x()*32.0f, -map->get_display_y()*32.0f, 0.0f);
    glm::mat4 translated = glm::translate(model, translate);
  


    map_render_component->set_projection_matrix(projection_matrix);
    map_render_component->set_modelview_matrix(translated);

    map_render_component->bind_shader();
  

    //TODO: I don't want to actually expose the shader, put these into wrappers in the shader object
    glUniformMatrix4fv(glGetUniformLocation(map_shader->get_program(), "mat_projection"), 1, GL_FALSE,glm::value_ptr(map_render_component->get_projection_matrix()));
    glUniformMatrix4fv(glGetUniformLocation(map_shader->get_program(), "mat_modelview"), 1, GL_FALSE, glm::value_ptr(map_render_component->get_modelview_matrix()));

    //Bind the vertex buffers and textures
    map_render_component->bind_vbos();
    map_render_component->bind_textures();


    glDrawArrays(GL_TRIANGLES, 0, map_render_component->get_num_vertices_render());
 
    //Release the vertex buffers and textures
    map_render_component->release_textures();
    map_render_component->release_vbos();

    map_render_component->release_shader();





    //Draw the characters
    const std::vector<int>& characters = map->get_characters();
    ObjectManager& object_manager = ObjectManager::get_instance();
    for(auto it = characters.begin(); it != characters.end(); ++it) {
        if(*it != 0) {
            std::shared_ptr<Object> sprite = object_manager.get_object<Object>(*it);
    
            RenderableComponent* character_render_component = sprite->get_renderable_component();
    
            //Move sprite to the required position
            glm::mat4 model1 = glm::mat4(1.0f);
            glm::vec3 translate1 = glm::vec3(
                (float(sprite->get_x_position()) - map->get_display_x()) * 32.0f,
                (float(sprite->get_y_position()) - map->get_display_y()) * 32.0f,
                0.0f
            );
            glm::mat4 translated1 = glm::translate(model1, translate1);
            character_render_component->set_modelview_matrix(translated1);
            character_render_component->set_projection_matrix(projection_matrix);

            character_render_component->bind_shader();

            Shader* shader = character_render_component->get_shader();
            if(shader == nullptr) {
                LOG(ERROR) << "MapViewer::render_map: Shader (character_render_component->get_shader()) should not be null";
                return;
            }

            //TODO: I don't want to actually expose the shader, put these into wrappers in the shader object
            glUniformMatrix4fv(glGetUniformLocation(shader->get_program(), "mat_projection"), 1, GL_FALSE,glm::value_ptr(character_render_component->get_projection_matrix()));

            glUniformMatrix4fv(glGetUniformLocation(shader->get_program(), "mat_modelview"), 1, GL_FALSE, glm::value_ptr(character_render_component->get_modelview_matrix()));

            character_render_component->bind_vbos();
            character_render_component->bind_textures();
            // LOG(INFO) << " X " << sprite->get_x_position()*32.0f << " Y " << sprite->get_y_position()*32.0f;
            glDrawArrays(GL_TRIANGLES, 0, character_render_component->get_num_vertices_render());

            character_render_component->release_textures();
            character_render_component->release_vbos();
            character_render_component->release_shader();
        }
    }

    window->swap_buffers();
}

/// 
/// Take a line of a given size (length) and a point offset on that line (point):
/// 
/// ← length    →
/// ├───────•───┤
/// ← point →
/// 
/// Also takes a display of a given size (bound):
/// 
/// ← bound→
/// ┼─────────────┼
/// 
/// If bound == length:
/// 
///     It places the boxes over eachother:
/// 
///     ┼─────────────┼
///     ├─────────•───┤
///
/// If length > bound:
/// 
///     It centres the box on the point:
/// 
///            ┼─────────────┼
///     ├─────────────•───┤
/// 
///     Then moves the box inside the bounds, if needed:
/// 
///         ┼─────────────┼
///     ├─────────────•───┤
/// 
/// If bound > length:
/// 
///     It centres the line inside the box:
/// 
///     ┼─────────────┼
///           |•────────┤
/// 
///     It then moves the line inside the box, if needed:
/// 
///     ┼─────────────┼
///         |•────────┤
/// 
/// Then it returns the distance from the start of length to the start of bound:
/// 
///     For example,
/// 
///         ────→
///             ┼─────────────┼
///         ├─────────────•───┤
/// 
///     which is positive, or
/// 
///         ←────
///         ┼─────────────┼
///             |•────────┤
/// 
///     which is negative.
/// 

float centre_point_in_range(float point, float length, float bound) {
    // First case is a subset of the other two
    // and both cases have same first step.
    float bound_offset = point - bound / 2.0f;

    // Crop to valid range: bound inside length or length inside bound
    // Note order of min/max
    if (length > bound) {
        // bound_offset positive by no more than | length - bound |
        bound_offset = std::min(std::max(bound_offset, 0.0f), length - bound);
    }
    else if (bound > length) {
        // bound_offset negative by no more than | length - bound |
        bound_offset = std::max(std::min(bound_offset, 0.0f), length - bound);
    }

    return bound_offset;
}

void MapViewer::refocus_map() {
    //Get the object
    ObjectManager& object_manager = ObjectManager::get_instance();

    if(map_focus_object == 0) {
        LOG(INFO) << "MapViewer::refocus_map: No focus.";
        return;
    }
        
    std::shared_ptr<Object> object = object_manager.get_object<Object>(map_focus_object);

    //If such an object exists, move the map to it
    if(object) {
        map->set_display_x(centre_point_in_range(
            // half-tile offset to take centre of character
            /* point  */ float(object->get_x_position()) + 0.5f,
            /* length */ float(map->get_width()),
            /* bound  */ map->get_display_width()
        ));

        map->set_display_y(centre_point_in_range(
            // half-tile offset to take centre of character
            /* point  */ float(object->get_y_position()) + 0.5f,
            /* length */ float(map->get_height()),
            /* bound  */ map->get_display_height()
        ));
    } else {
        LOG(INFO) << "MapViewer::refocus_map: No objects have focus.";
    }
}

void MapViewer::set_map(Map* new_map) {
    map = new_map;
}

void MapViewer::set_map_focus_object(int object_id) { 
    //Set the focus to the object if this is a valid object and it is on the map
    if(ObjectManager::is_valid_object_id(object_id)) {
        //        const std::vector<int>& characters = map->get_characters();
        map_focus_object = object_id;
        refocus_map();

        //TODO: add this in again
        //If the object is on the map
        /*        if(std::find(characters.begin(), characters.end(),object_id) != characters.end()) {
            //focus on it
            map_focus_object = object_id;
            }*/
    }
    else {
        LOG(ERROR) << "MapViewer::set_map_focus_object: Invalid focus object";
    }
}


Map* MapViewer::get_map() {
    return map;
}

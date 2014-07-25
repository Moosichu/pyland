#include "gui_manager.hpp"

#include <new>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#ifdef USE_GLES

#include <GLES2/gl2.h>

#endif

#ifdef USE_GL
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif


bool GUIManager::parse_components() {


    //Now generate the needed rendering data


    generate_tex_data();
    generate_vertex_data();
    load_textures();

}

void GUIManager::update_components() {
    init_shaders();
}



GUIManager::GUIManager() {
}

GUIManager::~GUIManager() {
    delete []gui_tex_data;
    delete []gui_data;
    delete []tex_buf;
}


void GUIManager::generate_tex_data() {
    //holds the texture data
    //need 12 float for the 2D texture coordinates
    gui_tex_data = new GLfloat[sizeof(GLfloat)*num_floats]; 

    //TODO: this is a remnant from C, update to catch exception
    if(gui_tex_data == NULL) {
        std::cerr << "ERROR in GUIManager::generate_tex_data, cannot allocate memory" << std::endl;
        return;
    }

//Generate the data
    renderable_component.set_texture_coords_data(gui_tex_data, sizeof(GLfloat)*num_floats, false);
} 

void GUIManager::generate_vertex_data() {



    //TODO: this is a remnant from C, update to catch exception
    if(gui_data == NULL) {
        std::cerr << "ERROR in Characater::generate_vertex_data, cannot allocate memory" << std::endl;
        return;
    }

    //generate the vertex data
    std::vector<std::pair<std::shared_ptr<GLfloat>, int>> components_data = root->generate_vertex_data();

    //calculate data size
    long num_floats = 0;
    for(auto component_vertex_data : components_data) {
        num_floats += component_vertex_data.second;
    }

    //Create a buffer for the data
    gui_data  = new GLfloat[sizeof(GLfloat)*num_floats]; 
    
    //Extract the data
    for(auto component_vertex_data : components_data) {
        std::shared_ptr<GLfloat> vertices = component_vertex_data.first;


        //copy data into buffer
        
    }

    
    renderable_component.set_vertex_data(gui_data,sizeof(GLfloat)*num_floats, false);
    renderable_component.set_num_vertices_render(num_floats/3);//GL_TRIANGLES being used
}

void GUIManager::load_textures() {

    /*
    FILE *tex_file2 = NULL;
    size_t bytes_read = 0;
    size_t image_sz_2 = IMAGE2_SIZE_WIDTH*IMAGE2_SIZE_HEIGHT*IMAGE2_NUM_COMPONENTS;

    tex_buf = new char[image_sz_2];

    tex_file2 = fopen("../resources/characters_1.raw", "rb");
    if(tex_file2 == NULL) {
        std::cerr << "ERROR: Couldn't load textures" << std::endl;
    }

    if (tex_file2 && tex_buf) {
        bytes_read = fread(tex_buf, 1, image_sz_2, tex_file2);
        assert(bytes_read == image_sz_2);  // some problem with file?
        fclose(tex_file2);
        }*/
    //Set the texture data in the rederable component
    //    renderable_component.set_texture_data(tex_buf, static_cast<int>(image_sz_2), IMAGE2_SIZE_WIDTH, IMAGE2_SIZE_HEIGHT, false);

}
bool GUIManager::init_shaders() {
#ifdef USE_GLES
    //read in the shaders
    std::ifstream vertex_shader_src("vert_shader.glesv");
    std::ifstream fragment_shader_src("frag_shader.glesf");
#endif
#ifdef USE_GL
    //read in the shaders
    std::ifstream vertex_shader_src("vert_shader.glv");
    std::ifstream fragment_shader_src("frag_shader.glf");
#endif

    if (!vertex_shader_src.good()){
        std::cerr << "Failed to load vertex shader" << std::endl;
        return false;
    }
    
    if (!fragment_shader_src.good()) {
        std::cerr << "Failed to load fragment shader" << std::endl;
        return false;
    }

    std::string vert_src, frag_src, line;
    while (getline(vertex_shader_src, line)) {
        vert_src += line + "\n";
    }

    while (getline(fragment_shader_src, line)) {
        frag_src += line + "\n";
    }

    Shader* shader = new Shader(vert_src, frag_src);
  
    if (!shader->is_loaded()) {
        delete shader;
        shader = NULL;
        std::cerr << "Failed to create the shader" << std::endl;
        return false;
    }

    //Set the shader
    renderable_component.set_shader(shader);

    return true;

}





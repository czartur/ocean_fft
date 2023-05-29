#pragma once


#include "cgp/cgp.hpp"
#include "cgp_custom.hpp"
#include "environment.hpp"

using cgp::mesh_drawable;

struct gui_parameters {
	bool display_frame = false;
	bool display_wireframe = false;
	bool dn_cycle = false;
	float fog_dmax = 150.f;
	float wind_magnitude = 40.f;
	float wind_angle = 45.f;
	float choppiness = 1.5f;
};

// The structure of the custom scene
struct scene_structure : cgp::scene_inputs_generic {
	
	// ****************************** //
	// Elements and shapes of the scene
	// ****************************** //
	camera_controller_first_person_custom camera_control;
	camera_projection_perspective camera_projection;
	window_structure window;
	window_structure debug_window;

	mesh_drawable global_frame;          // The standard global frame
	environment_structure environment;   // Standard environment controler
	input_devices inputs;                // Storage for inputs status (mouse, keyboard, window dimension)
	gui_parameters gui;                  // Standard GUI element storage
	
	// ****************************** //
	// Elements and shapes of the scene
	// ****************************** //

	// scene_elements_structure scene_elements;
	timer_basic timer;
	mesh_drawable terrain;
	mesh_drawable water, water_lq;
	mesh_drawable sun;

	std::vector<vec3> neighbors;

	// compute shaders 
	opengl_shader_structure_custom spectrum_0, spectrum_t, fft_horizontal, fft_vertical, normal, orientation;
	
	// vert / frag shaders
	opengl_shader_structure ocean;
 
	// textures
	opengl_texture_image_structure_custom spectrum_0_image, spectrum_t_image, temp_image, normal_image, displacement_image, dx_image, dz_image, dy_image, debug_image;
	opengl_texture_image_structure_custom gaussian_noise;

	// utility uniform
	uniform_generic_structure_custom input;

	bool compute_initial_spectrum = true;
	
	// debug meshs
	mesh_drawable debug_x, debug_y, debug_z; 
	
	// ****************************** //
	// Functions
	// ****************************** //

	void initial_spectrum();
	void fft(opengl_shader_structure_custom &shader, opengl_texture_image_structure_custom &texture);
	void spectrum_update();
	void normal_update();
	void texture_ordering(opengl_texture_image_structure_custom &input_image, opengl_texture_image_structure_custom &output_image);


	void initialize();    // Standard initialization to be called before the animation loop
	void display_frame(); // The frame display to be called within the animation loop
	void display_gui();   // The display of the GUI, also called within the animation loop

	void mouse_move_event();
	void mouse_click_event();
	void keyboard_event();
	void idle_frame();

};






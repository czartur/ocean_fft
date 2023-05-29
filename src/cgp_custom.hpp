#pragma once

#include "cgp/cgp.hpp"

using namespace cgp;

struct camera_controller_first_person_custom : camera_controller_first_person {
	bool is_game_mode() { return is_cursor_trapped; }
	void idle_frame(mat4& camera_matrix_view)
	{
		// Preconditions	
		assert_cgp_no_msg(inputs != nullptr);
		assert_cgp_no_msg(window != nullptr);
		if (!is_active) return;

		float const magnitude = 8*inputs->time_interval;
		float const angle_magnitude = 0.7*inputs->time_interval;


		if (inputs->keyboard.up || inputs->keyboard.is_pressed(GLFW_KEY_W))
			camera_model.manipulator_translate_front(magnitude);           // move front
		if (inputs->keyboard.down || inputs->keyboard.is_pressed(GLFW_KEY_S))
			camera_model.manipulator_translate_front(-magnitude);          // move back
		
		if (inputs->keyboard.left || inputs->keyboard.is_pressed(GLFW_KEY_A))
			camera_model.manipulator_translate_in_plane({ magnitude ,0 }); // move left
		if (inputs->keyboard.right || inputs->keyboard.is_pressed(GLFW_KEY_D))
			camera_model.manipulator_translate_in_plane({ -magnitude ,0 }); // move right

		if (inputs->keyboard.is_pressed(GLFW_KEY_R))
			camera_model.manipulator_translate_in_plane({ 0 , -magnitude }); // up
		if (inputs->keyboard.is_pressed(GLFW_KEY_F))
			camera_model.manipulator_translate_in_plane({ 0 , magnitude }); // down

		if (inputs->keyboard.is_pressed(GLFW_KEY_Q))
			camera_model.manipulator_rotate_roll_pitch_yaw(angle_magnitude, 0, 0);  // twist left
		if (inputs->keyboard.is_pressed(GLFW_KEY_E))
			camera_model.manipulator_rotate_roll_pitch_yaw(-angle_magnitude, 0, 0);  // twist right


		camera_matrix_view = camera_model.matrix_view();
	}
};

struct opengl_shader_structure_custom : opengl_shader_structure {
	// COMPUTE SHADER 
	void load(std::string const& compute_shader_path);
};

struct opengl_texture_image_structure_custom : opengl_texture_image_structure {
	// Initialize a GL_TEXTURE_2D from data
	void initialize_texture_2d_on_gpu(int width_arg, int height_arg, GLint format_arg=GL_RGB8, GLenum texture_type_arg= GL_TEXTURE_2D, GLint wrap_s= GL_CLAMP_TO_EDGE, GLint wrap_t= GL_CLAMP_TO_EDGE, GLint texture_mag_filter= GL_LINEAR, GLint texture_min_filter= GL_LINEAR, const GLvoid* data = 0);
};

struct uniform_generic_structure_custom : uniform_generic_structure {
	// clear common uniforms
	void clear(){ uniform_int.clear(); uniform_float.clear(); uniform_vec2.clear(); uniform_vec3.clear(); uniform_mat4.clear(); };
};
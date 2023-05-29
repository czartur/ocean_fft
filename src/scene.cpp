#include "scene.hpp"
#include <random>

using namespace cgp;

#define RESOLUTION 256 // must be 2^k
#define WORK_GROUP_DIM 16 // NE PAS CHANGER !!

#define PI 3.14159265359f

const int ocean_size = 512;
const int amplitude = 40;
const float scale = 0.15;
const int NUM_PATCHES = 5; // odd
const float sea_length = scale*ocean_size;

void scene_structure::initialize()
{
	camera_control.initialize(inputs, window); // Give access to the inputs and window global state to the camera controler
	camera_control.look_at({ -3.0f, 1.0f, -2.0f }, {0,0,0}, {0,1,0});
	global_frame.initialize_data_on_gpu(mesh_primitive_frame());
	
	// Set the light to the current position of the camera
	environment.light = camera_control.camera_model.position();
	environment.uniform_generic.uniform_vec3["light_color"] = {249.0/256.0, 215.0/256.0, 28.0/256.0}; 
	
	// COMPUTE SHADERS
	spectrum_0.load(project::path + "shaders/compute_shaders/spectrum_0.comp.glsl");
	spectrum_t.load(project::path + "shaders/compute_shaders/spectrum_t.comp.glsl");
	fft_horizontal.load(project::path + "shaders/compute_shaders/fft_rows.comp.glsl");
	fft_vertical.load(project::path + "shaders/compute_shaders/fft_columns.comp.glsl");
	normal.load(project::path + "shaders/compute_shaders/normal.comp.glsl");
	orientation.load(project::path + "shaders/compute_shaders/orientation.comp.glsl");
	
	// VERT / FRAG SHADERS
	ocean.load(
		project::path + "shaders/ocean/ocean.vert.glsl",
		project::path + "shaders/ocean/ocean.frag.glsl"
	);

 
	// TEXTURES
	// initial spectrum
	spectrum_0_image.initialize_texture_2d_on_gpu(RESOLUTION, RESOLUTION, GL_RGBA32F, GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST);
	// displacements and normals in each direction 
	dx_image.initialize_texture_2d_on_gpu(RESOLUTION, RESOLUTION, GL_RGBA32F, GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST);
	dy_image.initialize_texture_2d_on_gpu(RESOLUTION, RESOLUTION, GL_RGBA32F, GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST);
	dz_image.initialize_texture_2d_on_gpu(RESOLUTION, RESOLUTION, GL_RGBA32F, GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST);
	// final textures
	spectrum_t_image.initialize_texture_2d_on_gpu(RESOLUTION, RESOLUTION, GL_RGBA32F, GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST);
	normal_image.initialize_texture_2d_on_gpu(RESOLUTION, RESOLUTION, GL_RGBA32F, GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST);
	displacement_image.initialize_texture_2d_on_gpu(RESOLUTION, RESOLUTION, GL_RGBA32F, GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST);
	// utility texture
	temp_image.initialize_texture_2d_on_gpu(RESOLUTION, RESOLUTION, GL_RGBA32F, GL_TEXTURE_2D, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST);
	
	// WATER MESH
	// High Quality
	float sea_heiht = -2.0;
	float sea_l = sea_length/2.0;
	mesh sea_grid = mesh_primitive_grid({ -sea_l,-sea_l,0 }, { sea_l,-sea_l,0 }, { sea_l,sea_l,0 }, { -sea_l,sea_l,0 }, RESOLUTION, RESOLUTION);
	sea_grid.apply_rotation_to_position(vec3(1,0,0), PI/2.f).apply_translation_to_position(vec3(0,sea_heiht,0));
	sea_grid.apply_translation_to_position(vec3(sea_l, 0, sea_l));

	water.initialize_data_on_gpu(sea_grid);
	water.shader = ocean;
	water.supplementary_texture["u_normal_map"] =  normal_image;
	water.supplementary_texture["u_displacement_map"] = displacement_image;
	// water.material.texture_settings.two_sided = true;
	
	// Low Quality
	sea_grid = mesh_primitive_grid({ -sea_l,-sea_l,0 }, { sea_l,-sea_l,0 }, { sea_l,sea_l,0 }, { -sea_l,sea_l,0 }, RESOLUTION/2, RESOLUTION/2);
	sea_grid.apply_rotation_to_position(vec3(1,0,0), PI/2.f).apply_translation_to_position(vec3(0,sea_heiht,0));
	sea_grid.apply_translation_to_position(vec3(sea_l, 0, sea_l));

	water_lq.initialize_data_on_gpu(sea_grid);
	water_lq.shader = ocean;
	water_lq.supplementary_texture["u_normal_map"] =  normal_image;
	water_lq.supplementary_texture["u_displacement_map"] = displacement_image;
	// water_lq.material.texture_settings.two_sided = true;

	// Patch location of neighbors
	for(int i = -NUM_PATCHES/2; i <= NUM_PATCHES/2; ++i){
		for(int j = -NUM_PATCHES/2; j <= NUM_PATCHES/2; ++j){
			neighbors.push_back(vec3(i,0,j));
		}
	}

	// SUN MESH
	sun.initialize_data_on_gpu(mesh_primitive_sphere(5.0f));
	sun.material.color = {249.0/256.0, 215.0/256.0, 28.0/256.0};
	sun.material.phong.ambient = 1;
	sun.material.phong.diffuse = 0;
	sun.material.phong.specular = 0;
	 
	// DEBUG MESH
	mesh quad = mesh_primitive_quadrangle();
	debug_z.initialize_data_on_gpu(quad, mesh_drawable::default_shader, displacement_image);
	debug_y.initialize_data_on_gpu(quad.apply_rotation_to_position(vec3(1,0,0), PI/2.0f), mesh_drawable::default_shader, spectrum_t_image);
	debug_x.initialize_data_on_gpu(quad.apply_rotation_to_position(vec3(0,0,1), PI/2.0f), mesh_drawable::default_shader, normal_image);

	debug_z.texture = displacement_image;
	debug_y.texture = spectrum_t_image;
	debug_x.texture = normal_image;
} 

void scene_structure::display_frame()
{
	timer.update();

	// rendering ocean maps...
	if (compute_initial_spectrum)
	{
		// timer.t = 0;
		initial_spectrum();
		compute_initial_spectrum = false;
	}
 
	spectrum_update(); 
	texture_ordering(dy_image, spectrum_t_image);

	fft(fft_vertical, dy_image);
	fft(fft_horizontal, dy_image);
	fft(fft_horizontal, dx_image);
	fft(fft_vertical, dx_image);
	fft(fft_horizontal, dz_image);
	fft(fft_vertical, dz_image);

 	normal_update();
	
	
	vec3 player_position = camera_control.camera_model.position();
	
	// DRAW SUN (+ day night cycle)
	if(gui.dn_cycle){
		float rad = NUM_PATCHES/2.0 * sea_length;
		sun.model.translation = vec3(player_position.x + rad*cos(timer.t*0.1), rad*sin(timer.t*0.1) - 20.0, player_position.z);

		environment.light = sun.model.translation;
		environment.background_color = vec3(157.0,221.0,237.0)/256.0 * (std::max(0.1, sin(timer.t * 0.1)));

		// lil optimization
		if(sun.model.translation.y >= water.model.translation.y-2.0)
			draw(sun, environment);
	}


	// DRAW OCEAN
	input.uniform_int["u_resolution"] = RESOLUTION;
	input.uniform_vec3["u_bg_color"] = environment.background_color;
	input.uniform_float["u_fog_dmax"] = gui.fog_dmax;

	vec3 player_u0v = vec3(std::floor(player_position.x/sea_length), 0, std::floor(player_position.z/sea_length));
	float fov = camera_projection.field_of_view;	 
	float cosphi = -environment.camera_view[0][0];
	float sinphi = environment.camera_view[0][2];
	// float phi = (sinphi > 0 ?  acos(cosphi) : 2.0*PI - acos(cosphi))*180.0/PI;	
	// std::cout << phi << std::endl;

	float cosfov = cos(fov*1.4);
	vec2 view_cone_dir = normalize(vec2(sinphi, cosphi));
	vec2 view_point_dir = normalize(vec2(-player_position.x, -player_position.z));

	for(auto neigh : neighbors){
		vec3 corner = (player_u0v + neigh)*sea_length;
		view_point_dir = normalize(vec2(corner.x + sea_length/2.0 - player_position.x, corner.z + sea_length/2.0 - player_position.z));
		if(dot(view_cone_dir, view_point_dir) > cosfov) goto draw; 
		for(int i = 0; i < 2; ++i){
			for(int j = 0; j < 2; ++j){
				view_point_dir = normalize(vec2(corner.x + i*sea_length - player_position.x, corner.z + j*sea_length - player_position.z));
				if(dot(view_cone_dir, view_point_dir) > cosfov) goto draw;
			}
		}
		continue;
		
		draw:
		auto &agua = std::max(neigh.x, neigh.y) >= 2 || std::min(neigh.x, neigh.z) <= -2 ?  water_lq : water;
		agua.model.translation = corner;
		draw(agua, environment, 1, true, input);
		if(gui.display_wireframe) draw_wireframe(agua, environment);
	}
	input.clear();
	
	if (gui.display_frame){
		draw(global_frame, environment);
		draw(debug_x, environment);
		draw(debug_y, environment);
		draw(debug_z, environment);
	}
}

void scene_structure::display_gui()
{
	ImGui::Checkbox("Frame", &gui.display_frame);
	ImGui::Checkbox("Wireframe", &gui.display_wireframe);
	ImGui::Checkbox("Day & Night cycle", &gui.dn_cycle);
	ImGui::SliderFloat("Fog dmax", &gui.fog_dmax, 100.f, 200.f);
	bool wind_mag_changed = ImGui::SliderFloat("Wind Magnitude", &gui.wind_magnitude, 20.f, 60.f);
	bool wind_ang_changed = ImGui::SliderFloat("Wind Angle", &gui.wind_angle, 0, 359);
	ImGui::SliderFloat("Choppiness", &gui.choppiness, 0.f, 3.f);
	
	compute_initial_spectrum |= wind_ang_changed | wind_mag_changed;
}

void scene_structure::mouse_move_event()
{
	if (!inputs.keyboard.shift)
		camera_control.action_mouse_move(environment.camera_view);
}
void scene_structure::mouse_click_event()
{
	camera_control.action_mouse_click(environment.camera_view);
}
void scene_structure::keyboard_event()
{
	camera_control.action_keyboard(environment.camera_view);
}
void scene_structure::idle_frame()
{
	camera_control.idle_frame(environment.camera_view);
}

// OCEAN COMPUTATION
void scene_structure::initial_spectrum(){

	glUseProgram(spectrum_0.id);
	input.uniform_int["u_resolution"] = RESOLUTION;
	input.uniform_int["u_ocean_size"] = ocean_size; 
	input.uniform_float["u_amplitude"] = amplitude;

	float wind_angle_rad = PI*gui.wind_angle/180.f;
	input.uniform_vec2["u_wind"] = vec2(gui.wind_magnitude * cos(wind_angle_rad), gui.wind_magnitude * sin(wind_angle_rad));
	
	input.send_opengl_uniform(spectrum_0);
	input.clear();
	
	// random dist generation
	std::vector<float> gaussian_rnd(4* RESOLUTION * RESOLUTION);
	std::random_device dev;
	std::mt19937 rng(dev());
	std::normal_distribution<float> dist(0.f, 1.f); //~N(0,1)
	for (int i = 0; i < (int) gaussian_rnd.size(); ++i)
		gaussian_rnd[i] = dist(rng);
	gaussian_noise.initialize_texture_2d_on_gpu(RESOLUTION, RESOLUTION, GL_RGBA32F, GL_TEXTURE_2D, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_NEAREST, GL_NEAREST, gaussian_rnd.data());
	
	glBindImageTexture(0, spectrum_0_image.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(1, gaussian_noise.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	glDispatchCompute(RESOLUTION / WORK_GROUP_DIM, RESOLUTION / WORK_GROUP_DIM, 1);
	glFinish();
}

void scene_structure::spectrum_update(){
	glUseProgram(spectrum_t.id);
	input.uniform_int["u_resolution"] = RESOLUTION;
	input.uniform_int["u_ocean_size"] = ocean_size; 
	input.uniform_float["u_choppiness"] = gui.choppiness;
	timer.update();
	input.uniform_float["u_time"] = timer.t;
	input.send_opengl_uniform(spectrum_t);
	input.clear();

	glBindImageTexture(0, spectrum_0_image.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, dy_image.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(2, dx_image.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(3, dz_image.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glDispatchCompute(RESOLUTION / WORK_GROUP_DIM, RESOLUTION / WORK_GROUP_DIM, 1);
	glFinish();
}

void scene_structure::normal_update(){
	glUseProgram(normal.id);

	glBindImageTexture(0, dy_image.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, dx_image.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(2, dz_image.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(3, normal_image.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(4, displacement_image.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glDispatchCompute(RESOLUTION / WORK_GROUP_DIM, RESOLUTION / WORK_GROUP_DIM, 1);
	glFinish();
}

void scene_structure::fft(opengl_shader_structure_custom &shader, opengl_texture_image_structure_custom &texture){
	glUseProgram(shader.id);
	input.uniform_int["u_resolution"] = RESOLUTION; 
	input.send_opengl_uniform(shader); 
	
	auto& tmp = temp_image;

	bool swap_temp = false;
	for (int stride = 1, count = RESOLUTION; count >= 1; stride <<= 1, count >>= 1)
	{
		glBindImageTexture(0, texture.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, tmp.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		input.uniform_int["u_stride"] = stride;
	 	input.uniform_int["u_count"] = count;
		input.send_opengl_uniform(shader);

		// two calculations per shader execution
		glDispatchCompute(RESOLUTION, RESOLUTION / 2, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	
		std::swap(tmp, texture);
		swap_temp = !swap_temp;
	}
	if(swap_temp) std::swap(tmp, texture);
	input.clear();
}

// UTILITY
void scene_structure::texture_ordering(opengl_texture_image_structure_custom &input_image, opengl_texture_image_structure_custom &output_image){
	glUseProgram(orientation.id);
	input.uniform_int["u_resolution"] = RESOLUTION;
	input.send_opengl_uniform(orientation);
	input.clear();

	glBindImageTexture(0, input_image.id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glBindImageTexture(1, output_image.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glDispatchCompute(RESOLUTION / WORK_GROUP_DIM, RESOLUTION / WORK_GROUP_DIM, 1);
	glFinish();
}
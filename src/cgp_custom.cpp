#include "cgp_custom.hpp"

using namespace cgp;

// TEXTURE CUSTOM
static GLenum format_to_data_type(GLint format)
{
	switch (format)
	{
	case GL_RGB8:
	case GL_RGB32F:
		return GL_RGB;
	case GL_RGBA8:
		return GL_RGBA;
	case GL_R32F:
		return GL_RED;
	case GL_RGBA32F:
		return GL_RGBA;
	default:
		error_cgp("Unknown format");
	}
	error_cgp("Unreachable");
}
static GLenum format_to_component(GLint format)
{
	switch (format)
	{
	case GL_RGB8:
	case GL_RGBA8:
		return GL_UNSIGNED_BYTE;
	case GL_RGB32F:
		return GL_FLOAT;
	case GL_R32F:
		return GL_FLOAT;
	case GL_RGBA32F:
		return GL_FLOAT;

	default:
		error_cgp("Unknown format");
	}
	error_cgp("Unreachable");
}

template <typename TYPE>
static GLuint opengl_initialize_texture_2d_on_gpu(int width, int height, TYPE const* data,
	GLint wrap_s, GLint wrap_t,
	GLenum texture_type, GLint format, GLenum gl_format, GLenum data_type, 
	bool is_mipmap, GLint texture_mag_filter, GLint texture_min_filter)
{
	// Create texture
	GLuint id = 0;
	glGenTextures(1, &id); opengl_check;
	glBindTexture(texture_type, id); opengl_check;

	glTexImage2D(texture_type, 0, format, width, height, 0, gl_format, data_type, data); opengl_check;

	glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, wrap_s); opengl_check;
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, wrap_t); opengl_check;

	if (is_mipmap) {
		glGenerateMipmap(texture_type); opengl_check;
	}

	glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, texture_mag_filter); opengl_check;
	glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, texture_min_filter); opengl_check;
	

	glBindTexture(texture_type, 0); opengl_check;

	assert_cgp(glIsTexture(id), "Incorrect texture id");
	return id;
}

void opengl_texture_image_structure_custom::initialize_texture_2d_on_gpu(int width_arg, int height_arg, GLint format_arg, GLenum texture_type_arg, GLint wrap_s, GLint wrap_t, GLint texture_mag_filter, GLint texture_min_filter, const GLvoid* data)
{
	// Store parameters
	width = width_arg;
	height = height_arg;
	format = format_arg;
	texture_type = texture_type_arg;

	// Initialize texture data on GPU
	id = opengl_initialize_texture_2d_on_gpu(width, height, data,
		wrap_s, wrap_t, texture_type, format, format_to_data_type(format), format_to_component(format),
		false, texture_mag_filter, texture_min_filter);

}

// SHADER CUSTOM
static bool check_compilation(GLuint shader)
{
	GLint is_compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);

	// get info on compilation
	GLint maxLength = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
	std::vector<GLchar> infoLog( static_cast<size_t>(maxLength)+1 );
	glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

	if( maxLength >= 1 )
	{
		std::cout << "[Info from shader compilation]"<< std::endl;
		std::cout << &infoLog[0] << std::endl;
	}

	if( is_compiled==GL_FALSE )
	{
		std::cout << "Compilation Failed" <<std::endl;
		glDeleteShader(shader);
		return false;
	}
	return true;
}

static bool compile_shader(const GLenum shader_type, std::string const& shader_str, GLuint& shader_id)
{
	shader_id = glCreateShader(shader_type);
	assert_cgp( glIsShader(shader_id), "Error creating shader" );

	char const* const shader_cstring = shader_str.c_str();
	glShaderSource(shader_id, 1, &shader_cstring, nullptr);

	// Compile shader
	glCompileShader(shader_id);

	bool valid = check_compilation(shader_id);
	return valid;
}

// COMPUTE SHADER (only from path)
    GLuint opengl_load_shader(std::string const& compute_shader_path);

// COMPUTE SHADER (only from path)
void opengl_shader_structure_custom::load(std::string const& compute_shader_path){
	id = opengl_load_shader(compute_shader_path);
}

GLuint opengl_load_shader(std::string const& compute_shader_path){
	// Check the file are accessible
	if (check_file_exist(compute_shader_path) == 0) {
		std::cout << "Warning: Cannot read the compute shader at location " << compute_shader_path << std::endl;
		std::cout << "If this file exists, you may need to adapt the directory from where your program is executed \n" << std::endl;
	}

	// Stop the program here if the file cannot be accessed
	assert_file_exist(compute_shader_path);

	// Read the files
	std::string compute_shader_text = read_text_file(compute_shader_path);


	// Compile the programs
	GLuint compute_shader_id   = 0; 
	bool const compute_shader_valid   = compile_shader(GL_COMPUTE_SHADER  , compute_shader_text  , compute_shader_id);
	if (compute_shader_valid == false) {
		std::cout << "===> Failed to compile the Compute Shader [" << compute_shader_path << "]" << std::endl;
		std::cout << "The error message from the compiler should be listed above. The program will stop." << std::endl;
		error_cgp("Failed to compile compute shader "+ compute_shader_path);
	}

	assert_cgp_no_msg(glIsShader(compute_shader_id));


	// Create Program
	GLuint const program_id = glCreateProgram();
	assert_cgp_no_msg(glIsProgram(program_id));

	// Attach Shader to Program
	glAttachShader(program_id, compute_shader_id);

	// Link Program
	glLinkProgram(program_id);

	// Shader can be detached.
	glDetachShader(program_id, compute_shader_id);

	// Debug info
	std::string msg = "  [info] Shader compiled succesfully [ID=" + str(program_id) + "]\n";
	msg            += "         (" + compute_shader_path + ")\n";
	std::cout << msg << std::endl;

	return program_id;
}
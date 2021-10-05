   
#include "Mode.hpp"

#include "GL.hpp"
#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <string>
#include <map>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void load_dialog_tree(std::string path);
	void transition(int new_state);
	void render_text(hb_buffer_t* buffer, float width, float x, float y, float scale, glm::vec3 color);
	void draw_image(GLuint &tex, float left, float right, float top, float bottom);

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} down, up, enter;

	// Dialog tree types
	typedef struct Response {
		int index;
		std::string line;
	} Response;

	typedef struct Dialog {
		std::string line;
		std::vector< Response > responses;
		int mood;
	} Dialog;

	std::vector< Dialog > dialog_tree;
	int dialog_state;
	int response_selection;

	float textTimer;
	float drawSpeed = 30.f;
	bool drawing_done = false;

	//music coming from the tip of the leg (as a demonstration):
	//std::shared_ptr< Sound::PlayingSample > leg_tip_loop;

	//images?
	GLuint white_tex;
	GLuint bg_tex;
	std::string bg_path = "./images/bg.png";
	GLuint images[9];
	std::string image_paths[9] = { "./images/George_happy.png", "./images/George_neutral.png",
									"./images/John_anger.png", "./images/John_neutral.png",
									"./images/Paul_closedeyes.png", "./images/Paul_happy.png", "./images/Paul_neutral.png",
									"./images/Ringo_happy.png", "./images/Ringo_neutral.png" };
	glm::u8vec4 white = glm::u8vec4(255, 255, 255, 255);

	//font stuff
	FT_Library ftlibrary;
	FT_Face ftface;
	hb_buffer_t* buf;
	std::vector<hb_buffer_t*> response_bufs;
	hb_font_t* hb_font;
	std::string font_path = "hockey.ttf";
	float font_size = 32.0f;

	//https://learnopengl.com/In-Practice/Text-Rendering
	struct Character {
		unsigned int TextureID;  // ID handle of the glyph texture
		glm::ivec2   Size;       // Size of glyph
		glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
		unsigned int Advance;    // Offset to advance to next glyph
	};
	//use FT_ULong instead of char, to represent glyph id instead
	std::string maintext;
	std::vector<std::string> choice_text;
	std::map<FT_ULong, Character> Characters;

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const& Position_, glm::u8vec4 const& Color_, glm::vec2 const& TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4 * 3 + 1 * 4 + 4 * 2, "PlayMode::Vertex should be packed");

	glm::mat4 projection = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f);

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;
	GLuint VAO = 0; //text

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;
	GLuint VBO = 0; //text
};

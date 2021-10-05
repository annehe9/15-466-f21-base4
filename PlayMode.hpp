   
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

	virtual void render_text(hb_buffer_t* buffer, float width, float x, float y, float scale, int length, glm::vec3 color);

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
	GLuint background_tex;
	GLuint john_tex;
	GLuint paul_tex;
	GLuint george_tex;
	GLuint ringo_tex;
	GLuint white_tex;

	//font stuff
	FT_Library ftlibrary;
	FT_Face ftface;
	hb_buffer_t* buf;
	std::vector<hb_buffer_t*> response_bufs;
	hb_font_t* hb_font;
	std::string font_path = "hockey.ttf";

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

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;
};

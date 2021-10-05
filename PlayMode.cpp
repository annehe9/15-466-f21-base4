#include "PlayMode.hpp"

//#include "LitColorTextureProgram.hpp"

#include "ColorTextureProgram.hpp"
#include "TextTextureProgram.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>
#include <string>

void PlayMode::load_dialog_tree(std::string path) {
	std::ifstream file(path);

	dialog_tree.clear();

    std::string line;
    while (std::getline(file, line))
    {
		Dialog dialog;

		sscanf_s(line.c_str(), "%d", &dialog.mood);

		std::getline(file, line);
		dialog.line = line;

		std::getline(file, line);
		while((int)line.length() > 0) {
			Response response;
			
			// Find the first space
			size_t split_pos = line.find_first_of(" ");

			// Scan the number from the string before the space
			sscanf_s(line.substr(0, split_pos).c_str(), "%d", &response.index);

			// Scan the response label from the rest of the string
			response.line = line.substr(split_pos, line.size() - split_pos);

			dialog.responses.emplace_back(response);

			if(!std::getline(file, line)) break;
		}

		dialog_tree.emplace_back(dialog);
    }

	// Print out the entire dialog tree for testing purposes
	//for(Dialog &dialog : dialog_tree) {
	//	printf("\n%s (%d)\n", dialog.line.c_str(), dialog.mood);
	//	for(Response &response : dialog.responses) {
	//		printf("  ( %d ) %s\n", response.index, response.line.c_str());
	//	}
	//}
}//

PlayMode::PlayMode() {
	//load dialog tree
	load_dialog_tree(data_path("dialog-tree.txt"));

	dialog_state = 0;
	response_selection = 0;

	//init text rendering
	// referenced https://learnopengl.com/In-Practice/Text-Rendering and https://www.freetype.org/freetype2/docs/tutorial/step1.html
	if (FT_Init_FreeType(&ftlibrary)) throw std::runtime_error("ERROR::FREETYPE: Could not init FreeType Library");
	if (FT_New_Face(ftlibrary, const_cast<char*>(data_path(font_path).c_str()), 0, &ftface)) throw std::runtime_error("ERROR::FREETYPE: Failed to load font");

	//FT_Set_Pixel_Sizes(ftface, 0, 18); //another way to set size
	FT_Set_Char_Size(ftface, 0, 32*64, 0, 0); //64 units per pixel
	if (FT_Load_Char(ftface, 'X', FT_LOAD_RENDER)) throw std::runtime_error("ERROR::FREETYTPE: Failed to load Glyph");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

	buf = hb_buffer_create();
	hb_buffer_add_utf8(buf, "Hello World!!!", -1, 0, -1);
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
	hb_buffer_set_language(buf, hb_language_from_string("en", -1));
	hb_font = hb_ft_font_create(ftface, NULL);
	hb_shape(hb_font, buf, NULL, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);
	glGenBuffers(1, &vertex_buffer);
	glBindVertexArray(vertex_buffer_for_color_texture_program);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//start music loop playing:
	// (note: position will be over-ridden in update())
	//leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);

	// Initialize to the start state
	transition(0);
}

PlayMode::~PlayMode() {
	hb_buffer_destroy(buf);
	FT_Done_Face(ftface);
	FT_Done_FreeType(ftlibrary);
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}else if (evt.key.keysym.sym == SDLK_RETURN) {
			enter.downs += 1;
			enter.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RETURN) {
			enter.pressed = false;
			return true;
		}
	}
	return false;
}

//RenderText base function from https://learnopengl.com/In-Practice/Text-Rendering
//integrated harfbuzz using this tutorial: https://harfbuzz.github.io/ch03s03.html
void PlayMode::render_text(hb_buffer_t* buffer, float width, float x, float y, float scale, int length, glm::vec3 color)
{
	//glDisable(GL_DEPTH_TEST);
	glUseProgram(text_texture_program->program);
	glUniform3f(glGetUniformLocation(text_texture_program->program, "textColor"), color.x, color.y, color.z);
	glm::mat4 projection = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f);
	glUniformMatrix4fv(glGetUniformLocation(text_texture_program->program, "projection"), 1, GL_FALSE, &projection[0][0]);
	glActiveTexture(GL_TEXTURE0);

	glBindVertexArray(vertex_buffer_for_color_texture_program);

	unsigned int glyph_count;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buffer, &glyph_count);
	//if (length != -1 && glyph_count > (unsigned int)(length)) glyph_count = length;

	for (unsigned int i = 0; i < glyph_count; ++i) {

		//check if the glyph has been loaded yet. if not, load it with this code from https://learnopengl.com/In-Practice/Text-Rendering
		if (Characters.find(glyph_info[i].codepoint) == Characters.end()) {
			if (FT_Load_Glyph(ftface, glyph_info[i].codepoint, FT_LOAD_RENDER)) std::runtime_error("ERROR::FREETYTPE: Failed to load Glyph");
			// generate texture
			//https://www.khronos.org/opengl/wiki/Texture
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				ftface->glyph->bitmap.width,
				ftface->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				ftface->glyph->bitmap.buffer
			);
			// set texture options
			//https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //not using mipmap
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //not using mipmap
			// now store character for later use
			Character ch = {
				texture,
				glm::ivec2(ftface->glyph->bitmap.width, ftface->glyph->bitmap.rows),
				glm::ivec2(ftface->glyph->bitmap_left, ftface->glyph->bitmap_top),
				(unsigned int)(ftface->glyph->advance.x)
			};
			Characters.insert(std::pair<FT_ULong, Character>(glyph_info[i].codepoint, ch));
		}

		Character ch = Characters[glyph_info[i].codepoint];
		float xpos = x + ch.Bearing.x * scale;
		float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
		float w = ch.Size.x * scale;
		float h = ch.Size.y * scale;

		// update vertex_buffer for each character
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};

		// render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		// update content of vertex_buffer memory
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)

	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);

}

void PlayMode::transition(int new_state) {
	dialog_state = new_state;

	response_selection = 0;
	
	hb_buffer_reset(buf); 
	hb_buffer_add_utf8(buf, dialog_tree[dialog_state].line.c_str(), -1, 0, -1);
	hb_shape(hb_font, buf, NULL, 0);

	for(hb_buffer_t* resp_buf : response_bufs) hb_buffer_destroy(resp_buf);
	response_bufs.clear();

	for(Response response : dialog_tree[dialog_state].responses) {
		hb_buffer_t* resp_buf = hb_buffer_create();

		hb_buffer_add_utf8(resp_buf, response.line.c_str(), -1, 0, -1);
		hb_buffer_set_direction(resp_buf, HB_DIRECTION_LTR);
		hb_buffer_set_script(resp_buf, HB_SCRIPT_LATIN);
		hb_buffer_set_language(resp_buf, hb_language_from_string("en", -1));
		hb_shape(hb_font, resp_buf, NULL, 0);

		response_bufs.emplace_back(resp_buf);
	}
}

void PlayMode::update(float elapsed) {
	Dialog current_dialog = dialog_tree[dialog_state];
	int response_cnt = (int)current_dialog.responses.size();

	if(up.pressed) {
		response_selection--;
		if(response_selection < 0) response_selection = 0;
	}
	else if(down.pressed) {
		response_selection++;
		if(response_selection >= response_cnt) response_selection = response_cnt - 1;
	}
	else if (enter.pressed) {
		// Transition to the next dialog state
		transition(current_dialog.responses[response_selection].index);
	}

	up.pressed = false;
	down.pressed = false;
	enter.pressed = false;

	/*
	if (!finished_typing) {
		text_timer += elapsed;
		curr_text_len = (unsigned int)std::floor(text_timer / typing_speed);
		if (text_len >= total_text_len) {
			finished_typing = true;
			curr_text_len = total_text_len;
		}
	}
	*/
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	Dialog current_dialog = dialog_tree[dialog_state];
	int response_cnt = (int)current_dialog.responses.size();

	//clear the color buffer:
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//scene.draw(*camera);

	//instead of 14 pass in curr_text_len
	//text buffer, width of box, x, y, scale, text length, color
	//buf = hb_buffer_create();
	//hb_buffer_add_utf8(buf, dialog_tree[dialog_state].line, -1, 0, -1);


	float y = 200.0f;
	render_text(buf, 950.0f, 100.f, y, 1.0f, 14, glm::vec3(1.0f,1.0f,1.0f));
	y -= 32.f;

	for(int i = 0; i < response_cnt; i++) {
		y -= 32.f;
		glm::vec3 color(i == response_selection ? 1.0f : 0.75f);

		render_text(response_bufs[i], 950.0f, 150.f, y, 1.0f, 14, color);
	}

	GL_ERRORS();
}

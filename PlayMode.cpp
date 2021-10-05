#include "PlayMode.hpp"

#ifdef _MSC_VER 
#define SSCANF sscanf_s
#else    
#define SSCANF sscanf
#endif

//#include "LitColorTextureProgram.hpp"

#include "ColorTextureProgram.hpp"
#include "TextTextureProgram.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>
#include <string>

// Mood indices
// 0 George_happy
// 1 George_neutral
// 2 John_anger
// 3 John_neutral
// 4 Paul_closedeyes
// 5 Paul_happy
// 6 Paul_neutral
// 7 Ringo_happy
// 8 Ringo_neutral
// 9 all


void PlayMode::load_dialog_tree(std::string path) {
	std::ifstream file(path);

	dialog_tree.clear();

    std::string line;
    while (std::getline(file, line))
    {
		Dialog dialog;

		int id;

		SSCANF(line.c_str(), "%d %d", &id, &dialog.mood);

		std::getline(file, line);
		dialog.line = line;

		std::getline(file, line);
		while((int)line.length() > 0) {
			Response response;
			
			// Find the first space
			size_t split_pos = line.find_first_of(" ");

			// Scan the number from the string before the space
			SSCANF(line.substr(0, split_pos).c_str(), "%d", &response.index);

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

	//taken from game0
	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of Vertex:
		glVertexAttribPointer(
			color_texture_program->Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte*)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program->Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program->Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte*)0 + 4 * 3 //offset
		);
		glEnableVertexAttribArray(color_texture_program->Color_vec4);

		glVertexAttribPointer(
			color_texture_program->TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte*)0 + 4 * 3 + 4 * 1 //offset
		);
		glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1, 1);
		std::vector< glm::u8vec4 > data(size.x * size.y, glm::u8vec4(0x00, 0x00, 0x00, 0xff)); //whoops actually black tex
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	//load images
	std::vector< glm::u8vec4 > data;
	glm::uvec2 size(0, 0);

	//bg
	load_png(data_path(bg_path), &size, &data, UpperLeftOrigin);

	glGenTextures(1, &bg_tex);
	glBindTexture(GL_TEXTURE_2D, bg_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	data.clear();

	//character sprites
	for (uint8_t i = 0; i < 9; ++i) {
		load_png(data_path(image_paths[i]), &size, &data, UpperLeftOrigin);

		glGenTextures(1, &images[i]);
		glBindTexture(GL_TEXTURE_2D, images[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		data.clear();
	}

	//load dialog tree
	load_dialog_tree(data_path("dialog-tree.txt"));

	dialog_state = 0;
	response_selection = 0;

	//init text rendering
	// referenced https://learnopengl.com/In-Practice/Text-Rendering and https://www.freetype.org/freetype2/docs/tutorial/step1.html
	if (FT_Init_FreeType(&ftlibrary)) throw std::runtime_error("ERROR::FREETYPE: Could not init FreeType Library");
	if (FT_New_Face(ftlibrary, const_cast<char*>(data_path(font_path).c_str()), 0, &ftface)) throw std::runtime_error("ERROR::FREETYPE: Failed to load font");

	//FT_Set_Pixel_Sizes(ftface, 0, 18); //another way to set size
	FT_Set_Char_Size(ftface, 0, (unsigned int)font_size*64, 0, 0); //64 units per pixel
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

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
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
void PlayMode::render_text(hb_buffer_t* buffer, float width, float x, float y, float scale, glm::vec3 color)
{
	//glDisable(GL_DEPTH_TEST);
	glUseProgram(text_texture_program->program);
	glUniform3f(glGetUniformLocation(text_texture_program->program, "textColor"), color.x, color.y, color.z);
	glUniformMatrix4fv(glGetUniformLocation(text_texture_program->program, "projection"), 1, GL_FALSE, &projection[0][0]);
	glActiveTexture(GL_TEXTURE0);

	glBindVertexArray(VAO);

	unsigned int glyph_count;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buffer, &glyph_count);
	float currx = x;
	float curry = y;

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
		float xpos = currx + ch.Bearing.x * scale;
		float ypos = curry - (ch.Size.y - ch.Bearing.y) * scale;
		float w = ch.Size.x * scale;
		float h = ch.Size.y * scale;

		// update VBO for each character
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
		// update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		currx += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
		if (currx > width + x && glyph_info[i].codepoint == 3) {
			currx = x;
			curry = y - scale * font_size;
		}
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);

}

void PlayMode::transition(int new_state) {
	dialog_state = new_state;

	response_selection = 0;

	textTimer = 0.f;
	drawing_done = false;
	
	hb_buffer_reset(buf); 
	hb_buffer_add_utf8(buf, dialog_tree[dialog_state].line.c_str(), -1, 0, -1);
	hb_shape(hb_font, buf, NULL, 0);

	// The current mood is listed here select the appropriate image to match this mood
	//printf("Mood %d\n", dialog_tree[dialog_state].mood);

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
		if (drawing_done) {
			// Transition to the next dialog state
			transition(current_dialog.responses[response_selection].index);
		}
		else {
			hb_buffer_reset(buf);
			hb_buffer_add_utf8(buf, const_cast<char*>(dialog_tree[dialog_state].line.c_str()), -1, 0, -1);
			hb_shape(hb_font, buf, NULL, 0);
			drawing_done = true;
		}
	}

	up.pressed = false;
	down.pressed = false;
	enter.pressed = false;

	// Slowly draw in the text
	if(!drawing_done) {
		textTimer += elapsed * drawSpeed;
		int substring = (int)textTimer;

		hb_buffer_reset(buf); 
		hb_buffer_add_utf8(buf, dialog_tree[dialog_state].line.substr(0, substring).c_str(), -1, 0, -1);
		hb_shape(hb_font, buf, NULL, 0);

		drawing_done = substring >= (int)current_dialog.line.size();
	}
}

void PlayMode::draw_image(GLuint &tex, float left, float right, float top, float bottom) {
	std::vector< Vertex > vertices;
	vertices.emplace_back(Vertex(glm::vec3(left, top, 0.0f), white, glm::vec2(0.0f, 1.0f)));
	vertices.emplace_back(Vertex(glm::vec3(right, top, 0.0f), white, glm::vec2(1.0f, 1.0f)));
	vertices.emplace_back(Vertex(glm::vec3(left, bottom, 0.0f), white, glm::vec2(0.0f, 0.0f)));

	vertices.emplace_back(Vertex(glm::vec3(left, bottom, 0.0f), white, glm::vec2(0.0f, 0.0f)));
	vertices.emplace_back(Vertex(glm::vec3(right, top, 0.0f), white, glm::vec2(1.0f, 1.0f)));
	vertices.emplace_back(Vertex(glm::vec3(right, bottom, 0.0f), white, glm::vec2(1.0f, 0.0f)));

	glBindTexture(GL_TEXTURE_2D, tex);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	//glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	GL_ERRORS();
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

	projection = glm::ortho(0.0f, float(drawable_size.x), 0.0f, float(drawable_size.y));
	glUseProgram(color_texture_program->program);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(vertex_buffer_for_color_texture_program);
	glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(projection));

	draw_image(bg_tex, 0.0f, (float)drawable_size.x, 0.0f, (float)drawable_size.y);
	int currmood = dialog_tree[dialog_state].mood;
	float offsetx = (float)drawable_size.x / 12.0f;
	float offsety = (float)drawable_size.y / 3.0f;
	if (currmood == 9) { //draw all
		draw_image(images[3], offsetx,   offsetx + (float)drawable_size.x * (400.0f/1280.0f),     offsety, offsety+(float)drawable_size.y * (480.0f/720.0f)); //john
		draw_image(images[6], offsetx*3, offsetx*3 + (float)drawable_size.x * (400.0f / 1280.0f), offsety, offsety + (float)drawable_size.y * (480.0f / 720.0f)); //pete
		draw_image(images[1], offsetx*5, offsetx*5 + (float)drawable_size.x * (400.0f / 1280.0f), offsety, offsety + (float)drawable_size.y * (480.0f / 720.0f)); //geoff
		draw_image(images[8], offsetx*7, offsetx*7 + (float)drawable_size.x * (400.0f / 1280.0f), offsety, offsety + (float)drawable_size.y * (480.0f / 720.0f)); //richard
	}
	else { //draw one
		draw_image(images[currmood], offsetx * 4, offsetx * 4 + (float)drawable_size.x * (400.0f / 1280.0f), offsety, offsety + (float)drawable_size.y * (480.0f / 720.0f));
	}

	//text box
	draw_image(white_tex, offsetx, (float)drawable_size.x - offsetx, 0, offsety);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);

	// Draw the text
	font_size = (float)drawable_size.y * (32.0f / 720.0f);
	if (font_size != 32.0f) {
		FT_Set_Char_Size(ftface, 0, (unsigned int)font_size * 64, 0, 0); //64 units per pixel
		if (FT_Load_Char(ftface, 'X', FT_LOAD_RENDER)) throw std::runtime_error("ERROR::FREETYTPE: Failed to load Glyph");
		Characters.clear();
	}
	float y = (float)drawable_size.y * (200.0f / 720.0f);
	render_text(buf, (float)drawable_size.x * (800.0f/1280.0f), (float)drawable_size.x * (200.0f/1280.0f), y, 1.0f, glm::vec3(1.0f,1.0f,1.0f));
	y -= font_size;

	if(drawing_done) {
		for(int i = 0; i < response_cnt; i++) {
			y -= font_size;
			glm::vec3 color(i == response_selection ? 1.0f : 0.75f);

			render_text(response_bufs[i], (float)drawable_size.x * (750.0f / 1280.0f), (float)drawable_size.x * (250.0f / 1280.0f), y, 1.0f, color);
		}
	}

	GL_ERRORS();
}

#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>
#include <string>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

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

	for(Dialog &dialog : dialog_tree) {
		printf("\n%s (%d)\n", dialog.line.c_str(), dialog.mood);
		for(Response &response : dialog.responses) {
			printf("  ( %d ) %s\n", response.index, response.line.c_str());
		}
	}
}

PlayMode::PlayMode() : scene(*hexapod_scene) {
	load_dialog_tree(data_path("dialog-tree.txt"));

	dialog_state = 0;
	response_selection = 0;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
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
	else if(enter.pressed) {
		// Transition to the next dialog state
		dialog_state = current_dialog.responses[response_selection].index;
		response_selection = 0;
	}

	//reset button press counters:
	up.downs = 0;
	down.downs = 0;
	enter.downs = 0;

	up.pressed = false;
	down.pressed = false;
	enter.pressed = false;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		float y = -0.3f;
		lines.draw_text(dialog_tree[dialog_state].line,
			glm::vec3(-aspect + 0.1f * H + 0.3f, y + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		for(int i = 0; i < dialog_tree[dialog_state].responses.size(); i++) {
			y -= H * 1.2f;

			lines.draw_text(dialog_tree[dialog_state].responses[i].line,
				glm::vec3(-aspect + 0.1f * H + 0.5f, y + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				(i == response_selection)
				? glm::u8vec4(0xff, 0xff, 0xff, 0x00)
				: glm::u8vec4(0xc0, 0xc0, 0xc0, 0x00));
		}

		//float ofs = 2.0f / drawable_size.y;
		//lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
		//	glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
		//	glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
		//	glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

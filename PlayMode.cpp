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

PlayMode::PlayMode() {
	load_dialog_tree(data_path("dialog-tree.txt"));
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

}

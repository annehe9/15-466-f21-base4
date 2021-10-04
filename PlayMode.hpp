#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void load_dialog_tree(std::string path);

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

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
	
	//camera:
	Scene::Camera *camera = nullptr;

};
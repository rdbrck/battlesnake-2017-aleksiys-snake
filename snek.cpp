#include "crow/amalgamate/crow_all.h"
#include "json/src/json.hpp"
#include "game_defs.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <time.h>




using namespace std;
using namespace crow;
using JSON = nlohmann::json;


int findFallbackMove(GameInfo game) {
	profile prof(__FUNCTION__, __LINE__);
	cout << "FALL BACK MOVE" << endl;
	Point head = game.snake.getHead();

	vector<int> posmoves = vector<int>();
	for (auto m : moveslist) {
		Point p = head.addMove(m);

		//if we can move into tail
		if(p.compare(game.snake.getTail()) && game.snake.coords.size() > 3){
			return m;
		}

		if (game.board.isValid(p)) {
			posmoves.push_back(m);
		}
	}

	//dead end go into snake BUFFER
	if (!posmoves.size()) {
		cout << "BUFFER" << endl;
		for (auto m : moveslist) {	
			Point p = head.addMove(m);
			if (game.board.getCoord(p) == BUFFER) {
				posmoves.push_back(m);
			}
		}
	}

	//try to go into wall 
	if (!posmoves.size()) {
		cout << "WALL" << endl;
		for (auto m : moveslist) {	
			Point p = head.addMove(m);
			if (game.board.getCoord(p) == WALL) {
				posmoves.push_back(m);
			}
		}
	}

	//else we are fucked anyways yolo
	if (!posmoves.size()) {
		cout << "WE FUCKED" << endl;
		return 0;
	}

	return posmoves[rand() % posmoves.size()];
}

int eat(GameInfo game) {
	cout << "TEST" << endl;
	Point head = game.snake.getHead();
	Path path = game.breadthFirstSearch(head, {FOOD}, false);

	// found path
	if (path.path.size() > 1){
		return path.getStepDir(0);
	}

	return findFallbackMove(game);
}

int orbit(GameInfo game){
	Point head = game.snake.getHead();
	Point tail = game.snake.getTail();
	Path path = game.astarGraphSearch(head, tail);
	if(path.path.size() > 1 && game.snake.coords.size() > 3){
		return path.getStepDir(0);
	}
	return findFallbackMove(game);
}

int checkMove(GameInfo game, int move){


	vector<float> vals = game.lookahead();

	int i = 0;
	for(auto val: vals){
		cout << " " << val << " ";
	}
	cout << endl;
	cout << "size " << vals.size() << endl;
	cout << "MOVE " << move << endl;

	/*
	if(vals[move] < 0){
		cout << "Exec Max Move" << endl;
		move = distance(vals.begin(), max_element(vals.begin(), vals.end()));
		cout << "New move " << move << endl;
		return move;
	}*/

	return move;
}


string moveResponse(int dir) {
	JSON move;
	switch (dir) {
	case NORTH:
		move["move"] = "up";
		move["taunt"] = "THE NORTH REMEMBERS";
		break;
	case EAST:
		move["move"] = "right";
		move["taunt"] = "TO THE EAST";
		break;
	case SOUTH:
		move["move"] = "down";
		move["taunt"] = "SOUTH WHERE ITS WARM";
		break;
	case WEST:
		move["move"] = "left";
		move["taunt"] = "WEST IS BEST";
		break;
	}
	return move.dump();
}

int executeState(GameInfo game, int state) {
	int move = 0;
	switch (state) {
	case EAT:
		move = eat(game);
		return checkMove(game, move);
		break;
	case FINDFOOD:
		return findFallbackMove(game);
		break;

	case ORBIT:
		move = orbit(game);
		return checkMove(game, move);
		break;

	case DEFAULT:
		move = findFallbackMove(game);
		return checkMove(game, move);
		break;
	}
	return findFallbackMove(game);
}

int decideState(GameInfo game) {
	if (game.snake.health < 95) {
		return EAT;
	}
	if (game.snake.health < 101) {
		return ORBIT;
	}
	return DEFAULT;
}



string SnakeInfo() {
	JSON info;
	info["color"] = "#00FF00";
	info["head_url"] = "http://pets.wilco.org/Portals/7/Containers/Pets2011/images/star.png";
	info["taunt"] = "Test Taunt";
	info["name"] = "name";
	return info.dump();
}

SimpleApp initSnakeApp() {
	SimpleApp app;

	//INFO
	CROW_ROUTE(app, "/")([]() {
		return SnakeInfo();
	});


	//START
	CROW_ROUTE(app, "/start")
	.methods("POST"_method)
	([](const crow::request & req) {
		return SnakeInfo();
	});

	//MOVE
	CROW_ROUTE(app, "/move")
	.methods("POST"_method)
	([](const crow::request & req) {
		clock_t t = clock();
		GameInfo game = GameInfo(req.body);
		int state = decideState(game);
		int move =  executeState(game, state);
		t = clock() - t;
		cout << "Exec Move Time: " << ((float) t ) / CLOCKS_PER_SEC << endl; 
		return moveResponse(move);
	});

	return app;
}

int main(int argc, char **argv)
{
	int port = 7000;
	if (argc == 2) {
		port = atoi(argv[1]);
	}
	SimpleApp app = initSnakeApp();
	app.port(port).multithreaded().run();
}
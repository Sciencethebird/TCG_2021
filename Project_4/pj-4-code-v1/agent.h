/**
 * Framework for NoGo and similar games (C++ 11)
 * agent.h: Define the behavior of variants of the player
 *
 * Author: Theory of Computer Games (TCG 2021)
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include <fstream>
#include <limits>
#include <math.h>       /* log */
#include "board.h"
#include "action.h"
#include "node.h"

#define EXPLOR_CONST 1.2

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args); // "name=unknown...." gets replace if there's new value
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('=')); 
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; } // this is nogo so the last player with no legal move loses

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

/**
 * base agent for agents with randomness
 */
class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * random player for both side
 * put a legal piece randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=random role=unknown " + args),
		space(board::size_x * board::size_y), who(board::empty) {
		if (name().find_first_of("[]():; ") != std::string::npos)
			throw std::invalid_argument("invalid name: " + name());
		if (role() == "black") who = board::black;
		if (role() == "white") who = board::white;
		if (who == board::empty)
			throw std::invalid_argument("invalid role: " + role());
		for (size_t i = 0; i < space.size(); i++) 
			space[i] = action::place(i, who); // all go board is available???
	}

	virtual action take_action(const board& state) {
		std::cout << "who: "<< who << std::endl;
		std::shuffle(space.begin(), space.end(), engine);
		for (const action::place& move : space) {
			board after = state;
			if (move.apply(after) == board::legal){
				return move;
			}
		}
		return action(); // empty action
	}

private:
	std::vector<action::place> space;
	board::piece_type who;
};


class mcts_player : public random_agent {
public:
	mcts_player(const std::string& args = "") : random_agent(args),
		space(board::size_x * board::size_y), opponent_space(board::size_x * board::size_y), me(board::empty), opponent(board::empty) {
		if (name().find_first_of("[]():; ") != std::string::npos)
			throw std::invalid_argument("invalid name: " + name());
		if (role() == "black") {
			me = board::black;
			opponent = board::white;
		}
		if (role() == "white") {
			me = board::white;
			opponent = board::black;
		}
		if (me == board::empty)
			throw std::invalid_argument("invalid role: " + role());

		// init me action space
		for (size_t i = 0; i < space.size(); i++)
			space[i] = action::place(i, me);
		// init opponent action space
		for (size_t i = 0; i < opponent_space.size(); i++)
			opponent_space[i] = action::place(i, opponent);
	}

	virtual action take_action(const board& state) {
		//simulate(state, me);[[]]
		//std::cout << "black's turn:\n" << state << std::endl;
		//std::cin.get();
		node::node_ptr root = new node(opponent, state, space.size());

		for(int i = 0; i< 10000; i++){
			run_search(root);
		}
		//std::cin.get();

		double best_value = -1 * std::numeric_limits<double>::infinity();;
		action::place best_action = action();
		//std::cout << "root's children winrate: ";
		for(auto child: root->children){
			//printf("[%f, %d/%d], ", child->get_winrate(), child->win_count, child->visit_count);
			if(child->get_winrate() > best_value){
				best_value = child->get_winrate();
				best_action =  child->applied_action;
			}
		}
		//std::cout << std::endl;

		delete_tree(root);
		delete root;

		return best_action;
	}

	void run_search(node::node_ptr root){
		//std::cout <<"\nnew search, state = " << root->win_count <<"/" << root->visit_count << std::endl;
		node::node_ptr best_leaf = select(root);
		node::node_ptr new_leaf = expand(best_leaf);
		int score = simulate(new_leaf);
		back_propagation(new_leaf, score);
	}

	node::node_ptr select(node::node_ptr curr){
		while(1){
			if(curr->is_fully_expanded() && !curr->is_terminal_node()){
				curr = get_best_child(curr, EXPLOR_CONST);
			} else{
				break; // find a 
			}
		}
		return curr;
	}
	node::node_ptr expand(node::node_ptr curr){
		// return original node of the node is a terminal node (game end node)
		if(curr->is_terminal_node()) return curr;
		else return curr->create_child();
		
	}
	// int simulate(board state, board::piece_type who){
	int simulate(node::node_ptr new_leaf){
		//std::cout << "\n\n\n\n\n\n\n\nbefore simulation\n" << new_leaf->state << std::endl;
		int	score;
		board state = new_leaf->state;
		board::piece_type who, next;
		who = new_leaf->opponent; // start simulation from opponet 
		while(1){
			std::vector<action::place> *curr_space;
			if(who == me){
				curr_space = &space;
				next = opponent;
			} else { // opponent
				curr_space = &opponent_space;
				next = me;
			} 

			int terminate = 1;

			// takes random action
			std::shuffle(curr_space->begin(), curr_space->end(), engine);
			for (const action::place& move : (*curr_space)) {
				if (move.apply(state) == board::legal){
					terminate = 0;
					break;
				}
			}
			// end of simulation
			if(terminate){ // someone loses game terminates
				score = (who!=me); // opponent can't find any legal move
				break;
			}
			who = next;
		}
		//std::cout << "simulation result: "<< score  << "\n" << state << std::endl;
		return score;
	}
	int simulate(board state, board::piece_type who){
		std::cout << "\n\n\n\n\n\n\n\nbefore simulation\n" << state << std::endl;
		int	score;

		while(1){
			std::vector<action::place> *curr_space;
			board::piece_type next;
			if(who == me){
				curr_space = &space;
				next = opponent;
			} else { // opponent
				curr_space = &opponent_space;
				next = me;
			} 
			int terminate = 1;

			// takes random action
			std::shuffle(curr_space->begin(), curr_space->end(), engine);
			for (const action::place& move : (*curr_space)) {
				if (move.apply(state) == board::legal){
					terminate = 0;
					break;
				}
			}
			// end of simulation
			if(terminate){ // someone loses game terminates
				score = (who!=me); // opponent can't find any legal move
				break;
			}
			who = next;
		}
		std::cout << "simulation result: "<< score  << "\n" << state << std::endl;
		return score;
	}
	void back_propagation(node::node_ptr curr, int score){
		while(curr != nullptr){
			//std::cout << "backprop, node state = " << curr->win_count <<"/" <<curr->visit_count << std::endl;
			curr->visit_count++;
			curr->win_count+=score;
			curr = curr->parent;
		}
	}
	void delete_tree(node::node_ptr root){
		if(root->children.size() == 0) return;
		while(root->children.size()>0){
			delete_tree(root->children.back());
			delete root->children.back();
			root->children.pop_back();
		}
	}
private:

	
	node::node_ptr get_best_child(node::node_ptr curr, double exploration_const){
		double best_value = -1 * std::numeric_limits<double>::infinity();
		std::vector<node::node_ptr> best_nodes;
		for(auto child: curr->children){
			// UCB
			double node_value = child->win_count / child->visit_count +
				exploration_const * sqrt(2*log(curr->visit_count) / child->visit_count);
			if(node_value > best_value){
				best_value = node_value;
				best_nodes.clear();
				best_nodes.push_back(child);
			} else if(node_value == best_value){
				best_nodes.push_back(child);
			}
		}
		std::shuffle(best_nodes.begin(), best_nodes.end(), engine);
		return best_nodes.front();
	}

	/* 
	// random agent
	virtual action take_action(const board& state) {
		std::shuffle(space.begin(), space.end(), engine);
		for (const action::place& move : space) {
			board after = state;
			if (move.apply(after) == board::legal)
				return move;
		}
		return action();
	}
	*/

private:
	std::vector<action::place> space;
	std::vector<action::place> opponent_space; // move space of white
	board::piece_type me;
	board::piece_type opponent;
};


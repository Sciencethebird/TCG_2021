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
#include <math.h>       
#include <chrono>
#include <thread>
#include "board.h"
#include "action.h"
#include "node.h"

#define EXPLOR_CONST 0.5
#define STEP_LIMIT 36

std::mutex shuffle_mutex;
class agent {
public:
	agent(const std::string& args = "") {
		
		std::stringstream ss("name=unknown role=unknown search=random worker=1 exploration=1.0 timeout=9999999 N=-1 total_time=-1" + args); // "name=unknown...." gets replace if there's new value
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
	virtual std::string search() const { return property("search"); }
	virtual int timeout() const { return std::stoi(property("timeout")); }
	virtual int worker() const { return std::stoi(property("worker")); }
	virtual double exploration() const { return std::stof(property("exploration")); }
	virtual double simulation_N() const { return std::stoi(property("N")); }
	virtual double total_time() const { return std::stoi(property("total_time")); }
	//search=MCTS timeout=1000

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
//class player : public random_agent {
//public:
//	player(const std::string& args = "") : random_agent("name=random role=unknown " + args),
//		space(board::size_x * board::size_y), who(board::empty) {
//		if (name().find_first_of("[]():; ") != std::string::npos)
//			throw std::invalid_argument("invalid name: " + name());
//		if (role() == "black") who = board::black;
//		if (role() == "white") who = board::white;
//		if (who == board::empty)
//			throw std::invalid_argument("invalid role: " + role());
//		for (size_t i = 0; i < space.size(); i++) 
//			space[i] = action::place(i, who); // all go board is available???
//	}
//
//	virtual action take_action(const board& state) {
//		std::cout << "who: "<< who << std::endl;
//		std::shuffle(space.begin(), space.end(), engine);
//		for (const action::place& move : space) {
//			board after = state;
//			if (move.apply(after) == board::legal){
//				return move;
//			}
//		}
//		return action(); // empty action
//	}
//
//private:
//	std::vector<action::place> space;
//	board::piece_type who;
//};


class player : public random_agent {
public:
	enum search_methods { RANDAM = 0, MCTS = 1 };
public:
	player(const std::string& args = ""): random_agent(args),
		space(board::size_x * board::size_y), opponent_space(board::size_x * board::size_y), space_index(board::size_x * board::size_y), me(board::empty), opponent(board::empty) {
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
		for (size_t i = 0; i < space_index.size(); i++)
			space_index[i] = i;

		// assign node random engine
		node::engine = engine;

		// timeout of MCTS
		if(search() == "MCTS"){
			time_limit = timeout();
			search_method = player::MCTS;
			worker_num = worker();
			simulation_limit = simulation_N();
			exploration_const = exploration();
			curr_step_idx = 0;
			int total = total_time();
			int ttt = 0;
			if(total != -1){
				float slope = 1.95 * total / pow(STEP_LIMIT, 2.0);
				//std::cout << exploration_const << std::endl;
				std::cout << total << ", " << slope << std::endl;
				for(int i = 0; i<STEP_LIMIT; i++){
					time_schedule[i] = slope * (STEP_LIMIT - i);
					ttt += time_schedule[i];
					//std::cout << "time: " << time_schedule[i] << std::endl;
				}
			}else{
				for(int i = 0; i<STEP_LIMIT; i++){
					time_schedule[i] = time_limit;
					ttt += time_schedule[i];
					//std::cout << "time: " << time_schedule[i] << std::endl;
				}
			}
			
			//std::cout << ttt << std::endl;
			//std::cin.get();
		} 
	}
	virtual action take_action(const board& state) {
		if(search_method == player::MCTS){
			if(role() == "white") {
   int idx = 0;
   while(idx<=80) {
    board after = state;
    if(idx==30||idx==31||idx==32||idx==39||idx==40||idx==41||idx==48||idx==49||idx==50||std::count(temp_v.begin(),temp_v.end(), idx)){
     idx++;
     continue;
    }
    const action::place& move = space[idx];
    if (move.apply(after) == board::illegal_not_empty){
     temp_v.push_back(idx);
     // std::cout<<"idx = "<<idx<<std::endl;
     // std::cout<<move<<" is illegal_not_empty"<<std::endl;
     temp_v.push_back(80-idx);
     return space[80-idx];
    }
    idx++;
   }
  }
			return take_action_mcts(state);
		} else {
			return take_action_random(state);
		}
	}
	action take_action_random(const board& state) {
		std::shuffle(space.begin(), space.end(), engine);
		for (const action::place& move : space) {
			board after = state;
			if (move.apply(after) == board::legal){
				return move;
			}
		}
		return action(); // empty action
	}
	action take_action_mcts(const board& state) {

		std::vector<node*> roots;
		std::vector<std::thread> workers;

		// MCTS root parallelization
		//std::cin.get();
		//std::cout << worker_num << std::endl;
		for(int i = 0; i< worker_num; i++){
			node::node_ptr root = new node(opponent, state, space.size());
			std::thread worker(&player::run_mcts, this, root);
			roots.push_back(root);
			workers.push_back(std::move(worker));
		}

		for(int i = 0; i< worker_num; i++){
			workers[i].join();
		}
		
		std::map<action::place, int> majority_voting;
		std::map<action::place, int> majority_visit_count;
		//std::map<action::place, int> majority_win_count;
		//int idx = 0;
		for(auto root: roots){
			//double best_winrate = -1 * std::numeric_limits<double>::infinity();
			//unsigned long long best_value = 0;
			//action::place best_action = action();
			// search root's child for best-value action
			//printf("\nroot %d:\n", idx++);

			for(auto child: root->children){
				//printf("[%f, %d/%d], ", child->get_winrate(), child->win_count, child->visit_count);
				//if(child->get_winrate() > best_value){
				//	best_value = child->get_winrate();
				//	best_action =  child->applied_action;
				//}
				//printf("winrate, visit count: %f, %lld, %d\n",child->get_winrate(), child->visit_count, root->children.size());
				//if(child->visit_count >= best_value){
				//	if(child->get_winrate() >= best_winrate){
				//	
				//		best_winrate = child->get_winrate();
				//		best_value = child->visit_count;
				//		best_action = child->applied_action;
				//	}
				//}
				//printf("[%f, %d/%d], ", child->get_winrate(), child->win_count, child->visit_count);
				majority_visit_count[child->applied_action]+=child->visit_count;
				//majority_win_count[child->applied_action]+=child->win_count;
			}
			//majority_voting[best_action]++;
			
		}
		action::place best_action = action();
		//int highest_vote = 0;
		//for(auto act:majority_voting){
		//	std::cout << "action: " << act.first << " , ac: " << act.second << std::endl;
		//	if(act.second>highest_vote){
		//		highest_vote = act.second;
		//		best_action = act.first;
		//	}
		//}
		int highest_count = 0;
		for(auto act:majority_visit_count){
			//std::cout << "action: " << act.first << " , ac: " << act.second << std::endl;
			if(act.second>highest_count){
				highest_count = act.second;
				best_action = act.first;
				//std::cout << "ldfldlflsf\n\n" << std::endl;
			}
		}

		//int highest_count = 0;
		//for(auto act:majority_win_count){
		//	//std::cout << "action: " << act.first << " , ac: " << act.second << std::endl;
		//	if(act.second>highest_count){
		//		highest_count = act.second;
		//		best_action = act.first;
		//	}
		//}
		//if(highest_count == 0){
		//	for(auto act:majority_visit_count){
		//		//std::cout << "action: " << act.first << " , ac: " << act.second << std::endl;
		//		if(act.second>highest_count){
		//			highest_count = act.second;
		//			best_action = act.first;
		//			//std::cout << "ldfldlflsf\n\n" << std::endl;
		//		}
		//	}
		//}
		std::cout << "step: " << curr_step_idx << std::endl;
		std::cout << "best action" << best_action << "best count: " << highest_count << std::endl;
		//std::cout << "==============\n\n" << std::endl;

		// free memory of mcts trees
		auto start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();  
		for(auto root: roots){
			delete_tree(root);
		}
		auto curr_time = std::chrono::high_resolution_clock::now().time_since_epoch().count(); 
		auto duration = (curr_time - start_time) / 1000000; // msec
		printf("memory release time: %ld\n", duration);
		curr_step_idx++;
		return best_action;
	}
	void run_mcts(node* root){
		auto start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();   
		int sim_count = 0;
		//std::cout << simulation_limit << std::endl;
		//std::cin.get();
		std::vector<action::place> local_space = space;
		std::vector<action::place> local_opponent_space = opponent_space;

		while(1){
			mcts(root, &local_space, &local_opponent_space);
			sim_count++;
			//std::cout <<simulation_count << std::endl;
			auto curr_time = std::chrono::high_resolution_clock::now().time_since_epoch().count(); 
			auto duration = (curr_time - start_time) / 1000000; // msec
			
			if(duration > time_schedule[curr_step_idx]) break;
			if((simulation_limit) > 0 && (sim_count > simulation_limit)) break;
		}
		auto curr_time = std::chrono::high_resolution_clock::now().time_since_epoch().count(); 
		auto duration = (curr_time - start_time) / 1000000; // msec
		printf("MCTS: %d simulation done, duration: %ld\n", sim_count, duration);
	}
	void mcts(node* root, std::vector<action::place>* local_space , std::vector<action::place>* local_opponent_space ){
		//std::cout <<"\nnew search, state = " << root->win_count <<"/" << root->visit_count << std::endl;
		node* best_leaf = select(root);
		node*  new_leaf = expand(best_leaf);
		int score = simulate(new_leaf, local_space, local_opponent_space);
		back_propagation(new_leaf, score);
	}

	node::node_ptr select(node::node_ptr curr){
		while(1){
			if(curr->is_fully_expanded() && !curr->is_terminal_node()){
				curr = get_best_child(curr, curr->me);
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
	int simulate(node::node_ptr new_leaf, std::vector<action::place>* local_space , std::vector<action::place>* local_opponent_space ){
		//std::cout << "\n\n\n\n\n\n\n\nbefore simulation\n" << new_leaf->state << std::endl;
		int	score;
		board state = new_leaf->state;
		board::piece_type who, next;
		who = new_leaf->opponent; // start simulation from opponet 

		// create local space to prevent shuffle racing problem
		//std::shuffle(space.begin(), space.end(), engine);
		//std::shuffle(opponent_space.begin(), opponent_space.end(), engine);

		
		//std::vector<int> local_space_index = space_index;
		std::vector<action::place>* curr_space;
		std::shuffle(local_opponent_space->begin(), local_opponent_space->end(), engine);
		std::shuffle(local_space->begin(), local_space->end(), engine);

		std::vector<action::place> llocal_space = *local_space;
		std::vector<action::place> llocal_opponent_space = *local_opponent_space;
		//int steps = 0;
		int space_idx = 0;
		int opponent_space_idx = 0;
		int* curr_space_idx;
		while(1){
			
			if(who == me){
				curr_space = &llocal_space;
				next = opponent;
				curr_space_idx = &space_idx;
			} else { // opponent
				curr_space = &llocal_opponent_space;
				next = me;
				curr_space_idx = &opponent_space_idx;
			} 

			int terminate = 1;

			// takes random action
			//std::shuffle(curr_space->begin(), curr_space->end(), engine);

			for (size_t i = (*curr_space_idx); i< curr_space->size(); i++) {
				if ((*curr_space)[i].apply(state) == board::legal){
					terminate = 0;
					//curr_space->erase(curr_space->begin()+i);
					(*curr_space_idx)=i+1;
					break;
				}
			}

			//for (size_t i = 0; i< curr_space->size(); i++) {
			//	if ((*curr_space)[i].apply(state) == board::legal){
			//		terminate = 0;
			//		curr_space->erase(curr_space->begin()+i);
			//		break;
			//	}
			//}

			//for (const action::place& move : (*curr_space)) {
			//	if (move.apply(state) == board::legal){
			//		terminate = 0;
			//		break;
			//	}
			//}

			// end of simulation
			if(terminate){ // someone loses game terminates
				score = (who!=me); // opponent can't find any legal move
				break;
			}
			who = next;
		}
		
		// shuffle original space again to prevent same shuffle order
		//std::lock_guard<std::mutex> guard(shuffle_mutex); 
		//std::shuffle(space.begin(), space.end(), engine);
		//std::shuffle(opponent_space.begin(), opponent_space.end(), engine);
		//std::cout << "simulation result: "<< score  << "\n" << state << std::endl;
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

	void reset(){
		curr_step_idx = 0;
		std::cout << "reset agent" << std::endl;
	}
	
private:
	void delete_tree(node::node_ptr root){
		delete_tree_branch(root);
		delete root;
	}
	void delete_tree_branch(node::node_ptr root){
		if(root->children.size() == 0) return;
		while(root->children.size()>0){
			delete_tree_branch(root->children.back());
			delete root->children.back();
			root->children.pop_back();
		}
	}
	node::node_ptr get_best_child(node::node_ptr curr, board::piece_type who){
		double best_value = -1 * std::numeric_limits<double>::infinity();
		std::vector<node::node_ptr> best_nodes;
		for(auto child: curr->children){
			// UCB
			double node_value;
			if(who != me){
				//std::cout << "meeeeeeeee" << std::endl;
				node_value = (double) child->win_count / (double) child->visit_count +
				exploration_const * sqrt(2*log(curr->visit_count) / child->visit_count);
			} else {
				//std::cout << "opponent" << std::endl;
				node_value = (1.0 - (double) child->win_count / (double) child->visit_count ) +
				exploration_const * sqrt(2*log(curr->visit_count) / child->visit_count);
			}
			
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
public:
	std::vector<int> temp_v;
private:
	std::vector<action::place> space;
	std::vector<action::place> opponent_space; // move space of white
	std::vector<int> space_index;
	board::piece_type me;
	board::piece_type opponent;
	float exploration_const;
	int time_limit;
	int search_method;
	int worker_num;
	int simulation_limit;
	int time_schedule[36];
	int curr_step_idx;
};


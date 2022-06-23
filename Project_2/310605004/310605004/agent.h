/**
 * Framework for 2048 & 2048-like Games (C++ 11)
 * agent.h: Define the behavior of variants of agents including players and environments
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
#include "board.h"
#include "action.h"
#include "weight.h"
#include <fstream>

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
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
	virtual bool check_for_win(const board& b) { return false; }

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
 * base agent for agents with weight tables and a learning rate
 */
class td_player : public agent {
public:
	td_player(const std::string& args = "") : agent(args), alpha(0) {
		if (meta.find("init") != meta.end())
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end())
			load_weights(meta["load"]);
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~td_player() {
		if (meta.find("save") != meta.end())
			save_weights(meta["save"]);
	}
	virtual action take_action(const board& before) {
		int best_op = -1;
		int best_reward = -1;
		float best_value = -100000;
		board best_after;
		for(int op : {0, 1, 2, 3}) {
			board after = before;
			int reward = after.slide(op);
			if(reward == -1) continue;
			float value = estimate_value(after);
			if(reward + value > best_reward + best_value){
				best_reward = reward;
				best_value = value;
				best_op = op;
				best_after = after;
			}
		}
		if(best_op != -1){
			replay_buffer.push_back({best_reward, best_after});
		}
		return action::slide(best_op);
	}

	struct step
	{
		int reward;
		board after;
	};
	
	int extract_feature(const board& after, int a, int b, int c, int d) const {
		return after(a) * 25 * 25 * 25 + after(b) * 25 * 25 + after(c) * 25 + after(d);
	}
	float estimate_value(const board& after) const {
		float value = 0;
		value += net[0][extract_feature(after, 0, 1, 2, 3)];
		value += net[1][extract_feature(after, 4, 5, 6, 7)];
		value += net[2][extract_feature(after, 8, 9, 10, 11)];
		value += net[3][extract_feature(after, 12, 13, 14, 15)];
		value += net[4][extract_feature(after, 0, 4, 8, 12)];
		value += net[5][extract_feature(after, 1, 5, 9, 13)];
		value += net[6][extract_feature(after, 2, 6, 10, 14)];
		value += net[7][extract_feature(after, 3, 7, 11, 15)];	
		return value;
	}
	void adjust_value(const board& after, float target) {
		float current = estimate_value(after);
		float error = target - current;
		float adjust = alpha * error;
		net[0][extract_feature(after, 0, 1, 2, 3)] += adjust;
		net[1][extract_feature(after, 4, 5, 6, 7)] += adjust;
		net[2][extract_feature(after, 8, 9, 10, 11)] += adjust;
		net[3][extract_feature(after, 12, 13, 14, 15)] += adjust;
		net[4][extract_feature(after, 0, 4, 8, 12)] += adjust;
		net[5][extract_feature(after, 1, 5, 9, 13)] += adjust;
		net[6][extract_feature(after, 2, 6, 10, 14)] += adjust;
		net[7][extract_feature(after, 3, 7, 11, 15)] += adjust;
	}
	virtual void open_episode(const std::string & flag = ""){
		replay_buffer.clear();
	}
	virtual void close_episode(const std::string& flag = ""){
		if(replay_buffer.empty()) return;
		if (alpha == 0) return;
		adjust_value(replay_buffer[replay_buffer.size() - 1].after, 0);
		for(int t = replay_buffer.size() -2; t >= 0; t--) {
			adjust_value(replay_buffer[t].after, replay_buffer[t+1].reward + estimate_value(replay_buffer[t+1].after));
		}
	}
protected:
	virtual void init_weights(const std::string& info) {
		net.emplace_back(25 * 25 * 25 * 25); // tuple 0: first line 
		net.emplace_back(25 * 25 * 25 * 25); 
		net.emplace_back(25 * 25 * 25 * 25); 
		net.emplace_back(25 * 25 * 25 * 25); 
		net.emplace_back(25 * 25 * 25 * 25); 
		net.emplace_back(25 * 25 * 25 * 25); 
		net.emplace_back(25 * 25 * 25 * 25); 
		net.emplace_back(25 * 25 * 25 * 25); 
	}
	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
	}
	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
	}

protected:
	std::vector<weight> net;
	float alpha;
	std::vector<step> replay_buffer;
};

/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */
class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }), popup(0, 9) {}

	virtual action take_action(const board& after) {
		std::shuffle(space.begin(), space.end(), engine);
		for (int pos : space) {
			if (after(pos) != 0) continue;
			board::cell tile = popup(engine) ? 1 : 2;
			return action::place(pos, tile);
		}
		return action();
	}

private:
	std::array<int, 16> space;
	std::uniform_int_distribution<int> popup;
};

/**
 * dummy player
 * select a legal action randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward != -1) return action::slide(op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};

class reward_greedy_player : public agent {
public:
	reward_greedy_player(const std::string& args = "") : agent(args), opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
		//std::shuffle(opcode.begin(), opcode.end(), engine);
		int op_best = -1;
		board::reward reward_best = -1;
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward > reward_best){
				reward_best = reward;
				op_best = op;
			}
		}
		if (op_best != -1) return  action::slide(op_best);
		else return action();
	}

private:
	std::array<int, 4> opcode;
};

class merge_greedy_player : public agent {
public:
	merge_greedy_player(const std::string& args = "") : agent(args), opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
		// std::shuffle(opcode.begin(), opcode.end(), engine);
		int op_merge_best = -1;
		int merge_best = -1;

		for (int op : opcode) {
			auto temp = board(before);
			board::reward reward = temp.slide(op);
			int merge_count = temp.get_merget_count();
			//std::cout << "merge count" << merge_count << std::endl;
			//std::cout << "op: " << op << "berfore:" << empty_cell_before << ", after:" << empty_cell << std::endl;
			if (merge_count >= merge_best && reward > -1){
				merge_best = merge_count;
				op_merge_best = op;
			}
		}
		//std::cout << op_reward_best << std::endl;
		//if (op_space_best != -1) return  action::slide(op_space_best);
		if (op_merge_best != -1) return  action::slide(op_merge_best);
		else return action();
	}

private:
	std::array<int, 4> opcode;
};


class two_step_greedy_player : public agent {
public:
	two_step_greedy_player(const std::string& args = "") : agent(args), opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
		int op_best = -1;
		board::reward reward_best = -1;

		for (int op : opcode) {
			auto temp = board(before);
			board::reward reward = temp.slide(op);
			board::reward reward_2nd_best = -1;

			for (int op : opcode) {
				auto temp2 = board(temp);
				board::reward reward_2nd = temp2.slide(op);
				if (reward_2nd > reward_2nd_best) reward_2nd_best = reward_2nd;
			}
			//std::cout << reward_2nd_best << std::endl;
			if(reward + reward_2nd_best >= reward_best ){
				op_best = op;
				reward_best = reward + reward_2nd_best;
			}
		}
		if (op_best != -1) return  action::slide(op_best);
		else return action();
	}

private:
	std::array<int, 4> opcode;
};

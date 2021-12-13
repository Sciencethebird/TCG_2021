#pragma once
#include "board.h"
#include <vector>
#include <string>
#include <memory>
#include <iostream>



board::piece_type opponent_of(board::piece_type me){
    if(me == board::black) return board::white;
    else return board::black;
}
class node{
public:
    // static counters for debugging purpose
    static int simulation_count;
    static int node_count;
public:
    //typedef std::shared_ptr<node> node_ptr;
    typedef node* node_ptr;
public:
    // constructors and destructors
    node(){}; // default constructor
    node(board::piece_type who, const board& b, int size): visit_count(0), win_count(0), space_size(size){
        me = who;
        opponent = opponent_of(me);
        parent = nullptr;
        state = b;
        //std::cout << "constructor \n" << state << std::endl; 
        simulation_count+=1;
        node_count=0;
        //std::cout << "top node constructed, simulation count = " << simulation_count << std::endl;
        find_available_move();
    }
    node(board::piece_type who, node_ptr p, const board& b, action::place ac, int size): visit_count(0), win_count(0), space_size(size){
        node_count++;

        me = who;
        opponent = opponent_of(me);
        parent = p;
        state = b;
        applied_action = ac;

        //std::cout << "new node created, num = " << node_count << std::endl;
        find_available_move();
    }
    ~node(){
        //std::cout << "destructor called, " << node_count-- << "node left" << std::endl;
    }
public:
    // check availiable moves
    void find_available_move(){
		for (int i = 0; i< space_size; i++) {
            board after = state;
            action::place new_action(i, opponent);
            //std::cout << "test" << after << std::endl;
			if (new_action.apply(after) == board::legal){
				available_space.push_back(new_action);
                //std::cout << "availiable move\n" <<after << std::endl;
			}
            
            if(available_space.size() == 0){
                is_terminal = true;
            } else{
                is_terminal = false;
            }
		}
    }

    bool is_fully_expanded(){
        // terminal node is also fully expanded
        return (available_space.size() == 0);
    }
    bool is_terminal_node(){
        return is_terminal;
    }
    //node_ptr get_curr_ptr(){
    //    return shared_from_this();
    //}
    node_ptr create_child(){
        action::place curr_action = available_space.back();
        available_space.pop_back();
        //std::shared_ptr<Base> p = std::make_shared<Derived>()
        //std::cout << "before create child\n" << state << std::endl;
        board after = state;
        curr_action.apply(after); // maybe store board?????
        node_ptr new_node_ptr = new node(opponent, this, after, curr_action, space_size);
        //std::cout << "new child's board state\n" << after << std::endl;
        children.push_back(new_node_ptr);
        return new_node_ptr;
        // pop a available action
    }
    double get_winrate(){
        return (double) win_count / (double) visit_count;
    }
public: 
    // parent and child nodes
    node_ptr parent;
    std::vector<node_ptr> children; // 
public:
    // properties
    unsigned long long visit_count; // visit count of this node
    unsigned long long win_count;  // how many wins
    board::piece_type me;
    board::piece_type opponent;
    board state;
    int space_size;
    bool is_terminal; // is the end of the game node
    std::vector<action::place> available_space;
    action::place applied_action;
    //int available_moves_count;
};

int node::simulation_count = 0;
int node::node_count = 0;
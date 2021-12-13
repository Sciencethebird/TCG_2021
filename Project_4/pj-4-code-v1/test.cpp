/**
 * Framework for NoGo and similar games (C++ 11)
 * nogo.cpp: Main file for the framework
 *
 * Author: Theory of Computer Games (TCG 2021)
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <memory>
#include "board.h"
#include "action.h"
#include "agent.h"
#include "episode.h"
#include "statistic.h"
#include "node.h"

class seeb{
public:
    seeb(int num):num_seeb(num){}
    ~seeb(){
        std::cout << "seeb desctructor called" << std::endl;
    }
private:
    int num_seeb;
};

class birb{
public:
    birb(){
        seebs = std::make_shared<seeb>(3);
        std::cout << "new birb created" << std::endl;
    } 
    ~birb(){
        std::cout << "birb destrcutor called" << std::endl;
    }
private:
    std::shared_ptr<seeb> seebs;
};

std::shared_ptr<birb> create_local_birb(){
    auto borb = std::make_shared<birb>();
    return borb;
}
int main(int argc, const char* argv[]) {
    std::cout << "main" << std::endl;
    auto b = create_local_birb();
    std::cout << "end1" << std::endl;
    b.reset();
    std::cout << "end2" << std::endl;
	return 0;
}

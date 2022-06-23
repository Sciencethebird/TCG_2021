.PHONY: all test
all:
	#g++ -std=c++11 -O3 -g -Wall -fmessage-length=0 -o nogo nogo.cpp
	g++ -std=c++11 -O3 -g -Wall -fmessage-length=0 -o nogo-mcts nogo-mcts.cpp -lpthread
	# g++ -std=c++11 -O3 -g -Wall -fmessage-length=0 -o test test.cpp
test:
	#./nogo-mcts --total=1 --black="search=MCTS timeout=1000 worker=1" --white="seed=54321"
	#./nogo-mcts --total=1 --black="search=MCTS timeout=1000 worker=4 exploration=0.5" --white="seed=54321"
	./nogo-mcts --total=1 --black="search=MCTS timeout=1000 worker=1 exploration=0.5" --white="seed=54321"
	#./nogo --total=1 --black="seed=12345" --white="seed=54321"
clean:
	rm nogo
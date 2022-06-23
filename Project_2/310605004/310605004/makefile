all:
	g++ -std=c++11 -O3 -g -Wall -fmessage-length=0 -o 2584 2584.cpp
run:
	./2584 --save=stat.txt --play=load
train:
	./2584 --save=stat.txt --play="init alpha=0.0025" --total=1000
train_and_eval:
	bash ./train.sh
clean:
	rm 2584

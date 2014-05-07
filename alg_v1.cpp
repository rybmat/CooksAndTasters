#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <mpi.h>
#include <unistd.h>
#include <ctime>

using namespace std;

struct Process {
	int tid;
	int clk;
	int team;
	Process() : tid(-1), clk(-1){}
};

const int ROOT = 0, ITERS_NUM = 5;
enum MSG_TAGS {TEAM, TASTERS, MEAL, ROOM_PLACE};

int size, K, S, P, Z, D;
int t[2];
Process me;

int find_index(vector<Process>& v, int tid) {
	for (int i = 0; i < v.size(); ++i) {
		if (v[i].tid == tid)
			return i;
	}
	return -1;
}

bool process_comp(Process p1, Process p2) {
	if (p1.clk < p2.clk) {
		return true;
	}
	if (p2.clk < p1.clk) {
		return false;
	}
	if (p1.tid < p2.tid) {
		return true;
	} else {
		return false;
	}
}

bool init(int argc, char** argv) { 
	K = atoi(argv[1]);
	D = size - K;
	S = atoi(argv[2]);
	P = atoi(argv[3]);
	Z = atoi(argv[4]);

	me.clk = 0;

	ostringstream message("");
	if ( D <= S*P) {
		message << "number of tasters should be greater than number of rooms multiplied by number of seats per room (D > S*P) (D\n";
	}
	if (Z > P) {
		message << "number of cooks in one team should be less than or equal to number of seats per room (Z<=P)\n";
	}
	if (K/Z <= S) {
		message << "number of teams should be greater than number of rooms (K/Z > S)\n";
	}

	if (me.tid == 0) {
		cout << "K: " << K << endl << "D: " << D << endl << "S: " << S << endl << "P: " << P << endl << "Z: " << Z << endl;
	}
	if (message.str() == "") {
		if (me.tid == ROOT) {
			cout << "Init: OK" << endl;
		}
		return true;
	} else {
		if (me.tid == ROOT) {
			cout << message.str() << endl << "Init: FAILED" << endl;
		}
		return false;
	}

return true;
	
}

void clean_up(int tid, int room) {
	cout << " cook: " << tid << " cleans_up room " << room << endl;
	::sleep(3);
}

void eat(int tid, int room) {
	cout << "taster: " << tid << " eats in room " << room << endl;
	::sleep(3);
}

void write_mark(int tid, int room) {
	cout << "tid: " << tid << " mark for room " << room << ": " << rand() % 10 << endl;
}

void cook() {
	srand(time(NULL));
	vector<Process> cooks;
	vector<int> tasters;
	int team_id, room_id;
	Process p;
	ostringstream fname;
	fname << "log/cook_" << me.tid << ".log";
	ofstream log_file(fname.str().c_str(),fstream::app|fstream::out);
	log_file << "cook: tid " << me.tid << endl;

	for (int iter = 0; iter < ITERS_NUM; ++ iter) {
		//send my tid and clk to other cooks
		for (int i = 0; i < K; ++i) {
			t[0] = me.clk;
			t[1] = me.tid;
			if (i != me.tid)
				MPI::COMM_WORLD.Send(t, 2, MPI_INT, i, TEAM);
		}


					log_file << "cook " << me.tid << ": tids to other cooks sent" << endl;

		cooks.push_back(me);

 		//receiving tid and clk from other cooks	
		for (int i = 0; i < K; ++i) {
			if (i != me.tid) {
				MPI::COMM_WORLD.Recv(t, 2, MPI_INT, MPI_ANY_SOURCE, TEAM);
				p.clk = t[0];
				p.tid = t[1];
				cooks.push_back(p);
			}
		}
		//sort cooks table
		sort(cooks.begin(), cooks.end(), process_comp);

					log_file << "cook " << me.tid << ": tids from other cooks received and sorted" << endl;

					log_file << "tid : clk" << endl;
					for (vector<Process>::iterator it = cooks.begin(); it != cooks.end(); ++it) {
						log_file << it->tid << " : " << it->clk << endl;
					}

		int ind = find_index(cooks, me.tid);
		if (ind < 0) 
			exit(-1);
		team_id = ((ind/Z) < S) ? (ind/Z) : -1;

		if (team_id == -1) {
					log_file << "cook " << me.tid << ": continue" << endl;
					log_file << "==================================" << endl;
			cooks.clear();
			continue;
		}
		room_id = team_id;

					log_file << "cook " << me.tid << ": room_id " << room_id << endl;

		//send tid and room_id to tasters
		t[0] = room_id;
		t[1] = me.tid;
		for (int i = K; i < K+D; ++i)
        	MPI::COMM_WORLD.Send(t, 2, MPI_INT, i, TASTERS);

        			log_file << "cook " << me.tid << ": tid and room_id sent to tasters " << endl;
		
        //wait for fill the room by tasters
        int id;
        for (int i = 0; i < P; ++i) {
        	MPI::COMM_WORLD.Recv(&id, 1, MPI_INT, MPI_ANY_SOURCE, TASTERS);
        	tasters.push_back(id);
        			
        			log_file << i << " ";
        }
        log_file << endl;

        			log_file << "cook " << me.tid << ": tasters in room " << room_id << endl;
        			for (vector<int>::iterator it = tasters.begin(); it != tasters.end(); ++it) {
						log_file << *it << ", ";
					}
					log_file << endl;

        //send msg that tasters can eat to tasters
   		int tmp = 0;
        for (vector<int>::iterator it = tasters.begin(); it != tasters.end(); ++it) {
        	MPI::COMM_WORLD.Send(&tmp, 1, MPI_INT, *it, MEAL);
        }
		
					log_file << "cook " << me.tid << ": tasters can eat " << endl;

		// wait till tasters go out
        for (int i = 0; i < P; ++i) {
        	MPI::COMM_WORLD.Recv(&id, 1, MPI_INT, MPI_ANY_SOURCE, MEAL);
        }

        			log_file << "cook " << me.tid << ": tasters out of room " << endl;

        clean_up(me.tid, room_id);

        me.clk += rand()%10 + 1;

        			log_file << "cook " << me.tid << ": clk=" << me.clk << endl;

		tasters.clear();
		cooks.clear();
					log_file << "==================================" << endl;
	}
}

void taster() {
	srand(time(NULL));
	vector<Process> tasters;
	vector<int> cooks;
	Process p;
	int room_id;

	ostringstream fname;
	fname << "log/taster_" << me.tid << ".log";
	ofstream log_file(fname.str().c_str(),fstream::app|fstream::out);

	log_file << "taster: tid " << me.tid << endl;

	for (int iter = 0; iter < ITERS_NUM; ++ iter) {
		//send clk and tid to other tasters
		for (int i = K; i < K + D; ++i) {
			t[0] = me.clk;
			t[1] = me.tid;
			if (i != me.tid)
				MPI::COMM_WORLD.Send(t, 2, MPI_INT, i, ROOM_PLACE);
		}
					log_file << "taster " << me.tid << ": tids to other tasters sent" << endl;

		tasters.push_back(me);


		//receive clk and tid from other tasters
					log_file << "receiving tids and clk from other tasters" << endl;
		for (int i = K; i < K + D; ++i) {
			if (i != me.tid) {
				MPI::COMM_WORLD.Recv(t, 2, MPI_INT, MPI_ANY_SOURCE, ROOM_PLACE);
				p.clk = t[0];
				p.tid = t[1];

					log_file << p.tid << " : " << p.clk << endl;
				tasters.push_back(p);
			}
		}

					log_file << "taster " << me.tid << ": tids from other tasters received (unsorted)" << endl;
					log_file << "tid : clk" << endl;
					//for (vector<Process>::iterator it = tasters.begin(); it != tasters.end(); ++it) {
					//	log_file << it->tid << " : " << it->clk << endl;
					//}
		sort(tasters.begin(), tasters.end(), process_comp);

					log_file << "taster " << me.tid << ": tids from other tasters received and sorted" << endl;
					log_file << "tid : clk" << endl;
					for (vector<Process>::iterator it = tasters.begin(); it != tasters.end(); ++it) {
						log_file << it->tid << " : " << it->clk << endl;
					}

		int ind = find_index(tasters, me.tid);
		room_id = ((ind / P) < S) ? (ind / P) : -1;
		
		if (room_id == -1) {
					
					log_file << "taster " << me.tid << ": continue" << endl;
					log_file << "==================================" << endl;
			
			for (int i = 0; i < S*Z; ++i) 
				MPI::COMM_WORLD.Recv(t, 2, MPI_INT, MPI_ANY_SOURCE, TASTERS);
			tasters.clear();
			continue;
		}

					log_file << "taster " << me.tid << ": room_id " << room_id << endl;

		//receive from cooks in which room they serve
					
					log_file << "cooks and rooms:" << endl;
			
		for (int i = 0; i < S*Z; ++i) {
			MPI::COMM_WORLD.Recv(t, 2, MPI_INT, MPI_ANY_SOURCE, TASTERS);
					
					log_file << i << "room:" << t[0] << " cook:" << t[1] << endl;
			
			if (t[0] == room_id) {
				cooks.push_back(t[1]);
			}
		}

					log_file << "taster " << me.tid << ": cooks tids and room_ids received. My cooks: " << endl;
					for (vector<int>::iterator it = cooks.begin(); it != cooks.end(); ++it) {
						log_file << *it << ", ";
					}
					log_file << endl;

		//send to cooks from my room that i'm in
		for (vector<int>::iterator it = cooks.begin(); it != cooks.end(); ++it) {
			MPI::COMM_WORLD.Send(&me.tid, 1, MPI_INT, *it, TASTERS);
		}

					log_file << "taster " << me.tid << ": msg to cooks, that i'm in sent" << endl;

		//recv that cooks are out
		int tt;
		for (int i = 0; i < Z; ++i) {
			MPI::COMM_WORLD.Recv(&tt, 1, MPI_INT, MPI_ANY_SOURCE, MEAL);
		}
		//cout <<"iter " <<iter <<endl;
					log_file << "taster " << me.tid << ": cooks are out received" << endl;

		eat(me.tid, room_id);
		write_mark(me.tid, room_id);

		//send to cooks that i'm out
		for (vector<int>::iterator it = cooks.begin(); it != cooks.end(); ++it) {
			MPI::COMM_WORLD.Send(&me.tid, 1, MPI_INT, *it, MEAL);
		}

					log_file << "taster " << me.tid << ": msg to cooks, that i'm out sent" << endl;

		me.clk += rand()%10 + 1;

					log_file << "taster " << me.tid << ": clk=" << me.clk << endl;

		tasters.clear();
		cooks.clear();

					log_file << "==================================" << endl;
	}

}

int main(int argc, char** argv) {
	MPI::Init(argc, argv);	
	size=MPI::COMM_WORLD.Get_size();
	me.tid=MPI::COMM_WORLD.Get_rank();

	if (!init(argc, argv)) {
		MPI::Finalize();
		return 0;
	}

	//srand(time(NULL));

	if (me.tid < K) {
		cook();
	} else {
		taster();
	}

	//cout <<"Hello world: " << tid <<" of " << size << endl;
	MPI::Finalize();

	return 0;
}

// akemi's Nodeless Solver (C) 2018 Fixstars Corp.
// g++ akemi.cpp -std=c++14 -o akemi -O3 -Wall -Wno-unused-but-set-variable
#define DEBUG
#define DEBUG_PRINT
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <algorithm>
#include <utility>

#include "json.hpp"

using namespace std;

constexpr const char* version	= "0.04";
constexpr const char* revision = "a";
constexpr const char* ver_date = "20181211";

//#define DEBUG_PRINT

void remove_newline(std::string& s)
{
	std::string target("\n");
	std::string::size_type pos = s.find(target);
	while (pos != std::string::npos) {
		s.replace(pos, target.size(), "");
		pos = s.find(target, pos);
	}
}

struct Node {
	std::vector<Node*> neighbors;
	int idx;
	int player;
	int number;
	int value;

		Node(int idx, int player, int number) : idx(idx), player(player), number(number) {
		value = 0;
	}
};

class Graph {
private:
	void count_groups_dfs(int cur, vector<bool>& used){
		for(const auto& node: nodes[cur]->neighbors){
			if( used[node->idx] == false ){
				used[node->idx] = true;
				count_groups_dfs(node->idx, used);
			}
		}
	}
public:
	typedef std::map<int, Node*> nodes_t;
		nodes_t nodes;
		vector<int> representative_idx_of_each_groups;
		map<int,int> degree;

		int num_groups(){
			if( representative_idx_of_each_groups.size()!=0 )return representative_idx_of_each_groups.size();
			else{
				vector<bool> used(nodes.size(),false);
				for(const auto& node: nodes){
					if( used[node.first] == false ){
						representative_idx_of_each_groups.push_back( node.first );
						used[node.first] = true;
						count_groups_dfs(node.first, used);
					}
				}
			}
			return representative_idx_of_each_groups.size();
		}

		bool is_path_graph(int cur, vector<bool>& used){	//i頂点curを含む連結なグラフはpath_graphかどうか
			bool ret = true;
			if( degree[cur]>2 )return false;
			for(const auto& node: nodes[cur]->neighbors){
				if( used[node->idx]==true )return false;
				used[node->idx] = true;
				ret &= is_path_graph(node->idx, used);
			}
			return ret;
		}

		void add_node(const int idx) {
			nodes_t::iterator it = nodes.find(idx);
			if (it == nodes.end()) {
					Node *node;
					node = new Node(idx, -1, 0);
					nodes[idx] = node;

					//新しく追加された頂点の次数は0
					degree[idx] = 0;
					assert( nodes[idx]->neighbors.size()==0 );

					return;
			}
			std::cerr << "Node already exists!" << std::endl;
		}

		void add_edge(const int from, const int to) {
			Node* f = (nodes.find(from)->second);
			Node* t = (nodes.find(to)->second);
			f->neighbors.push_back(t);
			t->neighbors.push_back(f);

			//辺を追加することで隣接する2つの頂点の次数がそれぞれ増加する
			degree[f->idx]++;
			degree[t->idx]++;
		}
		int num_nodes() const { return static_cast<int>(nodes.size()); }
};

std::vector<int> initial_positions(Graph* G, int num_units)
{
	std::vector<int> positions;

	#ifdef DEBUG
		vector<bool> used(G->nodes.size());
		for( const auto& idx: G->representative_idx_of_each_groups ){
			assert( G->is_path_graph(idx, used)==false );	//入力される全てのグラフはpath_graphではない
		}
	#endif

	vector<int> idx_of_leaves;
	for(const auto &node : G->nodes){
		if( G->degree[node.first]<=1 ){
			idx_of_leaves.push_back( node.first );
		}
	}

	if( idx_of_leaves.size()!=0 ){
		for(const auto& idx: idx_of_leaves){
			//手駒が無くなったらbreak
			if( num_units - positions.size() == 0 )break;
			int prev = -1;
			int cur = idx;
			if( G->degree[cur]==0 )continue;

			assert( G->degree[cur]==1 );
			while( G->degree[cur]<=2 ){
				if( G->degree[cur]==1 || G->nodes[cur]->neighbors[1]->idx == prev ){
					prev = cur;
					cur = G->nodes[cur]->neighbors[0]->idx;
				}else{
					if( G->nodes[cur]->neighbors[0]->idx != prev ){
						cerr<<G->nodes[cur]->neighbors[0]->idx<<" "<<G->nodes[cur]->neighbors[1]->idx<<" "<<prev<<endl;
					}
					assert( G->nodes[cur]->neighbors[0]->idx == prev );
					prev = cur;
					cur = G->nodes[cur]->neighbors[1]->idx;
				}
			}
			positions.push_back(cur);
		}
	}

	//余ってたら、最後にpositionsにつっこんだ頂点にコマをすべて置く
	int remains = num_units - positions.size();
	for (int i = 0; i < remains; i++) {
		if( positions.size()>=1 ){
			positions.push_back(positions.front());
		}else{
			positions.push_back(0);
		}
	}
	std::sort(positions.begin(), positions.end());

	// #ifdef DEBUG_PRINT
	// 	for (const auto& node : priority_nodes) {
	// 		std::cerr << node.second->value << ", " << node.second->idx << ", " << node.second->neighbors.size() << ", " << node.second->player << std::endl; ///// debug
	// 	}
	// #endif // DEBUG_PRINT

	assert( num_units == positions.size() );
	return positions;
}
/*
	std::vector<std::pair<int, Node*>> priority_nodes;

	for (const auto& node : G->nodes) {
		int num_neighbors = node.second->neighbors.size();
		//次数が低い頂点に隣接しており、かつ次数が高い頂点のスコアが高くなる
		std::for_each(node.second->neighbors.begin(), node.second->neighbors.end(),
			[num_neighbors](const auto& neighbor){ neighbor->value += 100 / num_neighbors; });
	}
	for (const auto& node : G->nodes) {
		priority_nodes.push_back(std::make_pair(node.second->value, node.second));
	}
	std::sort(priority_nodes.begin(), priority_nodes.end(), [](const auto& a, const auto& b){ return a.first > b.first; });
	for (const auto& node : priority_nodes) {
		positions.push_back(node.second->idx);
		if (static_cast<int>(positions.size()) >= num_units) break;
	}
	return positions;
*/


void solver(Graph* G, int& src, int& dst, int& num)
{
	std::vector<std::pair<int, Node*>> priority_nodes;
	for (const auto& node : G->nodes) {
		int value = std::count_if(node.second->neighbors.begin(), node.second->neighbors.end(),
			[](const auto& neighbor){ return neighbor->player < 0; });
		priority_nodes.push_back(std::make_pair(value, node.second));
	}
	std::sort(priority_nodes.begin(), priority_nodes.end(), [](const auto& a, const auto& b){ return a.first > b.first; });

	for (const auto& node : priority_nodes) {
		if (node.second->number > 0 && !node.second->neighbors.empty()) {
			for (const auto& neighbor : node.second->neighbors) {
				if (neighbor->player == -1) {
					src = node.second->idx;
					dst = neighbor->idx;
					num = node.second->number;
					return;
				}
			}
		}
	}
	src = -1;
	dst = -1;
	num = -1;
}

int main(int argc, char* argv[])
{
	std::string s;

	Graph* G = new Graph();

	for (;;) {
		getline(std::cin, s);
		nlohmann::json obj = nlohmann::json::parse(s);
		auto action = obj["action"];

		if (action == "play") {
			auto nodes(obj["nodes"]);
			auto edges(obj["edges"]);
			auto state(obj["state"]);
			auto units(obj["units"]);

			for (int i = 0; i < G->num_nodes(); i++) {
				G->nodes[i]->player = state[i];
				G->nodes[i]->number = units[i];
			}

			int src;
			int dst;
			int num;
			solver(G, src, dst, num);

			nlohmann::json out;
			out["src"] = src;
			out["dst"] = dst;
			out["num"] = num;
			std::string json(out.dump());
			remove_newline(json);
			std::cout << json << std::endl << std::flush;

#ifdef DEBUG_PRINT
			std::cerr << "akemi: " << json << std::endl; ///// debug
#endif // DEBUG_PRINT

		} else if (action == "init") {
			int uid(obj["uid"]);
			auto names(obj["names"]);
			auto name(names[uid]);
			auto num_units(obj["num_units"]);
			auto nodes(obj["nodes"]);
			auto edges(obj["edges"]);
			auto state(obj["state"]);
			auto units(obj["units"]);

			for (const auto& node : nodes) {
				G->add_node(node);
			}
			for (const auto& edge : edges) {
				G->add_edge(edge[0], edge[1]);
			}

			std::vector<int> positions(initial_positions(G, num_units));

			nlohmann::json out;
			out["positions"] = positions;
			std::string json(out.dump());
			remove_newline(json);
			std::cout << json << std::endl << std::flush;

		} else if (action == "quit") {
			std::cout << std::endl << std::flush;
			break;
		}
	}

	delete G;

	return 0;
}

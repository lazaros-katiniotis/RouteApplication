#pragma once
#ifndef ROUTE_APP_PATHFINDER_H
#define ROUTE_APP_PATHFINDER_H

#include "Model.h"

using namespace std;
namespace route_app {
	class Pathfinder {
	private:
		Model* model_;
		int start_node_index_;
		int end_node_index_;
		double* node_distance_from_start_;
		map<Model::Node, int> open_list_;
		set<int> closed_list_;
		vector<Model::Node> nodes_;
		unordered_map<int, vector<int>> node_number_to_road_numbers_;

		bool CheckPreviousNode(vector<int>::iterator it, vector<int>::iterator begin);
		bool CheckNextNode(vector<int>::iterator it, vector<int>::iterator end);
		int FindNearestRoadNode(Model::Node node);
		bool StartAStarSearch();
		vector<int> DiscoverNeighbourNodes(int current);
	public:
		Pathfinder(Model* model, AppData* data);
		~Pathfinder();
		void Initialize(AppData* data);
		void Release();
		void CreateRoute();
		double EuclideanDistance(Model::Node const node, Model::Node const other);
	};

	double inline Pathfinder::EuclideanDistance(Model::Node const node, Model::Node const other) {
		return sqrt(pow(node.x - other.x, 2) + pow(node.y - other.y, 2));
	}
}

#endif
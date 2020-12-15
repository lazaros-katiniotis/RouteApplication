#include "Helper.h"
#include "Pathfinder.h"

using namespace route_app;

Pathfinder::Pathfinder(Model* model, AppData* data) {
	model_ = model;
	node_number_to_road_numbers_ = model_->GetNodeNumberToRoadNumber();
	nodes_ = model_->GetNodes();
	Initialize(data);
}

void Pathfinder::Initialize(AppData* data) {
	size_t size = nodes_.size();
	node_distance_from_start_ = new double[size];
	for (int i = 0; i < size; i++) {
		node_distance_from_start_[i] = 0.0f;
	}
	model_->InitializePoint(model_->GetStartingPoint(), data->start);
	model_->InitializePoint(model_->GetEndingPoint(), data->end);
}

void Pathfinder::CreateRoute() {
	PrintDebugMessage(APPLICATION_NAME, "Pathfinder", "Creating route...", true);

	start_node_index_ = FindNearestRoadNode(model_->GetStartingPoint());
	end_node_index_ = FindNearestRoadNode(model_->GetEndingPoint());

	bool found = false;
	if (start_node_index_ != -1 && end_node_index_ != -1) {
		found = StartAStarSearch();
		open_list_.clear();
		closed_list_.clear();
	}

	delete[] node_distance_from_start_;

	if (found) {
		model_->GetRoute().nodes.clear();
		auto nodes = nodes_;
		Model::Node node_it = nodes[end_node_index_];
		model_->GetRoute().nodes.emplace_back(end_node_index_);
		set<int> visited_nodes_indices;
		while (node_it.parent != -1) {
			if (auto index = visited_nodes_indices.find(node_it.parent); index != visited_nodes_indices.end()) {
				break;
			}
			model_->GetRoute().nodes.emplace_back(node_it.parent);
			visited_nodes_indices.insert(node_it.parent);
			node_it = nodes[node_it.parent];
		}
		visited_nodes_indices.clear();
	}
}

bool Pathfinder::StartAStarSearch() {
	open_list_[nodes_[start_node_index_]] = start_node_index_;
	while (!open_list_.empty()) {
		int current = open_list_.begin()->second;
		open_list_.erase(open_list_.begin());
		double f = 0.0f;
		double h = 0.0f;
		double distance = 0.0f;
		vector<int> new_neighbour_nodes = DiscoverNeighbourNodes(current);
		for (auto it = new_neighbour_nodes.begin(); it != new_neighbour_nodes.end(); it++) {
			distance = EuclideanDistance(nodes_[*it], nodes_[current]);
			node_distance_from_start_[*it] = node_distance_from_start_[current] + distance;
			h = EuclideanDistance(nodes_[*it], nodes_[end_node_index_]);
			f = node_distance_from_start_[*it] + h;

			if (*it == end_node_index_) {
				nodes_[*it].f = f;
				nodes_[*it].h = h;
				nodes_[*it].parent = current;
				return true;
			}

			bool already_in_open_list = false;
			if (auto other_it = open_list_.find(nodes_[*it]); other_it != open_list_.end()) {
				already_in_open_list = true;
				if (nodes_[other_it->second].f < f) {
					continue;
				}
			}

			if (auto other_it = closed_list_.find(*it); other_it != closed_list_.end()) {
				if (nodes_[*other_it].f < f) {
					continue;
				}
			}
			nodes_[*it].f = f;
			nodes_[*it].h = h;
			nodes_[*it].parent = current;
			if (already_in_open_list) {
				open_list_.erase(nodes_[*it]);
			}
			open_list_[nodes_[*it]] = *it;
		}
		new_neighbour_nodes.clear();
		closed_list_.insert(current);
	}
	return false;
}

int Pathfinder::FindNearestRoadNode(Model::Node node) {
	int closest_node_index = -1;
	double minimum_distance = INFINITY;
	double distance = 0;;
	for (auto it = node_number_to_road_numbers_.begin(); it != node_number_to_road_numbers_.end(); it++) {
		distance = EuclideanDistance(node, nodes_[it->first]);
		if (distance < minimum_distance) {
			minimum_distance = distance;
			closest_node_index = it->first;
		}
	}
	model_->GetRoute().nodes.emplace_back(closest_node_index);
	return closest_node_index;
}

vector<int> Pathfinder::DiscoverNeighbourNodes(int current) {
	vector<int> neighbour_nodes;
	auto node_number_to_road_number = model_->GetNodeNumberToRoadNumber();
	auto& roads = node_number_to_road_number[current];
	for (auto road = roads.begin(); road != roads.end(); road++) {
		int index = *road;
		int way = model_->GetRoads()[index].way;
		auto ways = model_->GetWays();
		for (auto it = ways[way].nodes.begin(); it != ways[way].nodes.end(); it++) {
			if (current == *it) {
				if (CheckPreviousNode(it, ways[way].nodes.begin())) {
					neighbour_nodes.emplace_back(*std::prev(it));
				}
				if (CheckNextNode(it, ways[way].nodes.end())) {
					neighbour_nodes.emplace_back(*std::next(it));
				}
				break;
			}
		}
	}
	return neighbour_nodes;
}

bool Pathfinder::CheckPreviousNode(vector<int>::iterator it, vector<int>::iterator begin) {
	if (it != begin) {
		return true;
	}
	return false;
}

bool Pathfinder::CheckNextNode(vector<int>::iterator it, vector<int>::iterator end) {
	if (it + 1 != end) {
		return true;
	}
	return false;
}

void Pathfinder::Release() {

}

Pathfinder::~Pathfinder() {
	Release();
}
#include <iostream>
#include <pugixml.hpp>
#include <locale>
#include <cmath>
#include <algorithm>
#include "Model.h"
#include "Helper.h"
#include <chrono>

using namespace pugi;
using namespace route_app;

static Model::Road::Type StringToRoadType(string_view type) {
	if (type == "motorway")        return Model::Road::Motorway;
	if (type == "motorway_link")   return Model::Road::Motorway;
	if (type == "trunk")           return Model::Road::Trunk;
	if (type == "primary")         return Model::Road::Primary;
	if (type == "secondary")       return Model::Road::Secondary;
	if (type == "tertiary")        return Model::Road::Tertiary;
	if (type == "residential")     return Model::Road::Residential;
	if (type == "living_street")   return Model::Road::Residential;
	if (type == "service")         return Model::Road::Service;
	if (type == "unclassified")    return Model::Road::Unclassified;
	if (type == "footway")         return Model::Road::Footway;
	if (type == "bridleway")       return Model::Road::Footway;
	if (type == "steps")           return Model::Road::Footway;
	if (type == "path")            return Model::Road::Footway;
	if (type == "pedestrian")      return Model::Road::Footway;
	if (type == "cycleway")        return Model::Road::Cycleway;
	return Model::Road::Invalid;
}

static Model::Landuse::Type StringToLanduseType(string_view type) {
	if (type == "commercial")      return Model::Landuse::Commercial;
	if (type == "construction")    return Model::Landuse::Construction;
	if (type == "grass")           return Model::Landuse::Grass;
	if (type == "forest")          return Model::Landuse::Forest;
	if (type == "industrial")      return Model::Landuse::Industrial;
	if (type == "railway")         return Model::Landuse::Railway;
	if (type == "residential")     return Model::Landuse::Residential;
	return Model::Landuse::Invalid;
}

Model::Model(AppData* data) {
	PrintDebugMessage(APPLICATION_NAME, "Model", "Initiating model...", false);
	if (OpenDocument(data)) {
		ParseData(data);
		AdjustCoordinates(data);
		model_created_ = true;
	}
	else {
		PrintDebugMessage(APPLICATION_NAME, "Model", "Error: Failed to parse the xml file.", false);
		model_created_ = false;
	}
}

void Model::PrintData() {
	PrintDebugMessage(APPLICATION_NAME, "Model", "Printing roads...", true);
	for (int i = 0; i < roads_.size(); i++) {
		cout << "road[" << i << "].way: " << roads_[i].way << endl;
		cout << "road[" << i << "].type: " << roads_[i].type << endl;
		cout << endl;
	}

	PrintDebugMessage(APPLICATION_NAME, "Model", "Printing ways...", true);
	for (int i = 0; i < ways_.size(); i++) {
		cout << "way[" << i << "]:" << endl;
		for (auto it = ways_[i].nodes.begin(); it != ways_[i].nodes.end(); it++) {
			cout << *it;
			if ((it + 1) != ways_[i].nodes.end()) {
				cout << ", ";
			}
		}
		cout << endl;
	}

	PrintDebugMessage(APPLICATION_NAME, "Model", "Printing node_number_to_road_numbers values...", true);
	for (int index = 0; index < nodes_.size(); index++) {
		if (auto it = node_number_to_road_numbers.find(index); it != node_number_to_road_numbers.end()) {
			cout << "node: " << index << endl;
			for (auto road_number = it->second.begin(); road_number != it->second.end(); road_number++) {
				cout << "road name: " << roads_[*road_number].name << "(" << *road_number << ")" << endl;
			}
		}
	}
}

xml_parse_result Model::OpenDocument(AppData* data) {
	xml_parse_result result;
	errno_t err = NULL;

	switch (data->sm) {
	case StorageMethod::FILE_STORAGE:
		CloseFile(data->query_file);
		result = doc_.load_file(data->query_file->filename.c_str());
		break;
	case StorageMethod::MEMORY_STORAGE:
		string memory = data->query_data->memory;
		result = doc_.load_buffer(data->query_data->memory, data->query_data->size);
		break;
	}
	return result;
}

void Model::ParseData(AppData* data) {
	PrintDebugMessage(APPLICATION_NAME, "Model", "Parsing data...", false);

	ParseBounds();

	int index;
	PrintDebugMessage(APPLICATION_NAME, "Model", "Parsing nodes...", false);
	for (const xpath_node& node : doc_.select_nodes("/osm/node")) {
		ParseNode(node.node(), index);
	}

	PrintDebugMessage(APPLICATION_NAME, "Model", "Parsing ways...", false);
	for (const xpath_node& way : doc_.select_nodes("/osm/way")) {
		ParseNode(way.node(), index);

		for (const xpath_node& child : way.node().children()) {
			ParseAttributes(child.node(), index);
		}
	}
}

void Model::ParseBounds() {
	PrintDebugMessage(APPLICATION_NAME, "Model", "Parsing bounds...", false);
	if (auto bounds = doc_.select_nodes("/osm/bounds"); !bounds.empty()) {
		auto node = bounds.first().node();
		min_lat_ = node.attribute("minlat").as_double();
		max_lat_ = node.attribute("maxlat").as_double();
		min_lon_ = node.attribute("minlon").as_double();
		max_lon_ = node.attribute("maxlon").as_double();
	}
	else {
		throw std::logic_error("map's bounds are not defined.");
	}
}

void Model::ParseNode(const xml_node& node, int &index) {
	auto name = string_view{ node.name() };
	auto value = string_view{ node.value() };

	if (name == "node") {
		string id = node.attribute("id").as_string();
		index = (int)nodes_.size();
		node_id_to_number_[id] = index;
		nodes_.emplace_back();
		nodes_.back().x = node.attribute("lon").as_double();
		nodes_.back().y = node.attribute("lat").as_double();
		nodes_.back().f = 0.0f;
		nodes_.back().h = 0.0f;
		nodes_.back().parent = -1;
	}
	else if (name == "way") {
		string id = node.attribute("id").as_string();
		index = (int)ways_.size();
		way_id_to_number_[id] = index;
		ways_.emplace_back();
	}
}

void Model::ParseAttributes(const xml_node& node, int index) {
	string_view name = string_view{ node.name() };

	if (name == "nd") {
		auto ref = node.attribute("ref").as_string();
		if (auto it = node_id_to_number_.find(ref); it != std::end(node_id_to_number_)) {
			ways_.back().nodes.emplace_back(it->second);
		}
	}
	else if (name == "tag") {
		auto category = string_view{ node.attribute("k").as_string() };
		auto type = string_view{ node.attribute("v").as_string() };

		if (category == "highway") {
			if (auto road_type = StringToRoadType(type); road_type != Road::Invalid) {
				roads_.emplace_back();
				roads_.back().way = index;
				roads_.back().type = road_type;

				for (const xpath_node& child : node.parent().children()) {
					auto k = string_view{ child.node().attribute("k").as_string() };
					if (k == "name") {
						roads_.back().name = child.node().attribute("v").as_string();
					}
				}
			}
		} 
		else if (category == "building") {
			buildings_.emplace_back();
			buildings_.back().outer = { index };
		}
		else if (category == "amenity") {
			if (type == "school") {
				buildings_.emplace_back();
				buildings_.back().outer = { index };
			}
		}
		else if (category == "railway") {
			railways_.emplace_back();
			railways_.back().way = index;
		}
		else if (category == "leisure" || (category == "natural" && (type == "wood" || type == "tree_row" || type == "scrub" || type == "grassland")) || (category == "landcover" && type == "grass")) {
			leisures_.emplace_back();
			leisures_.back().outer = { index };
		}
		else if (category == "natural" && (type == "water" || type == "coastline")) {
			waters_.emplace_back();
			waters_.back().outer = { index };
		}
		else if (category == "landuse") {
			if (auto landuse_type = StringToLanduseType(type); landuse_type != Landuse::Invalid) {
				landuses_.emplace_back();
				landuses_.back().outer = { index };
				landuses_.back().type = landuse_type;
			}
		}
	}
}

void Model::CreateRoute() {
	PrintDebugMessage(APPLICATION_NAME, "Model", "Creating route...", false);
	CreateRoadGraph();
	InitializePathfindingData();
	start_node_index_ = FindNearestRoadNode(start_);
	end_node_index_ = FindNearestRoadNode(end_);

	bool found = false;
	if (start_node_index_ != -1 && end_node_index_ != -1) {
		found = StartAStarSearch();
		open_list_.clear();
		closed_list_.clear();
	}

	delete[] node_distance_from_start_;

	if (found) {
		route_.nodes.clear();
		Node node_it = nodes_[end_node_index_];
		route_.nodes.emplace_back(end_node_index_);
		set<int> visited_nodes_indices;
		while (node_it.parent != -1) {
			if (auto index = visited_nodes_indices.find(node_it.parent); index != visited_nodes_indices.end()) {
				break;
			}
			route_.nodes.emplace_back(node_it.parent);
			visited_nodes_indices.insert(node_it.parent);
			node_it = nodes_[node_it.parent];
		}
		visited_nodes_indices.clear();
	}
}

void Model::InitializePathfindingData() {
	int size = nodes_.size();
	node_distance_from_start_ = new double[size];
	for (int i = 0; i < size; i++) {
		node_distance_from_start_[i] = 0.0f;
	}
}

void Model::CreateRoadGraph() {
	PrintDebugMessage(APPLICATION_NAME, "Model", "Creating map of nodes to roads...", false);
	sort(roads_.begin(), roads_.end(), [](const auto& _1st, const auto& _2nd) {
		return (int)_1st.type < (int)_2nd.type;
	});
	for (int i = 0; i < roads_.size(); i++) {
		auto& way = ways_[roads_[i].way];
		for (auto node_number = way.nodes.begin(); node_number != way.nodes.end(); node_number++) {
			node_number_to_road_numbers[*node_number].emplace_back(i);
		}
	}
}

int Model::FindNearestRoadNode(Node node) {
	int closest_node_index = -1;
	double minimum_distance = INFINITY;
	double distance = 0;
	for (auto it = node_number_to_road_numbers.begin(); it != node_number_to_road_numbers.end(); it++) {
		distance = EuclideanDistance(node, nodes_[it->first]);
		if (distance < minimum_distance) {
			minimum_distance = distance;
			closest_node_index = it->first;
		}
	}
	route_.nodes.emplace_back(closest_node_index);
	return closest_node_index;
}

bool Model::StartAStarSearch() {
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

vector<int> Model::DiscoverNeighbourNodes(int current) {
	vector<int> neighbour_nodes;
	auto& roads = node_number_to_road_numbers[current];
	for (auto road = roads.begin(); road != roads.end(); road++) {
		int index = *road;
		int way = roads_[index].way;
		for (auto it = ways_[way].nodes.begin(); it != ways_[way].nodes.end(); it++) {
			if (current == *it) {
				if (CheckPreviousNode(it, ways_[way].nodes.begin())) {
					neighbour_nodes.emplace_back(*std::prev(it));
				}
				if (CheckNextNode(it, ways_[way].nodes.end())) {
					neighbour_nodes.emplace_back(*std::next(it));
				}
				break;
			}
		}
	}
	return neighbour_nodes;
}

bool Model::CheckPreviousNode(vector<int>::iterator it, vector<int>::iterator begin) {
	if (it != begin) {
		return true;
	}
	return false;
}

bool Model::CheckNextNode(vector<int>::iterator it, vector<int>::iterator end) {
	if (it + 1 != end) {
		return true;
	}
	return false;
}

void Model::AdjustCoordinates(AppData* data) {
	PrintDebugMessage(APPLICATION_NAME, "Model", "Projecting node coordinates to cartesian coordinate system...", false);
	const auto pi = 3.14159265358979323846264338327950288;
	const auto deg_to_rad = 2. * pi / 360.;
	const auto earth_radius = 6378137.0;
	const auto lat2ym = [&](double lat) { return log(tan(lat * deg_to_rad / 2 + pi / 4)) / 2 * earth_radius; };
	const auto lon2xm = [&](double lon) { return lon * deg_to_rad / 2 * earth_radius; };
	const auto dx = lon2xm(max_lon_) - lon2xm(min_lon_);
	const auto dy = lat2ym(max_lat_) - lat2ym(min_lat_);
	aspect_ratio_ = dx / dy;
	if (aspect_ratio_ > 1) {
		data->start.y /= GetAspectRatio();
		data->end.y /= GetAspectRatio();
	}
	else {
		data->start.x *= GetAspectRatio();
		data->end.x *= GetAspectRatio();
	}
	const auto min_x = lon2xm(min_lon_);
	const auto min_y = lat2ym(min_lat_);
	metric_scale_ = std::max(dx, dy);

	for (auto& node : nodes_) {
		node.x = (lon2xm(node.x) - min_x) / metric_scale_;
		node.y = (lat2ym(node.y) - min_y) / metric_scale_;
	}
}

void Model::InitializePoint(Node& point, double x, double y) {
	point.x = x;
	point.y = y;
}

void Model::InitializePoint(Node& point, Node& other) {
	point.x = other.x;
	point.y = other.y;
}

void Model::Release() {

}

Model::~Model() {
	Release();
}


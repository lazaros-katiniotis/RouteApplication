#include <iostream>
#include <pugixml.hpp>
#include <locale>
#include <cmath>
#include <algorithm>
#include "Model.h"

//#include <fstream>
//#include <iomanip>


using namespace pugi;

namespace route_app {

	Model::Element::~Element() {
	}

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
		OpenDocument(data);
		ParseData(data);
		AdjustCoordinates();
		//PrintData();
	}

	void Model::PrintDoc(const char* message, pugi::xml_document* doc, pugi::xml_parse_result* result)
	{
		//std::cout
		//	<< message
		//	<< "\t: load result '" << result->description() << "'"
		//	<< ", first character of root name: U+" << std::hex << std::uppercase << setw(4) << setfill('0') << pugi::as_wide(doc->first_child().name())[0]
		//	<< ", year: " << doc->first_child().first_child().first_child().child_value()
		//	<< std::endl;
	}

	void Model::PrintData() {
		//cout << "Printing roads..." << endl;
		//for (Road road : roads_) {
		//	cout << &road << ", " << road.type << ", " << road.way << endl;
		//	auto& way = ways_[road.way];
		//	for (int it : way.nodes) {
		//		cout << "nodes_[" << it << "]: (" << (double)nodes_[it].x << ", " << (double)nodes_[it].y << ")" << endl;
		//	}
		//}
		//for (Way way : ways_) {
		//	for (int it : way.nodes) {
		//		cout << "nodes_[" << it << "]: (" << (double)nodes_[it].x << ", " << (double)nodes_[it].y << ")" << endl;
		//	}
		//}

		PrintDebugMessage(APPLICATION_NAME, "Model", "Printing roads...", true);
		for (int i = 0; i < roads_.size(); i++) {
			cout << "road[" << i << "].way: " << roads_[i].way << endl;
			//cout << "road[" << i << "].type: " << roads_[i].type << endl;
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
				//print nodes that share at least 2 roads
				//if (it->second.size() > 1) {
					cout << "node: " << index << endl;
					for (auto road_number = it->second.begin(); road_number != it->second.end(); road_number++) {
						cout << "road name: " << roads_[*road_number].name << "(" << *road_number << ")" << endl;
					}
				//}
			}
		}
	}

	void Model::OpenDocument(AppData* data) {
		xml_parse_result result;
		errno_t err = NULL;

		switch (data->sm) {
		case StorageMethod::FILE_STORAGE:
			CloseFile(data->query_file);
			result = doc_.load_file(data->query_file->filename.c_str());
			break;
		case StorageMethod::MEMORY_STORAGE:
			string memory = data->query_data->memory;
			//cout << "User-preferred locale setting is " << std::locale("").name().c_str() << '\n';
			//locale loc(std::locale(), new std::codecvt_utf8<char32_t>);
			//memory.imbue(loc);
			result = doc_.load_buffer(data->query_data->memory, data->query_data->size);
			break;
		}
		if (!result) {
			throw std::logic_error("failed to parse the xml file");
		}
	}

	void Model::ParseData(AppData* data) {
		PrintDebugMessage(APPLICATION_NAME, "Model", "Parsing data...", false);

		ParseBounds();

		int index;
		Element* new_node;
		PrintDebugMessage(APPLICATION_NAME, "Model", "Parsing nodes...", false);
		for (const xpath_node& node : doc_.select_nodes("/osm/node")) {
			new_node = ParseNode(node.node(), index);
		}

		Element* new_way;
		PrintDebugMessage(APPLICATION_NAME, "Model", "Parsing ways...", false);
		for (const xpath_node& way : doc_.select_nodes("/osm/way")) {
			new_way = ParseNode(way.node(), index);

			for (const xpath_node& child : way.node().children()) {
				ParseAttributes(child.node(), new_way, index);
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
			throw std::logic_error("map's bounds are not defined");
		}
	}

	Model::Element* Model::ParseNode(const xml_node& node, int &index) {
		auto name = string_view{ node.name() };
		auto value = string_view{ node.value() };

		if (name == "node") {
			string id = node.attribute("id").as_string();
			index = (int)nodes_.size();
			node_id_to_number_[id] = index;
			nodes_.emplace_back();
			nodes_.back().x = node.attribute("lon").as_double();
			nodes_.back().y = node.attribute("lat").as_double();
			return dynamic_cast<Element*>(&nodes_.back());
		}
		else if (name == "way") {
			string id = node.attribute("id").as_string();
			index = (int)ways_.size();
			way_id_to_number_[id] = index;
			ways_.emplace_back();
			return dynamic_cast<Element*>(&ways_.back());
		}
		return NULL;
	}

	void Model::ParseAttributes(const xml_node& node, Element* element, int index) {
		string_view name = string_view{ node.name() };

		if (name == "nd") {
			auto ref = node.attribute("ref").as_string();
			if (auto it = node_id_to_number_.find(ref); it != std::end(node_id_to_number_)) {
				Way* way = dynamic_cast<Way*>(element);
				way->nodes.emplace_back(it->second);
			}
		}
		else if (name == "tag") {
			auto category = string_view{ node.attribute("k").as_string() };
			auto type = string_view{ node.attribute("v").as_string() };

			if (category == "highway") {
				//cout << "parsing highway..." << endl;
				if (auto road_type = StringToRoadType(type); road_type != Road::Invalid) {
					roads_.emplace_back();
					roads_.back().way = index;
					roads_.back().type = road_type;

					//cout << "road index: " << roads_.back().index << endl;
					//cout << "road way: " << roads_.back().way << endl;
					//for (auto index : ways_[roads_.back().way].nodes) {
						//cout << index << ", ";
					//}
					//cout << endl;
					for (const xpath_node& child : node.parent().children()) {
						auto k = string_view{ child.node().attribute("k").as_string() };
						if (k == "name") {
							roads_.back().name = child.node().attribute("v").as_string();
						}
					}
				}
				else {
					cout << "Invalid road: " << index << endl;
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
		PrintDebugMessage(APPLICATION_NAME, "", "Creating route...", true);
		CreateRoadGraph();
		InitializePathfindingData();
		//PrintData();

		start_node_index_ = FindNearestRoadNode(start_);
		open_list_[start_node_index_] = nodes_[start_node_index_];

		end_node_index_ = FindNearestRoadNode(end_);

		cout << "Starting node: " << open_list_.begin()->first << endl;
		cout << "Ending node: " << end_node_index_ << endl;

		StartAStarSearch();
	}


	void Model::InitializePathfindingData() {
		int size = node_number_to_road_numbers.size();
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
		bool find_nearest_roads_first = (roads_.size()*3 < node_number_to_road_numbers.size());
		find_nearest_roads_first = false;
		if (find_nearest_roads_first) {
			PrintDebugMessage(APPLICATION_NAME, "Model", "Searching for nearest node using nearest roads...", false);
			for (auto road_it = roads_.begin(); road_it != roads_.end(); road_it++) {
				auto nodes = ways_[road_it->way].nodes;
				int node_indices[] = { 0, nodes.size() / 2, nodes.size() - 1 };
				for (int i = 0; i < 3; i++) {
					int index = nodes[node_indices[i]];
					distance = EuclideanDistance(node, nodes_[index]);
					if (distance < minimum_distance) {
						minimum_distance = distance;
						closest_node_index = index;
					}
				}
			}
		}
		else {
			PrintDebugMessage(APPLICATION_NAME, "Model", "Searching for nearest node using all nodes in roads...", false);
			for (auto it = node_number_to_road_numbers.begin(); it != node_number_to_road_numbers.end(); it++) {
				distance = EuclideanDistance(node, nodes_[it->first]);
				if (distance < minimum_distance) {
					minimum_distance = distance;
					closest_node_index = it->first;
				}
			}
		}
		route_.nodes.emplace_back();
		route_.nodes.back() = closest_node_index;
		return closest_node_index;
	}

	double inline Model::EuclideanDistance(Node node, Node other) {
		return sqrt(pow(node.x - other.x, 2) + pow(node.y - other.y, 2));
	}

	bool CompareNodeCost(double first, double second) {
		return first < second;
	}

	void Model::StartAStarSearch() {

		//bool(*compare)(double, double) = CompareNodeCost;
		//std::map<int, Node, bool(*)(double, double)> ol_(compare);
		//open_list_

		//for (auto it = open_list_.begin(); it != open_list_.end(); it++) {

		//}

		while (!open_list_.empty()) {

			int current = open_list_.begin()->first;
			open_list_.erase(open_list_.begin());

			closed_list_[current] = true;


			DiscoverNeighbourNodes(current);
		}
	}

	void Model::DiscoverNeighbourNodes(int current) {
		if (iterators_.empty()) {
			//cout << "node '" << current << "' is found on the following roads: " << endl;
			auto& roads = node_number_to_road_numbers[current];
			for (auto road = roads.begin(); road != roads.end(); road++) {
				int index = *road;
				int way = roads_[index].way;
				way_nodes_[index] = ways_[way].nodes;

				//cout << "road: " << (int)*road << endl;
				//cout << "nodes on road: " << endl;
				for (auto it = way_nodes_[index].begin(); it != way_nodes_[index].end(); it++) {

					if (CheckPreviousNode(it, way_nodes_[index].begin())) {
						//cout << "(previous node: " << *std::prev(it) << ") - ";
					}

					//cout << *it;
					if (current == *it) {
						iterators_[index] = it;
						break;
					}

					if (CheckNextNode(it, way_nodes_[index].end())) {
						//cout << " - (next node: " << *std::next(it) << ") ";
					}
					//cout << endl;
				}
			}

			for (auto it = iterators_.begin(); it != iterators_.end(); it++) {
				if (CheckPreviousNode(it->second, way_nodes_[it->first].begin())) {
					cout << *std::prev(it->second) << endl;
				}
				if (CheckNextNode(it->second, way_nodes_[it->first].end())) {
					cout << *std::next(it->second) << endl;
				}
			}
		}
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

	void Model::AdjustCoordinates() {
		PrintDebugMessage(APPLICATION_NAME, "Model", "Projecting node coordinates to cartesian coordinate system...", false);
		const auto pi = 3.14159265358979323846264338327950288;
		const auto deg_to_rad = 2. * pi / 360.;
		const auto earth_radius = 6378137.0;
		const auto lat2ym = [&](double lat) { return log(tan(lat * deg_to_rad / 2 + pi / 4)) / 2 * earth_radius; };
		const auto lon2xm = [&](double lon) { return lon * deg_to_rad / 2 * earth_radius; };
		cout << "delta longtitude: " << (max_lon_ - min_lon_) << endl;
		cout << "delta latitude: " << (max_lat_ - min_lat_) << endl;
		const auto dx = lon2xm(max_lon_) - lon2xm(min_lon_);
		const auto dy = lat2ym(max_lat_) - lat2ym(min_lat_);
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
		int size = node_number_to_road_numbers.size();
		delete[] node_distance_from_start_;
	}

	Model::~Model() {
		PrintDebugMessage(APPLICATION_NAME, "Model", "destroying model...", false);
		Release();
	}
}

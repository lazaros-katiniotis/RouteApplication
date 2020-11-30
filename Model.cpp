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

	Model::Element::~Element()
	{
	}

	static Model::Road::Type StringToRoadType(string_view type)
	{
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

	static Model::Landuse::Type StringToLanduseType(string_view type)
	{
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
		CreateRoadGraph();
		AdjustCoordinates();
		//PrintData();
	}

	Model::~Model() {
		PrintDebugMessage(APPLICATION_NAME, "Model", "destroying model...", false);
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
		//cout << "Printing ways..." << endl;
		//for (Way way : ways_) {
		//	for (int it : way.nodes) {
		//		cout << "nodes_[" << it << "]: (" << (double)nodes_[it].x << ", " << (double)nodes_[it].y << ")" << endl;
		//	}
		//}

		for (int index = 0; index < nodes_.size(); index++) {
			if (auto it = node_number_to_road_numbers.find(index); it != node_number_to_road_numbers.end()) {
				//print nodes that share at least 2 roads
				if (it->second.size() > 1) {
					cout << "node: " << index << endl;
					for (auto road_number = it->second.begin(); road_number != it->second.end(); ++road_number++) {
						cout << "road name: " << roads_[*road_number].name << "(" << *road_number << ")" << endl;
					}
				}
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
				if (auto road_type = StringToRoadType(type); road_type != Road::Invalid) {
					roads_.emplace_back();
					roads_.back().way = index;
					roads_.back().index = (int)roads_.size() - 1;
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

	void Model::CreateRoadGraph() {
		PrintDebugMessage(APPLICATION_NAME, "Model", "Creating map of nodes to roads...", false);
		sort(roads_.begin(), roads_.end(), [](const auto& _1st, const auto& _2nd) {
			return (int)_1st.type < (int)_2nd.type;
		});
		for (auto road = roads_.begin(); road != std::end(roads_); ++road) {
			auto way = ways_[road->way];
			for (auto node_number = way.nodes.begin(); node_number != std::end(way.nodes); ++node_number) {
				node_number_to_road_numbers[*node_number].emplace_back();
				node_number_to_road_numbers[*node_number].back() = road->index;
			}
		}
	}

	void Model::CreateRoute() {
		cout << "Total roads: " << roads_.size() << endl;
		cout << "Total nodes in roads: " << node_number_to_road_numbers.size() << endl;
		FindNearestRoadNode(start_);
		FindNearestRoadNode(end_);

	}


	void Model::FindNearestRoadNode(Node node) {
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
	}

	double inline Model::EuclideanDistance(Node node, Node other) {
		return sqrt(pow(node.x - other.x, 2) + pow(node.y - other.y, 2));
	}

	void Model::InitializePoint(Node& point, double x, double y) {
		point.x = x;
		point.y = y;
	}

	void Model::InitializePoint(Node& point, Node& other) {
		point.x = other.x;
		point.y = other.y;
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
		//const auto max_x = lon2xm(max_lon_);
		const auto min_y = lat2ym(min_lat_);
		//const auto max_y = lat2ym(max_lat_);
		//cout << "min_x, max_x: " << min_x << ", " << max_x << ", dx: " << dx << endl;
		//cout << "min_y, max_y: " << min_y << ", " << max_y << ", dy: " << dy << endl;
		metric_scale_ = std::max(dx, dy);
		//cout << "Metric scale: " << metric_scale_ << endl;

		for (auto& node : nodes_) {
			//cout << "node.x: " << node.x << " (lon), " << lon2xm(node.x) << " (meters)" << endl;
			//cout << "node.y: " << node.y << " (lat), " << lat2ym(node.y) << " (meters)" << endl << endl;
			node.x = (lon2xm(node.x) - min_x) / metric_scale_;
			node.y = (lat2ym(node.y) - min_y) / metric_scale_;
		}
	}
}

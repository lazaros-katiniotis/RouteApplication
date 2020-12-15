#include <iostream>
#include <pugixml.hpp>
#include <locale>
#include <cmath>
#include <algorithm>
#include "Model.h"
#include "Helper.h"
#include <chrono>
#include <cassert>

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
		CreateRoadGraph();
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
		if (auto it = node_number_to_road_numbers_.find(index); it != node_number_to_road_numbers_.end()) {
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

	for (const xpath_node& relation : doc_.select_nodes("/osm/relation")) {
		ParseRelations(relation.node(), index);
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

void Model::ParseRelations(const xpath_node& relation, int& index) {
	auto node = relation.node();
	std::vector<int> outer, inner;
	auto commit = [&](Multipolygon& mp) {
		mp.outer = std::move(outer);
		mp.inner = std::move(inner);
	};
	for (auto child : node.children()) {
		auto name = std::string_view{ child.name() };
		if (name == "member") {
			if (std::string_view{ child.attribute("type").as_string() } == "way") {
				if (!way_id_to_number_.count(child.attribute("ref").as_string()))
					continue;
				auto way_num = way_id_to_number_[child.attribute("ref").as_string()];
				if (std::string_view{ child.attribute("role").as_string() } == "outer")
					outer.emplace_back(way_num);
				else
					inner.emplace_back(way_num);
			}
		}
		else if (name == "tag") {
			auto category = std::string_view{ child.attribute("k").as_string() };
			auto type = std::string_view{ child.attribute("v").as_string() };
			if (category == "building") {
				commit(buildings_.emplace_back());
				break;
			}
			if (category == "natural" && type == "water") {
				commit(waters_.emplace_back());
				BuildRings(waters_.back());
				break;
			}
			if (category == "landuse") {
				if (auto landuse_type = StringToLanduseType(type); landuse_type != Landuse::Invalid) {
					commit(landuses_.emplace_back());
					landuses_.back().type = landuse_type;
					BuildRings(landuses_.back());
				}
				break;
			}
		}
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
			node_number_to_road_numbers_[*node_number].emplace_back(i);
		}
	}
}

static bool TrackRec(const std::vector<int>& open_ways,
	const Model::Way* ways,
	std::vector<bool>& used,
	std::vector<int>& nodes)
{
	if (nodes.empty()) {
		for (int i = 0; i < open_ways.size(); ++i)
			if (!used[i]) {
				used[i] = true;
				const auto& way_nodes = ways[open_ways[i]].nodes;
				nodes = way_nodes;
				if (TrackRec(open_ways, ways, used, nodes))
					return true;
				nodes.clear();
				used[i] = false;
			}
		return false;
	}
	else {
		const auto head = nodes.front();
		const auto tail = nodes.back();
		if (head == tail && nodes.size() > 1)
			return true;
		for (int i = 0; i < open_ways.size(); ++i)
			if (!used[i]) {
				const auto& way_nodes = ways[open_ways[i]].nodes;
				const auto way_head = way_nodes.front();
				const auto way_tail = way_nodes.back();
				if (way_head == tail || way_tail == tail) {
					used[i] = true;
					const auto len = nodes.size();
					if (way_head == tail)
						nodes.insert(nodes.end(), way_nodes.begin(), way_nodes.end());
					else
						nodes.insert(nodes.end(), way_nodes.rbegin(), way_nodes.rend());
					if (TrackRec(open_ways, ways, used, nodes))
						return true;
					nodes.resize(len);
					used[i] = false;
				}
			}
		return false;
	}
}

static std::vector<int> Track(std::vector<int>& open_ways, const Model::Way* ways)
{
	assert(!open_ways.empty());
	std::vector<bool> used(open_ways.size(), false);
	std::vector<int> nodes;
	if (TrackRec(open_ways, ways, used, nodes))
		for (int i = 0; i < open_ways.size(); ++i)
			if (used[i])
				open_ways[i] = -1;
	return nodes;
}

void Model::BuildRings(Multipolygon& mp)
{
	auto is_closed = [](const Model::Way& way) {
		return way.nodes.size() > 1 && way.nodes.front() == way.nodes.back();
	};

	auto process = [&](std::vector<int>& ways_nums) {
		auto ways = ways_.data();
		std::vector<int> closed, open;

		for (auto& way_num : ways_nums)
			(is_closed(ways[way_num]) ? closed : open).emplace_back(way_num);

		while (!open.empty()) {
			auto new_nodes = Track(open, ways);
			if (new_nodes.empty())
				break;
			open.erase(std::remove_if(open.begin(), open.end(), [](auto v) {return v < 0; }), open.end());
			closed.emplace_back((int)ways_.size());
			Model::Way new_way;
			new_way.nodes = std::move(new_nodes);
			ways_.emplace_back(new_way);
		}
		std::swap(ways_nums, closed);
	};

	process(mp.outer);
	process(mp.inner);
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

	if (data->use_aspect_ratio) {
		aspect_ratio_ = dx / dy;
	}
	else {
		aspect_ratio_ = 1;
	}

	if (aspect_ratio_ > 1) {
		data->start.y /= GetAspectRatio();
		data->end.y /= GetAspectRatio();
	}
	else if (aspect_ratio_ < 1) {
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

void Model::InitializePoint(Model::Node& point, Model::Node& other) {
	point.x = other.x;
	point.y = other.y;
}

void Model::Release() {

}

Model::~Model() {
	Release();
}


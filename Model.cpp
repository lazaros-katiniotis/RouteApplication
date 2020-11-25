#include <iostream>
#include <pugixml.hpp>
#include <locale>
#include <cmath>
#include "Model.h"

#include <fstream>
#include <iomanip>

using namespace pugi;

namespace route_app {

	Model::Element::~Element()
	{
	}

	Model::Model(AppData* data) {
		PrintDebugMessage(APPLICATION_NAME, "Model", "Initiating model...", true);
		OpenDocument(data);
		ParseData(data);
		AdjustCoordinates();
		PrintData();
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
		cout << "Printing ways..." << endl;
		for (Way way : ways_) {
			for (int it : way.nodes) {
				cout << "nodes_[" << it << "]: (" << (double)nodes_[it].x << ", " << (double)nodes_[it].y << ")" << endl;
			}
		}
	}

	void Model::OpenDocument(AppData* data) {
		xml_parse_result result;
		errno_t err = NULL;

		switch (data->sf) {
		case StorageFlags::FILE_STORAGE:
			CloseFile(data->query_file);
			result = doc_.load_file(data->query_file->filename);
			break;
		case StorageFlags::STRUCT_STORAGE:
			string memory = data->query_data->memory;
			cout << "User-preferred locale setting is " << std::locale("").name().c_str() << '\n';
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
		for (const xpath_node& node : doc_.select_nodes("/osm/node")) {
			new_node = ParseNode(node.node(), index);
		}

		Element* new_way;
		for (const xpath_node& way : doc_.select_nodes("/osm/way")) {
			new_way = ParseNode(way.node(), index);

			for (const xpath_node& child : way.node().children()) {
				ParseAttributes(child.node(), new_way, index);
			}
		}

		PrintData();
	}

	void Model::ParseBounds() {
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
		//cout << name << ":" << value << endl;

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
			cout << "ref: " << ref << endl;
			if (auto it = node_id_to_number_.find(ref); it != end(node_id_to_number_)) {
				Way* way = dynamic_cast<Way*>(element);
				way->nodes.emplace_back(it->second);
			}
		}
		else if (name == "tag") {
			auto category = string_view{ node.attribute("k").as_string() };
			auto type = string_view{ node.attribute("v").as_string() };

			if (category == "building") {
				cout << "category: " << category << endl;
				cout << "type: " << type << endl;
				buildings_.emplace_back();
				buildings_.back().outer = { index };
			}
		}
	}

	void Model::AdjustCoordinates() {
		const auto pi = 3.14159265358979323846264338327950288;
		const auto deg_to_rad = 2. * pi / 360.;
		const auto earth_radius = 6378137.0;
		const auto lat2ym = [&](double lat) { return log(tan(lat * deg_to_rad / 2 + pi / 4)) / 2 * earth_radius; };
		const auto lon2xm = [&](double lon) { return lon * deg_to_rad / 2 * earth_radius; };
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
			cout << "node.x: " << node.x << " (lon), " << lon2xm(node.x) << " (meters)" << endl;
			cout << "node.y: " << node.y << " (lat), " << lat2ym(node.y) << " (meters)" << endl << endl;
			node.x = (lon2xm(node.x) - min_x) / metric_scale_;
			node.y = (lat2ym(node.y) - min_y) / metric_scale_;
		}
	}
}

#include <iostream>
#include <pugixml.hpp>
#include <locale>

#include "Model.h"

#include <fstream>
#include <iomanip>

using namespace pugi;
using namespace route_app;

Model::Model(AppData *data) {
    PrintDebugMessage(APPLICATION_NAME, "Model", "Initiating model...", true);
	OpenDocument(data);
	ParseData(data);
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

xml_parse_result Model::OpenDocument(AppData *data) {
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
	return result;
}

enum NodeName {
	node, way,
};

void Model::ParseData(AppData *data) {
    PrintDebugMessage(APPLICATION_NAME, "Model", "Parsing data...", false);
	xml_parse_result result = OpenDocument(data);

	if (!result) {
		throw std::logic_error("failed to parse the xml file");
	}

	for( const xpath_node &nd: doc_.select_nodes("/osm/node") ) {
		auto node = nd.node();
		ParseNode(node, true);
	}

	for( const xpath_node &way: doc_.select_nodes("/osm/way") ) {
		auto node = way.node();
		ParseNode(node, true);

		for( const xpath_node &child: node.children() ) {
			ParseNode(child.node(), true);
		}
	}

	// for (const xpath_node& node : doc_.child("osm").children()) {
	// 	string_view name {node.node().name()};
	// 	cout << name << endl;

	// 	//string to enum type

	// 	//parse node data depending on enum type
	// 	// for (const xml_attribute& attribute : node.node().attributes()) {
	// 	// 	cout << "\t" << attribute.name() << ": " << attribute.value() << endl;
	// 	// }

	// }

}

const xml_node& Model::ParseNode(const xml_node &node, bool shouldParseAttributes) {
	auto name = string_view {node.name()};
	auto value = string_view {node.value()};
	cout << "\t" << name << ":" << value << endl;
	if (shouldParseAttributes) {
		// for( const xml_attribute &atribute: node.attributes() ) {
		// }
		ParseAttributes(node, name);

	}
	return node;
}

void Model::ParseAttributes(const xml_node &node, string_view name) {
	if( name == "nd" ) {
		auto ref = string_view{node.attribute("ref").as_string()};
		cout << "ref: " << ref << endl;
	}
	else if( name == "tag" ) {
		auto category = string_view{node.attribute("k").as_string()};
		cout << "category: " << category << endl;
		auto address = string_view{node.attribute("addr").as_string()};
		cout << "address: " << address << endl;
		auto type = string_view{node.attribute("v").as_string()};
		cout << "type: " << type << endl;
	}
}

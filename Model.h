#include <iostream>
#include <pugixml.hpp>
#include <vector>

#include "Helper.h"

using namespace std;
using namespace pugi;

namespace route_app {
    class Model {
    public:
        struct Node {
            double x = 0.f;
            double y = 0.f;
        };
        
        struct Way {
            vector<int> nodes;
        };
        
        struct Road {
            enum Type { Invalid, Unclassified, Service, Residential, Tertiary, Secondary, Primary, Trunk, Motorway, Footway };
            int way;
            Type type;
        };
        
        struct Railway {
            int way;
        };    
        
        struct Multipolygon {
            vector<int> outer;
            vector<int> inner;
        };
        
        struct Building : Multipolygon {};
        
        struct Leisure : Multipolygon {};
        
        struct Water : Multipolygon {};
        
        struct Landuse : Multipolygon {
            enum Type { Invalid, Commercial, Construction, Grass, Forest, Industrial, Railway, Residential };
            Type type;
        };

        Model(AppData *data);
        ~Model();
        void ParseData(AppData* data);
        xml_parse_result OpenDocument(AppData* data);
        void PrintDoc(const char* message, xml_document* doc, xml_parse_result* result);
        auto &GetBuildings() { return buildings_; }
    private:
        xml_document doc_;
        vector<Building> buildings_;
        const xml_node& ParseNode(const xml_node& node, bool shouldParseAttributes);
        void ParseAttributes(const xml_node& node, string_view name);

    };
}

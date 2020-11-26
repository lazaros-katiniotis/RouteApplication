#pragma once
#ifndef ROUTE_APP_MODEL_H
#define ROUTE_APP_MODEL_H
#include <iostream>
#include <pugixml.hpp>
#include <vector>
#include <unordered_map>

#include "Helper.h"

using namespace std;
using namespace pugi;
using namespace route_app;
namespace route_app {
    class Model {
    public:
        struct Element {
            virtual ~Element();
        };

        struct Node : Element {
            double x = 0.f;
            double y = 0.f;
            ~Node() {
                x = 0.f;
                y = 0.f;
            }
        };

        struct Way : Element {
            vector<int> nodes;
            ~Way() {
                nodes.clear();
            }
        };

        struct Road {
            enum Type { Invalid, Unclassified, Service, Residential, Tertiary, Secondary, Primary, Trunk, Motorway, Footway, Cycleway };
            int way;
            Type type;
            string_view name;
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

        Model(AppData* data);
        ~Model();
        double GetMetricScale() { return metric_scale_; }
        auto& GetBuildings() { return buildings_; }
        auto& GetRoads() { return roads_; }
        auto& GetRailways() { return railways_; }
        auto& GetLanduses() { return landuses_; }
        auto& GetLeisures() { return leisures_; }
        auto& GetWaters() { return waters_; }
        const vector<Node>& GetNodes() const noexcept { return nodes_; }
        const vector<Way>& GetWays() const noexcept { return ways_; }
    private:
        xml_document doc_;
        double min_lat_ = 0.;
        double max_lat_ = 0.;
        double min_lon_ = 0.;
        double max_lon_ = 0.;
        double metric_scale_ = 1.f;
        unordered_map<string, int> node_id_to_number_;
        unordered_map<string, int> way_id_to_number_;
        unordered_map<string, int> node_id_to_road_number;
        vector<Building> buildings_;
        vector<Railway> railways_;
        vector<Landuse> landuses_;
        vector<Leisure> leisures_;
        vector<Water> waters_;
        vector<Road> roads_;
        vector<Node> nodes_;
        vector<Way> ways_;
        void OpenDocument(AppData* data);
        void ParseData(AppData* data);
        void CreateRoadGraph();
        void AdjustCoordinates();
        void PrintDoc(const char* message, xml_document* doc, xml_parse_result* result);
        void PrintData();
        void ParseBounds();
        Element* ParseNode(const xml_node& node, int& index);
        void ParseAttributes(const xml_node& node, Element* element, int index);
    };
}


#endif
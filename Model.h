#pragma once
#ifndef ROUTE_APP_MODEL_H
#define ROUTE_APP_MODEL_H

#include <pugixml.hpp>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace pugi;

namespace route_app {
    struct AppData;

    class Model {
    public:
        struct Node {
            int parent = -1;
            double x = 0.0f;
            double y = 0.0f;
            double f = 0.0f;
            double h = 0.0f;
            bool operator < (const Node& other) const { return f < other.f; }
        };

        struct Way {
            vector<int> nodes;
        };

        struct Road {
            enum Type { Invalid, Unclassified, Service, Residential, Tertiary, Secondary, Primary, Trunk, Motorway, Footway, Cycleway };
            int way;
            Type type;
            string name;
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
        double GetAspectRatio();
        auto& GetBuildings() { return buildings_; }
        auto& GetRoads() { return roads_; }
        auto& GetRailways() { return railways_; }
        auto& GetLanduses() { return landuses_; }
        auto& GetLeisures() { return leisures_; }
        auto& GetWaters() { return waters_; }
        auto& GetNodes() const { return nodes_; }
        auto& GetWays() const { return ways_; }
        auto GetNodeNumberToRoadNumber() const { return node_number_to_road_numbers_; }
        bool WasModelCreated() const;
        Way& GetRoute() { return route_; }
        void InitializePoint(Node& point, Node& other);
        Model::Node& GetStartingPoint() { return start_; }
        Model::Node& GetEndingPoint() { return end_; }
    private:
        xml_document doc_;
        bool model_created_;
        double min_lat_ = 0.;
        double max_lat_ = 0.;
        double min_lon_ = 0.;
        double max_lon_ = 0.;
        double metric_scale_ = 1.f;
        double aspect_ratio_;
        unordered_map<string, int> node_id_to_number_;
        unordered_map<string, int> way_id_to_number_;
        unordered_map<int, vector<int>> node_number_to_road_numbers_;
        vector<Building> buildings_;
        vector<Railway> railways_;
        vector<Landuse> landuses_;
        vector<Leisure> leisures_;
        vector<Water> waters_;
        vector<Road> roads_;
        vector<Node> nodes_;
        vector<Way> ways_;
        Way route_;
        Node start_;
        Node end_;

        xml_parse_result OpenDocument(AppData* data);
        void ParseData(AppData* data);
        void ParseBounds();
        void ParseNode(const xml_node& node, int& index);
        void ParseAttributes(const xml_node& node, int index);
        void ParseRelations(const xpath_node& relation, int& index);
        void PrintData();
        void CreateRoadGraph();
        void AdjustCoordinates(AppData* data);
        void BuildRings(Multipolygon& mp);
        void Release();
    };

    bool inline Model::WasModelCreated() const { return model_created_; }

    double inline Model::GetAspectRatio() { return aspect_ratio_; }
}

#endif
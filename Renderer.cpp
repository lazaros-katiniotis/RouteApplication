#include <iostream>
#include <io2d.h>

#include "Model.h"
#include "Renderer.h"

using namespace std;
using namespace std::experimental::io2d;

static float RoadMetricWidth(Model::Road::Type type);
static rgba_color RoadColor(Model::Road::Type type);
static dashes RoadDashes(Model::Road::Type type);
static point_2d ToPoint2D(const Model::Node& node) noexcept;

namespace route_app {
    Renderer::Renderer(Model *model) {
        model_ = model;
        BuildRoadReps();
        BuildLanduseBrushes();
    }

    void Renderer::Initialize(output_surface& surface) {
        scale_ = static_cast<float>(std::max(surface.dimensions().x(), surface.dimensions().y()));
        cout << "scale_: " << scale_ << endl;
        pixels_in_meters_ = static_cast<float>(scale_ / model_->GetMetricScale());
        cout << "pixels_in_meters_: " << pixels_in_meters_ << endl;
        matrix_ = matrix_2d::create_scale({ scale_, -scale_ }) * matrix_2d::create_translate({ 0.f, static_cast<float>(surface.dimensions().y()) });
    }

    void Renderer::DrawLanduses(output_surface& surface) const {
        for (auto& landuse : model_->GetLanduses())
            if (auto br = landuse_brushes_.find(landuse.type); br != landuse_brushes_.end())
                surface.fill(br->second, PathFromMP(landuse));
    }

    void Renderer::DrawLeisure(output_surface& surface) const {
        for (auto& leisure : model_->GetLeisures()) {
            auto path = PathFromMP(leisure);
            surface.fill(leisure_fill_brush_, path);
            surface.stroke(leisure_outline_brush_, path, std::nullopt, leisure_outline_stroke_props_);
        }
    }

    void Renderer::DrawRailways(output_surface& surface) const {
        auto ways = model_->GetWays().data();
        for (auto& railways : model_->GetRailways()) {
            auto way = ways[railways.way];
            auto path = PathFromWay(way);
            surface.stroke(railway_stroke_brush_, path, nullopt, stroke_props{ railway_outer_width_ * pixels_in_meters_ });
            surface.stroke(railway_dash_brush_, path, nullopt, stroke_props{ railway_inner_width_ * pixels_in_meters_ }, railway_dashes_);
        }
    }

    void Renderer::DrawWater(output_surface& surface) const {
        for (auto& water : model_->GetWaters()) {
            surface.fill(water_fill_brush_, PathFromMP(water));
        }
    }

    void Renderer::DrawBuildings(output_surface& surface) const
    {
        for (auto& building : model_->GetBuildings()) {
            //cout << "building: " << &building << endl;
            auto path = PathFromMP(building);
            //cout << "path: " << &path << endl;
            surface.fill(building_fill_brush_, path);
            surface.stroke(building_outline_brush_, path, std::nullopt, building_outline_stroke_props_);
        }
    }

    void Renderer::DrawHighways(output_surface& surface) const
    {
        auto ways = model_->GetWays().data();
        for (auto road : model_->GetRoads()) {
            if (auto rep_it = road_reps_.find(road.type); rep_it != road_reps_.end()) {
                auto& rep = rep_it->second;
                auto& way = ways[road.way];
                auto width = rep.metric_width > 0.f ? (rep.metric_width * pixels_in_meters_) : 1.f;
                auto sp = stroke_props{ width, line_cap::round };
                auto path = PathFromWay(way);
                if (road.type != Model::Road::Cycleway && road.type != Model::Road::Footway) {
                    surface.stroke(railway_stroke_brush_, path, nullopt, road_outline_stroke_props_);
                }
                //surface.stroke(rep.brush, path, nullopt, sp, rep.dashes);
            }
        }
        for (auto road : model_->GetRoads()) {
            if (auto rep_it = road_reps_.find(road.type); rep_it != road_reps_.end()) {
                auto& rep = rep_it->second;
                auto& way = ways[road.way];
                bool found = false;
                for (auto index : way.nodes) {
                    //cout << index << ", ";
                    if (index == 178 || index == 163) {
                        found = true;
                    }
                }
                if (!found) continue;
                auto width = rep.metric_width > 0.f ? (rep.metric_width * pixels_in_meters_) : 1.f;
                auto sp = stroke_props{ width, line_cap::round };
                auto path = PathFromWay(way);
                surface.stroke(rep.brush, path, nullopt, sp, rep.dashes);
            }
        }
    }

    void Renderer::DrawRoute(output_surface& surface) const {
        auto& way = model_->GetRoute();
        auto path = PathFromWay(way);
        surface.stroke(route_stroke_brush_, path, nullopt, route_outline_stroke_props_);
    }

    interpreted_path Renderer::PathFromWay(const Model::Way& way) const {
        if (way.nodes.empty()) {
            return {};
        }

        const auto nodes = model_->GetNodes().data();

        auto pb = path_builder{};
        pb.matrix(matrix_);
        pb.new_figure(ToPoint2D(nodes[way.nodes.front()]));
        for (auto it = ++way.nodes.begin(); it != end(way.nodes); ++it) {
            pb.line(ToPoint2D(nodes[*it]));
        }
        return interpreted_path{ pb };
    }

    interpreted_path Renderer::PathFromMP(const Model::Multipolygon& mp) const {
        const auto nodes = model_->GetNodes().data();
        const auto ways = model_->GetWays().data();

        auto pb = path_builder{};
        pb.matrix(matrix_);

        auto create_path = [&](const Model::Way& way) {
            if (way.nodes.empty()) {
                return;
            }
            pb.new_figure(ToPoint2D(nodes[way.nodes.front()]));
            for (auto it = ++way.nodes.begin(); it != end(way.nodes); ++it) {
                pb.line(ToPoint2D(nodes[*it]));
                //cout << *it << ": " << &nodes[*it] << endl;
            }
            pb.close_figure();
        };

        for (auto way_num : mp.outer) {
            //cout << &ways[way_num] << endl;
            create_path(ways[way_num]);
        }
        for (auto way_num : mp.inner) {
            create_path(ways[way_num]);
        }

        return interpreted_path{ pb };
    }

    void Renderer::Display(output_surface& surface) {
        surface.paint(background_fill_brush_);
        DrawLanduses(surface);
        DrawLeisure(surface);
        DrawWater(surface);
        DrawRailways(surface);
        DrawHighways(surface);
        DrawBuildings(surface);

        auto pb = path_builder{};
        pb.matrix(matrix_);
        pb.new_figure(point_2d(static_cast<float>(model_->GetStartingPoint().x), static_cast<float>(model_->GetStartingPoint().y)));
        pb.line(point_2d(static_cast<float>(model_->GetEndingPoint().x), static_cast<float>(model_->GetEndingPoint().y)));

        auto path = interpreted_path{ pb };
        surface.stroke(test_brush_, path, nullopt, route_outline_stroke_props_);

        DrawRoute(surface);

    }

    void Renderer::Resize(output_surface& surface) {
        surface.dimensions(surface.display_dimensions());
    }

    void Renderer::Release() {

    }

    Renderer::~Renderer() {
        Release();
    }

    void Renderer::BuildRoadReps()
    {
        using R = Model::Road;
        auto types = { R::Motorway, R::Trunk, R::Primary,  R::Secondary, R::Tertiary,
            R::Residential, R::Service, R::Unclassified, R::Footway, R::Cycleway};
        for (auto type : types) {
            auto& rep = road_reps_[type];
            rep.brush = brush{ RoadColor(type) };
            rep.metric_width = RoadMetricWidth(type);
            rep.dashes = RoadDashes(type);
        }
    }
}

void Renderer::BuildLanduseBrushes()
{
    landuse_brushes_.insert_or_assign(Model::Landuse::Commercial, brush{ rgba_color{233, 195, 196} });
    landuse_brushes_.insert_or_assign(Model::Landuse::Construction, brush{ rgba_color{187, 188, 165} });
    landuse_brushes_.insert_or_assign(Model::Landuse::Grass, brush{ rgba_color{197, 236, 148} });
    landuse_brushes_.insert_or_assign(Model::Landuse::Forest, brush{ rgba_color{158, 201, 141} });
    landuse_brushes_.insert_or_assign(Model::Landuse::Industrial, brush{ rgba_color{223, 197, 220} });
    landuse_brushes_.insert_or_assign(Model::Landuse::Railway, brush{ rgba_color{223, 197, 220} });
    landuse_brushes_.insert_or_assign(Model::Landuse::Residential, brush{ rgba_color{209, 209, 209} });
}

static float RoadMetricWidth(Model::Road::Type type)
{
    switch (type) {
    case Model::Road::Motorway:     return 6.f;
    case Model::Road::Trunk:        return 6.f;
    case Model::Road::Primary:      return 5.f;
    case Model::Road::Secondary:    return 5.f;
    case Model::Road::Tertiary:     return 4.f;
    case Model::Road::Residential:  return 2.5f;
    case Model::Road::Unclassified: return 2.5f;
    case Model::Road::Service:      return 1.f;
    case Model::Road::Footway:      return 0.5f;
    case Model::Road::Cycleway:     return 0.75f;
    default:                        return 1.f;
    }
}

static rgba_color RoadColor(Model::Road::Type type) {
    switch (type) {
    case Model::Road::Motorway:     return rgba_color{ 226, 122, 143 };
    case Model::Road::Trunk:        return rgba_color{ 245, 161, 136 };
    case Model::Road::Primary:      return rgba_color{ 249, 207, 144 };
    case Model::Road::Secondary:    return rgba_color{ 244, 251, 173 };
    case Model::Road::Tertiary:     return rgba_color{ 244, 251, 173 };
    case Model::Road::Residential:  return rgba_color{ 254, 254, 254 };
    case Model::Road::Service:      return rgba_color{ 254, 254, 254 };
    case Model::Road::Footway:      return rgba_color{ 241, 106, 96 };
    case Model::Road::Cycleway:     return rgba_color{ 96, 96, 254 };
    case Model::Road::Unclassified: return rgba_color{ 254, 254, 254 };
    default:                        return rgba_color::grey;
    }
}

static dashes RoadDashes(Model::Road::Type type) {
    switch (type) {
    case Model::Road::Footway:      return dashes{ 5.f, {1.f, 3.f} };
    //case Model::Road::Cycleway:     return dashes{ 0.f, {1.f, 2.f} };
    case Model::Road::Cycleway:     return dashes{ 0.f, {1.f, 2.f} };
    default:                        return dashes{};
    }    
}

static point_2d ToPoint2D(const Model::Node& node) noexcept {
    return point_2d(static_cast<float>(node.x), static_cast<float>(node.y));
}

#include <iostream>
#include <io2d.h>

#include "Model.h"
#include "Renderer.h"

using namespace std;
using namespace std::experimental::io2d;

//static float RoadMetricWidth(Model::Road::Type type);
//static io2d::rgba_color RoadColor(Model::Road::Type type);
//static io2d::dashes RoadDashes(Model::Road::Type type);
static point_2d ToPoint2D(const Model::Node& node) noexcept;
namespace route_app {
    Renderer::Renderer(Model *model) {
        model_ = model;
        Initialize();
    }

    void Renderer::Initialize() {

    }

    //void Renderer::InitializeWindow(int width, int height) {
    //    auto display = output_surface {width, height, format::argb32, scaling::none, refresh_style::fixed, 30.f};
    //    display.size_change_callback([&](output_surface &surface) {
    //        surface.dimensions(surface.display_dimensions());
    //    });
    //    display.draw_callback([&](auto &surface) {
    //        brush mainColor {rgba_color::green};
    //        surface.paint(mainColor);
    //    });
    //    display.begin_show();
    //}

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
        scale_ = static_cast<float>(std::max(surface.dimensions().x(), surface.dimensions().y()));
        //cout << "scale_: " << scale_ << endl;
        pixels_in_meters_ = static_cast<float>(scale_ / model_->GetMetricScale());
        //cout << "pixels_in_meters_: " << pixels_in_meters_ << endl;
        matrix_ = matrix_2d::create_scale({ scale_, -scale_ }) *
            matrix_2d::create_translate({ 0.f, static_cast<float>(surface.dimensions().y()) });
        //surface.paint(background_fill_brush_);
        surface.paint(mainColor);
        DrawBuildings(surface);
    }

    void Renderer::Release() {

    }

    Renderer::~Renderer() {
        Release();
    }
}

static point_2d ToPoint2D(const Model::Node& node) noexcept {
    return point_2d(static_cast<float>(node.x), static_cast<float>(node.y));
}


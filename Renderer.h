#pragma once
#ifndef ROUTE_APP_RENDERER_H
#define ROUTE_APP_RENDERER_H
#include <iostream>
#include "Model.h"
#include <io2d.h>

using namespace std::experimental::io2d;

namespace route_app {

    class Renderer {
        private:

            struct RoadRep {
                brush brush{ rgba_color::black };
                dashes dashes{};
                float metric_width = 1.f;
            };

            void Initialize();
            void Release();
            brush mainColor { rgba_color::green };
            Model *model_;
            float scale_ = 1.f;
            float pixels_in_meters_ = 1.f;
            matrix_2d matrix_;
            brush building_fill_brush_                  { rgba_color{208, 197, 190} };
            brush background_fill_brush_                { rgba_color{238, 235, 227} };
            brush building_outline_brush_               { rgba_color{181, 167, 154} };
            brush leisure_fill_brush_                   { rgba_color{189, 252, 193} };
            brush leisure_outline_brush_                { rgba_color{160, 248, 162} };
            stroke_props leisure_outline_stroke_props_  { 1.f };
            brush water_fill_brush_                     { rgba_color{155, 201, 215} };
            brush railway_stroke_brush_                 { rgba_color{93,93,93} };
            brush railway_dash_brush_                   { rgba_color::white };
            dashes railway_dashes_                      { 0.f, {3.f, 3.f} };
            float railway_outer_width_ = 3.f;
            float railway_inner_width_ = 2.f;
            stroke_props building_outline_stroke_props_{ 1.f };
            unordered_map<Model::Road::Type, RoadRep> road_reps_;
            unordered_map<Model::Landuse::Type, brush> landuse_brushes_;

        public:
            void Display(output_surface& surface);
            void Resize(output_surface& surface);
            void DrawBuildings(output_surface& surface) const;
            void DrawHighways(output_surface& surface) const;
            void DrawLanduses(output_surface& surface) const;
            void DrawLeisure(output_surface& surface) const;
            void DrawRailways(output_surface& surface) const;
            void DrawWater(output_surface& surface) const;
            interpreted_path PathFromMP(const Model::Multipolygon& mp) const;
            interpreted_path PathFromWay(const Model::Way& way) const;
            void BuildRoadReps();
            void BuildLanduseBrushes();
            Renderer(Model *model);
            ~Renderer();
    };
}

#endif
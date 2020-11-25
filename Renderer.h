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
            void Initialize();
            void Release();
            brush mainColor { rgba_color::green };
            Model *model_;
            float scale_ = 1.f;
            float pixels_in_meters_ = 1.f;
            matrix_2d matrix_;
            brush building_fill_brush_ { rgba_color{208, 197, 190} };
            brush background_fill_brush_ { rgba_color{238, 235, 227} };
            brush building_outline_brush_{ rgba_color{181, 167, 154} };
            stroke_props building_outline_stroke_props_{ 1.f };

        public:
            void Display(output_surface& surface);
            void DrawBuildings(output_surface& surface) const;
            interpreted_path PathFromMP(const Model::Multipolygon& mp) const;
            Renderer(Model *model);
            ~Renderer();
    };
}

#endif
#include <iostream>
#include <io2d.h>

#include "Renderer.h"

using namespace std;
using namespace std::experimental::io2d;

namespace route_app {

    Renderer::Renderer() {
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

    void Renderer::Display(output_surface& surface) {
        surface.paint(mainColor);
    }

    void Renderer::Release() {

    }

    Renderer::~Renderer() {
        Release();
    }
}


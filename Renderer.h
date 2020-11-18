#pragma once
#include <iostream>
#include <io2d.h>

using namespace std::experimental::io2d;

namespace route_app {
    class Renderer {
        private:
            void Initialize();
            void Release();
            brush mainColor { rgba_color::green };

        public:
            void Display(output_surface& surface);
            Renderer();
            ~Renderer();
    };
}
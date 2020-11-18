#include <iostream>
#include <fstream>
#include <cstdlib>
#include <fcntl.h>
#include <io.h>
#include <io2d.h>

#include "Helper.h"
#include "HTTPHandler.h"
#include "Renderer.h"
#include "Model.h"

using namespace std;
namespace io2d = std::experimental::io2d;

namespace route_app {
    
    class RouteApplication {
    private:
        AppData *data_;
        HTTPHandler *handler_;
        Model *model_;
        Renderer *renderer_;
        void InitializeAppData(StorageFlags sf);
        void InitializeHTTPRequestQuery(string url, string api, string arguments);
        void Release();
    public:
        RouteApplication(StorageFlags sf, string url, string api, string arguments);
        ~RouteApplication();
        static string GetApplicationName();
        void HTTPRequest();
        void ModelData();
        void FindRoute();
        void Render();
        void DisplayMap();
    };

    RouteApplication::RouteApplication(StorageFlags sf, string url, string api, string arguments) {
        PrintDebugMessage(APPLICATION_NAME, "", "Initiating Route Application...", true);
        InitializeAppData(sf);
        InitializeHTTPRequestQuery(url, api, arguments);
    }

    void RouteApplication::InitializeAppData(StorageFlags sf) {
        data_ = new AppData();
        data_->sf = sf;
        errno_t error;
        switch (data_->sf) {
            case StorageFlags::FILE_STORAGE:
                data_->query_file = new QueryFile();
                data_->query_file->filename = "data.xml";
                _set_errno(0);
                error = fopen_s(&data_->query_file->file, data_->query_file->filename, "wb");
                if (error != 0) {
                    string filename = (data_->query_file->filename);
                    string message = "Error opening file '" + filename + "'.";
                    PrintDebugMessage(APPLICATION_NAME, "", message, false);
                    exit(EXIT_FAILURE);
                }
                break;
            case StorageFlags::STRUCT_STORAGE:
                data_->query_data = new QueryData();
                data_->query_data->memory = new char(1);
                data_->query_data->size = 0;
                data_->query_data->callback_count = 0;
            break;
        }
    }

    void RouteApplication::InitializeHTTPRequestQuery(string url, string api, string arguments) {
        PrintDebugMessage(APPLICATION_NAME, "", "Initializing HTTP request query...", false);
        handler_ = new HTTPHandler(url, api, arguments);
    }

    void RouteApplication::HTTPRequest() {
        handler_->Request(data_);
    }

    void RouteApplication::ModelData() {
        PrintDebugMessage(APPLICATION_NAME, "", "Creating model...", true);
        model_ = new Model(data_);
    }

    void RouteApplication::FindRoute() {

    }

    void RouteApplication::Render() {
        PrintDebugMessage(APPLICATION_NAME, "", "Initializing renderer...", true);
        renderer_ = new Renderer();
        DisplayMap();
    }

    string RouteApplication::GetApplicationName() {
        return APPLICATION_NAME;
    }

    RouteApplication::~RouteApplication() {
        Release();
    }

    void RouteApplication::DisplayMap() {
        auto display = io2d::output_surface{ 400, 400, io2d::format::argb32, io2d::scaling::none, io2d::refresh_style::fixed, 30 };
        display.size_change_callback([](io2d::output_surface& surface) {
            surface.dimensions(surface.display_dimensions());
        });
        display.draw_callback([&](io2d::output_surface& surface) {
            renderer_->Display(surface);
        });
        display.begin_show();
    }

    void RouteApplication::Release() {
        if (data_->sf == StorageFlags::STRUCT_STORAGE) {
            delete data_->query_data->memory;
            delete data_->query_data;
        }
        delete handler_;
        delete model_;
        delete renderer_;
    }
}

int main(int argc, char** argv) {
    string url = "https://api.openstreetmap.org";
    string api = "/api/0.6";
    string bounding_box_query_prefix = "/map?bbox=";

    //input from command line arguments
    route_app::StorageFlags sf = route_app::StorageFlags::FILE_STORAGE;
    string minLong = "20.75456";
    string minLat = "38.95622";
    string maxLong = "20.75478";
    string maxLat = "38.95636";

    //4to lykeio
    //minLong = "20.73755";
    //minLat = "38.95697";
    //maxLong = "20.74000";
    //maxLat = "38.95866";
    string bounding_box_arguments = minLong + "," + minLat + "," + maxLong + "," + maxLat;
    string bounding_box_query = bounding_box_query_prefix + bounding_box_arguments;

    route_app::RouteApplication *routeApp = new route_app::RouteApplication(sf, url, api, bounding_box_query);
    routeApp->HTTPRequest();
    routeApp->ModelData();
    //routeApp->FindRoute();
    routeApp->Render();
    delete routeApp;
}
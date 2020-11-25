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
        void ParseCommandLineArguments(int argc, char** argv);
        void InitializeAppData(StorageMethod sm, string filename, const char* file_mode);
        void InitializeHTTPRequestQuery(string url, string api, string arguments);
        void Release();
    public:
        RouteApplication(int argc, char** argv);
        ~RouteApplication();
        static string GetApplicationName();
        void HTTPRequest();
        void ModelData();
        void FindRoute();
        void Render();
        void DisplayMap();
    };

    RouteApplication::RouteApplication(int argc, char** argv) {
        PrintDebugMessage(APPLICATION_NAME, "", "Initiating Route Application...", false);
        cout << std::setprecision(7);
        cout << std::fixed;
        ParseCommandLineArguments(argc, argv);
    }

    void RouteApplication::ParseCommandLineArguments(int argc, char** argv) {
        PrintDebugMessage(APPLICATION_NAME, "", "Parsing arguments from command line...", false);
        //RouteApplication.exe -b 20.75456,38.95622,20.75478,38.95636 -f test.xml
        string url = "https://api.openstreetmap.org";
        string api = "/api/0.6";
        string osm_bounding_box_query_prefix = "/map?bbox=";
        route_app::StorageMethod sm = route_app::StorageMethod::MEMORY_STORAGE;
        string osm_bounding_box_query = "";
        string osm_data_file;
        const char* file_mode = "w";
        bool successfully_parsed_arguments = true;
        if (argc > 1) {
            for (int i = 1; i < argc; ++i) {
                if (string_view{ argv[i] } == "-b") {
                    int coordinate_count = 0;
                    while (++i < argc) {
                        osm_bounding_box_query += argv[i];
                        if (++coordinate_count != 4) {
                            osm_bounding_box_query += ",";
                        }
                        else {
                            break;
                        }
                    }
                    if (coordinate_count != 4) {
                        successfully_parsed_arguments = false;
                        break;
                    }
                    PrintDebugMessage(APPLICATION_NAME, "", "OSM Bounding box found: " + osm_bounding_box_query, false);
                }
                else if (string_view{ argv[i] } == "-f") {
                    sm = StorageMethod::FILE_STORAGE;
                    if (++i < argc) {
                        osm_data_file = argv[i];
                        if (osm_bounding_box_query == "") {
                            file_mode = "r";
                        }
                        PrintDebugMessage(APPLICATION_NAME, "", "OSM data file found: '" + osm_data_file + "'", false);
                    }
                    else {
                        successfully_parsed_arguments = false;
                        break;
                    }
                }
            }
        }
        if (!successfully_parsed_arguments) {
            cout << "Usage: maps [-b MinLongitude MinLattitude MaxLongitude MaxLattitude] [-f filename.xml]" << std::endl;
            cout << "Will use the map of Rapperswil: 8.81598,47.22277,8.83,47.23" << std::endl << std::endl;
            osm_bounding_box_query = "8.81598,47.22277,8.83,47.23";
        }

        InitializeAppData(sm, osm_data_file, file_mode);
        InitializeHTTPRequestQuery(url, api, osm_bounding_box_query_prefix + osm_bounding_box_query);
    }

    void RouteApplication::InitializeAppData(StorageMethod sm, string filename, const char* file_mode) {
        data_ = new AppData();
        data_->sm = sm;
        errno_t error;
        switch (data_->sm) {
            case StorageMethod::FILE_STORAGE:
                data_->query_file = new QueryFile();
                data_->query_file->filename = filename;
                _set_errno(0);
                error = fopen_s(&data_->query_file->file, data_->query_file->filename.c_str(), file_mode);
                if (error != 0) {
                    string filename = (data_->query_file->filename);
                    string message = "Error opening file '" + filename + "'.";
                    PrintDebugMessage(APPLICATION_NAME, "", message, false);
                    exit(EXIT_FAILURE);
                }
                break;
            case StorageMethod::MEMORY_STORAGE:
                data_->query_data = new QueryData();
                data_->query_data->memory = new char(1);
                data_->query_data->size = 0;
                data_->query_data->callback_count = 0;
            break;
        }
    }

    void RouteApplication::InitializeHTTPRequestQuery(string url, string api, string arguments) {
        PrintDebugMessage(APPLICATION_NAME, "", "Initializing HTTP request query...", true);
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
        renderer_ = new Renderer(model_);
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
        if (data_->sm == StorageMethod::MEMORY_STORAGE) {
            delete data_->query_data->memory;
            delete data_->query_data;
        }
        delete handler_;
        delete model_;
        delete renderer_;
    }
}

int main(int argc, char** argv) {
    RouteApplication *routeApp = new RouteApplication(argc, argv);
    routeApp->HTTPRequest();
    routeApp->ModelData();
    //routeApp->FindRoute();
    routeApp->Render();
    delete routeApp;
}
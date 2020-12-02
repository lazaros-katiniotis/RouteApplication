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
        bool CreateQueryFromBoundingBox(int argc, char** argv, int& index, string* bounding_box_query);
        bool CreateQueryFromPoint(int argc, char** argv, int& index, string* bounding_box_query);
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
        const double BOUNDING_BOX_INTERVAL = 0.00333333f;
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
        //preveza: 20.7500 38.9571 20.7594 38.9628
        //fruengen: 17.95956 59.28393 17.96599  59.28651
        //area near fruengen: 17.9519 59.2908 17.9594 59.2943
        string url = "https://api.openstreetmap.org";
        string api = "/api/0.6";
        string osm_bounding_box_query_prefix = "/map?bbox=";
        route_app::StorageMethod sm = route_app::StorageMethod::MEMORY_STORAGE;
        string *osm_bounding_box_query = new string("");
        string osm_data_file;
        const char* file_mode = "w";
        bool successfully_parsed_arguments = false;
        //x - 0
        //interval 0.00333333
        //-b 20.74694 38.95525 20.75073 38.95908
        //-b 17.9519 59.2908 17.9594 59.2943

        if (argc > 1) {
            for (int i = 1; i < argc; ++i) {
                if (string_view{ argv[i] } == "-b") {
                    successfully_parsed_arguments = CreateQueryFromBoundingBox(argc, argv, i, osm_bounding_box_query);
                }
                else if (string_view{ argv[i] } == "-p") {
                    successfully_parsed_arguments = CreateQueryFromPoint(argc, argv, i, osm_bounding_box_query);
                }
                else if (string_view{ argv[i] } == "-f") {
                    sm = StorageMethod::FILE_STORAGE;
                    if (++i < argc) {
                        osm_data_file = argv[i];
                        if (*osm_bounding_box_query == "") {
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
            cout << "Usage: maps [-b MinLongitude MinLattitude MaxLongitude MaxLattitude] [-p Longtitude Latitude] [-f filename.xml]" << std::endl;
            cout << "Will use the map of Rapperswil: 8.81598,47.22277,8.83,47.23" << std::endl << std::endl;
            osm_bounding_box_query = new string("8.81598,47.22277,8.83,47.23");
        }

        InitializeAppData(sm, osm_data_file, file_mode);
        InitializeHTTPRequestQuery(url, api, osm_bounding_box_query_prefix + *osm_bounding_box_query);
        delete osm_bounding_box_query;
    }

    bool RouteApplication::CreateQueryFromBoundingBox(int argc, char** argv, int& index, string* bounding_box_query) {
        static int coordinate_count = 0;
        while (++index < argc) {
            //if (total_coords == 4) {
            //}
            //else if (total_coords == 2) {
            //    double coord = atof(argv[index]);
            //    for (int i = 0; i < 2; i++) {
            //        *bounding_box_query += (coord - BOUNDING_BOX_INTERVAL);
            //        *bounding_box_query += ",";
            //    }
            //}
            *bounding_box_query += argv[index];
            if (++coordinate_count != 4) {
                *bounding_box_query += ",";
            }
            else {
                break;
            }
        }
        if (coordinate_count != 4) {
            return false;
        }
        PrintDebugMessage(APPLICATION_NAME, "", "OSM Bounding box found: " + *bounding_box_query, false);
        return true;
    }

    bool RouteApplication::CreateQueryFromPoint(int argc, char** argv, int& index, string* bounding_box_query) {
        static int coordinate_count = 0;
        while (++index < argc) {
            //if (total_coords == 4) {
            //}
            //else if (total_coords == 2) {
            //    double coord = atof(argv[index]);
            //    for (int i = 0; i < 2; i++) {
            //        *bounding_box_query += (coord - BOUNDING_BOX_INTERVAL);
            //        *bounding_box_query += ",";
            //    }
            //}
            *bounding_box_query += argv[index];
            if (++coordinate_count != 2) {
                *bounding_box_query += ",";
            }
            else {
                break;
            }
        }
        if (coordinate_count != 2) {
            return false;
        }
        PrintDebugMessage(APPLICATION_NAME, "", "OSM Bounding box found: " + *bounding_box_query, false);
        return true;
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
        PrintDebugMessage(APPLICATION_NAME, "", "Finding route...", true);
        Model::Node start;
        Model::Node end;
        //start.x = 0.6f;
        //start.y = 0.5f;
        start.x = 0.133f;
        start.y = 0.275f;
        end.x = 0.65f;
        end.y = 0.65f;
        model_->InitializePoint(model_->GetStartingPoint(), start);
        model_->InitializePoint(model_->GetEndingPoint(), end);
        model_->CreateRoute();
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
        auto display = io2d::output_surface{ 600, 600, io2d::format::argb32, io2d::scaling::none, io2d::refresh_style::fixed, 30 };
        renderer_->Initialize(display);

        display.size_change_callback([&](io2d::output_surface& surface) {
            renderer_->Resize(surface);
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
    route_app::RouteApplication *routeApp = new route_app::RouteApplication(argc, argv);
    routeApp->HTTPRequest();
    routeApp->ModelData();
    routeApp->FindRoute();
    routeApp->Render();
    delete routeApp;
}
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <fcntl.h>
#include <io.h>
#include <io2d.h>

#include "Helper.h"
#include "HTTPHandler.h"
#include "ArgumentParser.h"
#include "Renderer.h"

using namespace std;
namespace io2d = std::experimental::io2d;

namespace route_app {
    
    class RouteApplication {
    private:
        AppData *data_ = NULL;
        HTTPHandler *handler_ = NULL;
        Model *model_ = NULL;
        Renderer *renderer_ = NULL;
        ArgumentParser* parser_ = NULL;
        string url_;
        string api_;
        string query_prefix_;
        string query_bounds_;
        bool download_osm_data_;

        void InitializeAppData();
        void Release();
        void Exit(int code);
    public:
        RouteApplication(int argc, char** argv);
        ~RouteApplication();
        static string GetApplicationName();
        bool ParseCommandLineArguments(int argc, char** argv);
        void Initialize();
        void ReleaseParser();
        void HTTPRequest();
        void ModelData();
        void FindRoute();
        void Render();
        void DisplayMap();
        const double BOUNDING_BOX_INTERVAL = 0.00166666;
    };

    RouteApplication::RouteApplication(int argc, char** argv) {
        PrintDebugMessage(APPLICATION_NAME, "", "Creating Route Application...", false);
        url_ = "https://api.openstreetmap.org";
        api_ = "/api/0.6";
        query_prefix_ = "/map?bbox=";
        query_bounds_ = "";
        download_osm_data_ = false;
        cout << std::setprecision(7);
        cout << std::fixed;
    }

    bool RouteApplication::ParseCommandLineArguments(int argc, char** argv) {
        PrintDebugMessage(APPLICATION_NAME, "", "Parsing arguments from command line...", false);

        parser_ = new ArgumentParser(argc, argv);
        return parser_->SyntaxAnalysis(argc, argv);
    }

    void RouteApplication::Initialize() {
        InitializeAppData();
        using S = ArgumentParser::SyntaxFlags;
        if ((parser_->GetSyntaxState() & (int)S::BOUNDS) == (int)S::BOUNDS) {
            query_bounds_ = parser_->GetBoundQuery();
            download_osm_data_ = true;
        }
        else if ((parser_->GetSyntaxState() & (int)S::POINT) == (int)S::POINT) {
            query_bounds_ += to_string(data_->point.x - BOUNDING_BOX_INTERVAL);
            query_bounds_ += ",";
            query_bounds_ += to_string(data_->point.y - BOUNDING_BOX_INTERVAL);
            query_bounds_ += ",";
            query_bounds_ += to_string(data_->point.x + BOUNDING_BOX_INTERVAL);
            query_bounds_ += ",";
            query_bounds_ += to_string(data_->point.y + BOUNDING_BOX_INTERVAL);
            download_osm_data_ = true;
        }
        ReleaseParser();
    }

    void RouteApplication::InitializeAppData() {
        data_ = new AppData();
        string file_mode;

        using S = ArgumentParser::SyntaxFlags;
        switch (parser_->GetSyntaxState()) {
        case (int)S::BOUNDS:
            data_->sm = StorageMethod::MEMORY_STORAGE;
            break;
        case (int)S::FILE:
            file_mode = "r";
            data_->sm = StorageMethod::FILE_STORAGE;
            break;
        case (int)S::BOUNDS | (int)S::FILE:
            file_mode = "w";
            data_->sm = StorageMethod::FILE_STORAGE;
            break;
        case (int)S::POINT:
            data_->point.x = parser_->GetPoint().x;
            data_->point.y = parser_->GetPoint().y;
            data_->sm = StorageMethod::MEMORY_STORAGE;
            break;
        case (int)S::POINT | (int)S::FILE:
            data_->point.x = parser_->GetPoint().x;
            data_->point.y = parser_->GetPoint().y;
            file_mode = "w";
            data_->sm = StorageMethod::FILE_STORAGE;
            break;
        default:
            file_mode = "w";
            data_->sm = StorageMethod::FILE_STORAGE;
            break;
        }

        data_->start.x = parser_->GetStartingPoint().x;
        data_->start.y = parser_->GetStartingPoint().y;

        data_->end.x = parser_->GetEndingPoint().x;
        data_->end.y = parser_->GetEndingPoint().y;

        errno_t error;
        switch (data_->sm) {
            case StorageMethod::FILE_STORAGE:
                data_->query_file = new QueryFile();
                data_->query_file->filename = parser_->GetFilename();
                _set_errno(0);
                error = fopen_s(&data_->query_file->file, data_->query_file->filename.c_str(), file_mode.c_str());
                if (error != 0) {
                    PrintDebugMessage(APPLICATION_NAME, "", "Error opening file '" + data_->query_file->filename + "'.", false);
                    Exit(EXIT_FAILURE);
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

    void RouteApplication::HTTPRequest() {
        if (download_osm_data_) {
            PrintDebugMessage(APPLICATION_NAME, "", "Initializing HTTP request query...", true);
            handler_ = new HTTPHandler(url_, api_, query_prefix_ + query_bounds_);
            handler_->Request(data_);
        }
    }

    void RouteApplication::ModelData() {
        PrintDebugMessage(APPLICATION_NAME, "", "Creating model...", true);
        model_ = new Model(data_);
    }

    void RouteApplication::FindRoute() {
        PrintDebugMessage(APPLICATION_NAME, "", "Finding route...", true);

        model_->InitializePoint(model_->GetStartingPoint(), data_->start);
        model_->InitializePoint(model_->GetEndingPoint(), data_->end);
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

    void RouteApplication::ReleaseParser() {
        if (parser_ != NULL) {
            delete parser_;
            parser_ = NULL;
        }
    }

    void RouteApplication::Release() {
        if (data_ != NULL) {
            if (data_->sm == StorageMethod::MEMORY_STORAGE) {
                delete data_->query_data->memory;
                data_->query_data->memory = NULL;
                delete data_->query_data;
                data_->query_data = NULL;
            }
            delete data_;
            data_ = NULL;
        }
        if (parser_ != NULL) {
            delete parser_;
            parser_ = NULL;
        }
        if (handler_ != NULL) {
            delete handler_;
            handler_ = NULL;
        }
        if (model_ != NULL) {
            delete model_;
            model_ = NULL;
        }
        if (renderer_ != NULL) {
            delete renderer_;
            renderer_ = NULL;
        }
    }

    void RouteApplication::Exit(int code) {
        Release();
        exit(code);
    }
}

int main(int argc, char** argv) {
    route_app::RouteApplication *routeApp = new route_app::RouteApplication(argc, argv);
    if (routeApp->ParseCommandLineArguments(argc, argv)) {
        routeApp->Initialize();
        routeApp->HTTPRequest();
        routeApp->ModelData();
        routeApp->FindRoute();
        routeApp->Render();
    }
    delete routeApp;
}
#pragma once
#ifndef ROUTE_APP_HELPER_H
#define ROUTE_APP_HELPER_H
#include <iostream>

#include "Model.h"

using namespace std;
namespace route_app {
    class Model;

    static const string APPLICATION_NAME = "RouteApplication";

    enum class StorageMethod {
        MEMORY_STORAGE, FILE_STORAGE
    };

    struct QueryData {
        char* memory;
        size_t size;
        int callback_count;
    };

    struct QueryFile {
        FILE* file;
        string filename;
    };

    struct AppData {
        StorageMethod sm;
        QueryData* query_data;
        QueryFile* query_file;
        Model::Node point;
        Model::Node start;
        Model::Node end;
        bool use_aspect_ratio;
    };

    static void CloseFile(QueryFile* query_file) {
        errno_t error;
        if (query_file->file == NULL) {
            return;
        }
        _set_errno(0);
        error = fclose(query_file->file);
        query_file->file = NULL;
        if (error != 0) {
            string filename = query_file->filename;
            throw std::logic_error("The file '" + filename + "' was not closed.\n");
        }
    }

    void inline PrintDebugMessage(string application_name, string system_name, string message, bool newlineAtStart) {
        string newline = "";
        string tabs = "";

        if (newlineAtStart) {
            newline = "\n";
        }

        if (system_name == "") {
            tabs = "\t\t";
        }
        else {
          system_name = "::" + system_name;
          tabs = "\t";
        }

        cout << newline + APPLICATION_NAME + system_name + tabs + message << endl;
    }
}
#endif
#include <iostream>
#include <curl/curl.h>

#include "HTTPHandler.h"

using namespace route_app;

size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t realsize = size * nmemb;
    struct QueryData* mem = (struct QueryData*)userp;

    char* ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    mem->callback_count++;

    return realsize;
}

HTTPHandler::HTTPHandler(string url, string api, string arguments) {
	query_ = url + api + arguments;
	Initialize();
}

HTTPHandler::~HTTPHandler() {
	Release();
}

void HTTPHandler::Initialize() {
	curl_global_init(CURL_GLOBAL_ALL);
	//locale loc(std::locale(), new std::codecvt_utf8<char32_t>);
    //response.imbue(loc);
	PrintDebugMessage(APPLICATION_NAME, "HTTPHandler", "HTTP Request Handler initialized.", false);
	PrintDebugMessage(APPLICATION_NAME, "HTTPHandler", "HTTP Request Query: " + query_, false);
}

CURLcode HTTPHandler::Request(AppData *data) {
    PrintDebugMessage(APPLICATION_NAME, "HTTPHandler", "Initiating HTTP Request...", false);
    CURL* curl_handle;
    CURLcode res{};
    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, &(query_[0]));
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0);
    switch (data->sm) {
    case StorageMethod::MEMORY_STORAGE:
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&(*data->query_data));
        break;
    case StorageMethod::FILE_STORAGE:
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, data->query_file->file);
        break;
    }

    res = curl_easy_perform(curl_handle);
    curl_easy_cleanup(curl_handle);
    
    return res;
}

void HTTPHandler::Release() {
    PrintDebugMessage(APPLICATION_NAME, "HTTPHandler", "releasing resources...", false);
	curl_global_cleanup();
}
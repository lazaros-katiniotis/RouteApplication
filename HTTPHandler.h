#pragma once
#ifndef ROUTE_APP_HTTP_HANDLER_H
#define ROUTE_APP_HTTP_HANDLER_H
#include <iostream>
#include <curl/curl.h>

#include "Helper.h"

using namespace std;

namespace route_app {

	class HTTPHandler {
	private:
		string query_;
		void Initialize();
		void Release();
	public:
		HTTPHandler(string url, string api, string arguments);
		~HTTPHandler();
		CURLcode Request(AppData *data);
	};
}

#endif
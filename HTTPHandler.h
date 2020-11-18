#pragma once
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
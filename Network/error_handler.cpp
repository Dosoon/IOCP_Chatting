#include "error_handler.h"

#include <iostream>

bool ErrorHandler::Handle(bool result, std::function<int(void)> get_err_func, const char* method, int32_t allow_codes, ...)
{
	if (result) {
		return true;
	}
	else {
		auto error_code = get_err_func();

		va_list code_list;
		va_start(code_list, allow_codes);

		for (auto i = 0; i < allow_codes; ++i) {
			if (error_code == va_arg(code_list, int32_t)) {
				return true;
			}
		}

		std::cout << '[' << method << "] Failed With Error Code : " << error_code << '\n';
		va_end(code_list);
		return false;
	}
}

bool ErrorHandler::Handle(int32_t socket_result, std::function<int(void)> get_err_func, const char* method, int32_t allow_codes, ...)
{
	if (socket_result != SOCKET_ERROR) {
		return true;
	}
	else {
		auto error_code = get_err_func();

		va_list code_list;
		va_start(code_list, allow_codes);

		for (auto i = 0; i < allow_codes; ++i) {
			if (error_code == va_arg(code_list, int32_t)) {
				return true;
			}
		}

		std::cout << '[' << method << "] Failed With Error Code : " << error_code << '\n';
		va_end(code_list);
		return true;
	}
}

bool ErrorHandler::Handle(HANDLE handle_result, std::function<int(void)> get_err_func, const char* method, HANDLE allow_handle)
{
	if (handle_result == allow_handle) {
		return true;
	}
	else {
		auto error_code = get_err_func();

		std::cout << '[' << method << "] Failed With Error Code : " << error_code << '\n';
		return false;
	}
}
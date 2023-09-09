#pragma once

#include <functional>
#include <Windows.h>

class ErrorHandler
{
public:
	static bool Handle(bool result, std::function<int(void)> get_err_func, const char* method, int32_t allow_codes, ...);
	static bool Handle(int32_t socket_result, std::function<int(void)> get_err_func, const char* method, int32_t allow_codes, ...);
	static bool Handle(HANDLE handle_result, std::function<int(void)> get_err_func, const char* method, HANDLE allow_handle);
};
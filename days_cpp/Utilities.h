#pragma once
#include <optional>
#include <string>
#include <chrono>

class Utilities
{
public:
	template <typename T>
	void display(const T& value);
	void newline();
	std::optional<std::chrono::year_month_day> getDateFromString(const std::string& buf);
	auto validate_date(char* argv[], auto arg_num, bool to_string);
	std::string getStringFromDate(const std::chrono::year_month_day& date);
	std::optional<std::string> getEnvironmentVariable(const std::string& name);
	void print_birthday(auto currentDate);
};


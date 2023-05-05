#include "Utilities.h"

#pragma warning(disable : 4996)

#include <iostream>	   // for standard I/O streams
#include <iomanip>	   // for stream control
#include <string>	   // for std::string class
#include <cstdlib>	   // for std::getenv
#include <chrono>	   // for the std::chrono facilities
#include <sstream>	   // for std::stringstream class
#include <vector>	   // for std::vector class
#include <optional>	   // for std::optional
#include <string_view> // for std::string_view
#include <filesystem>  // for path utilities
#include <memory>	   // for smart pointers

#include <vector>
#include <algorithm>

#include "Event.h"	  // for our Event class
#include "rapidcsv.h" // for the header-only library RapidCSV

using namespace std;


template <typename T>
void Utilities::display(const T &value)
{
	std::cout << value;
}

// Prints a newline to standard output.
void Utilities::newline()
{
	std::cout << std::endl;
}

// Parses the string `buf` for a date in YYYY-MM-DD format. If `buf` can be parsed,
// returns a wrapped `std::chrono::year_month_day` instances, otherwise `std::nullopt`.
// NOTE: Once clang++ and g++ implement chrono::from_stream, this could be replaced by something like this:
//  chrono::year_month_day birthdate;
//  std::istringstream bds{birthdateValue};
//  std::basic_istream<char> stream{bds.rdbuf()};
//  chrono::from_stream(stream, "%F", birthdate);
// However, I don't know how errors should be handled. Maybe this function could then
// continue to serve as a wrapper.
std::optional<std::chrono::year_month_day> Utilities::getDateFromString(const std::string &buf)
{
	using namespace std; // use std facilities without prefix inside this function

	constexpr string_view yyyymmdd = "YYYY-MM-DD";
	if (buf.size() != yyyymmdd.size())
	{
		return nullopt;
	}

	istringstream input(buf);
	string part;
	vector<string> parts;
	while (getline(input, part, '-'))
	{
		parts.push_back(part);
	}
	if (parts.size() != 3)
	{ // expecting three components, year-month-day
		return nullopt;
	}

	int year{0};
	unsigned int month{0};
	unsigned int day{0};
	try
	{
		year = stoul(parts.at(0));
		month = stoi(parts.at(1));
		day = stoi(parts.at(2));

		auto result = chrono::year_month_day{
			chrono::year{year},
			chrono::month(month),
			chrono::day(day)};

		if (result.ok())
		{
			return result;
		}
		else
		{
			return nullopt;
		}
	}
	catch (invalid_argument const &ex)
	{
		cerr << "conversion error: " << ex.what() << endl;
	}
	catch (out_of_range const &ex)
	{
		cerr << "conversion error: " << ex.what() << endl;
	}

	return nullopt;
}



// Returns `date` as a string in `YYYY-MM-DD` format.
// The ostream support for `std::chrono::year_month_day` is not
// available in most (any?) compilers, so we roll our own.
std::string Utilities::getStringFromDate(const std::chrono::year_month_day &date)
{
	std::ostringstream result;

	result
		<< std::setfill('0') << std::setw(4) << static_cast<int>(date.year())
		<< "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(date.month())
		<< "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(date.day());

	return result.str();
}


// Returns the value of the environment variable `name` as an `std::optional``
// value. If the variable exists, the value is a wrapped `std::string`,
// otherwise `std::nullopt`.
std::optional<std::string> Utilities::getEnvironmentVariable(const std::string &name)
{
	const char *value = std::getenv(const_cast<char *>(name.c_str()));
	if (nullptr != value)
	{
		std::string valueString = value;
		return valueString;
	}
	return std::nullopt;
}


void Utilities::print_birthday(auto currentDate)
{
	using namespace std;
	// Check the birthdate and user with generic helper functions
	auto birthdateValue = Utilities::getEnvironmentVariable("BIRTHDATE");
	if (birthdateValue.has_value())
	{
		auto birthdate = Utilities::getDateFromString(birthdateValue.value());
		ostringstream message;
		if (birthdate.has_value())
		{
			auto b = birthdate.value();
			if (b.month() == currentDate.month() && b.day() == currentDate.day())
			{
				message << "Happy birthday";
				auto userEnv = getEnvironmentVariable("USER");
				if (userEnv.has_value())
				{
					auto user = userEnv.value();
					message << ", " << user;
				}
				message << "! ";
			}

			int age = getNumberOfDaysBetween(
				chrono::floor<chrono::days>(chrono::sys_days{ b }),
				chrono::floor<chrono::days>(chrono::sys_days{ currentDate }));

			message << "You are " << age << " days old.";
			if (age % 1000 == 0)
			{
				message << " That's a nice round number!";
			}
			
			//display(message.str());
			//newline();
		}
	}
}




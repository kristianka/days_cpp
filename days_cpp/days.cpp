// days_cpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// Needed for the std::getenv function, VS 2022 complains otherwise.
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

// Parses the string `buf` for a date in YYYY-MM-DD format. If `buf` can be parsed,
// returns a wrapped `std::chrono::year_month_day` instances, otherwise `std::nullopt`.
// NOTE: Once clang++ and g++ implement chrono::from_stream, this could be replaced by something like this:
//  chrono::year_month_day birthdate;
//  std::istringstream bds{birthdateValue};
//  std::basic_istream<char> stream{bds.rdbuf()};
//  chrono::from_stream(stream, "%F", birthdate);
// However, I don't know how errors should be handled. Maybe this function could then
// continue to serve as a wrapper.
std::optional<std::chrono::year_month_day> getDateFromString(const std::string &buf)
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

// Returns the value of the environment variable `name` as an `std::optional``
// value. If the variable exists, the value is a wrapped `std::string`,
// otherwise `std::nullopt`.
std::optional<std::string> getEnvironmentVariable(const std::string &name)
{
	const char *value = std::getenv(const_cast<char *>(name.c_str()));
	if (nullptr != value)
	{
		std::string valueString = value;
		return valueString;
	}
	return std::nullopt;
}

// Returns `date` as a string in `YYYY-MM-DD` format.
// The ostream support for `std::chrono::year_month_day` is not
// available in most (any?) compilers, so we roll our own.
std::string getStringFromDate(const std::chrono::year_month_day &date)
{
	std::ostringstream result;

	result
		<< std::setfill('0') << std::setw(4) << static_cast<int>(date.year())
		<< "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(date.month())
		<< "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(date.day());

	return result.str();
}

// Print `T` to standard output.
// `T` needs to have an overloaded << operator.
template <typename T>
void display(const T &value)
{
	std::cout << value;
}

// Prints a newline to standard output.
inline void newline()
{
	std::cout << std::endl;
}

// Overload the << operator for the Event class.
// See https://learn.microsoft.com/en-us/cpp/standard-library/overloading-the-output-operator-for-your-own-classes?view=msvc-170
std::ostream &operator<<(std::ostream &os, const Event &event)
{
	os
		<< getStringFromDate(event.getTimestamp()) << ": "
		<< event.getDescription()
		<< " (" + event.getCategory() + ")";
	return os;
}

// Gets the number of days betweem to time points.
int getNumberOfDaysBetween(std::chrono::sys_days const &earlier, std::chrono::sys_days const &later)
{
	return (later - earlier).count();
}

std::vector<std::string> remove_commas(std::string args)
{
	std::vector<std::string> separated_args;
	size_t pos = args.find(",");

	while (pos != std::string::npos)
	{
		// Get the substring from the beginning to the comma
		std::string sub = args.substr(0, pos);

		separated_args.push_back(sub);

		// Erase the substring and the comma from the original string
		args.erase(0, pos + 1);

		// Find the next comma
		pos = args.find(',');
	}

	// Add the remaining string to the vector
	separated_args.push_back(args);
	return separated_args;
}


int main(int argc, char* argv[])
{
	using namespace std;

	// Get the current date from the system clock and extract year_month_day.
	// See https://en.cppreference.com/w/cpp/chrono/year_month_day
	const chrono::time_point now = chrono::system_clock::now();
	const chrono::year_month_day currentDate{ chrono::floor<chrono::days>(now) };

	// Check the birthdate and user with generic helper functions
	auto birthdateValue = getEnvironmentVariable("BIRTHDATE");
	if (birthdateValue.has_value())
	{
		auto birthdate = getDateFromString(birthdateValue.value());
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

			display(message.str());
			newline();
		}
	}

	// Note that you can't print an `std::chrono::year_month_day`
	// with `display()` because there is no overloaded << operator
	// for it (yet).

	// Construct a path for the events file.
	// If the user's home directory can't be determined, give up.
	string homeDirectoryString;
	auto homeString = getEnvironmentVariable("HOME");
	if (!homeString.has_value())
	{
		// HOME not found, maybe this is Windows? Try USERPROFILE.
		auto userProfileString = getEnvironmentVariable("USERPROFILE");
		if (!userProfileString.has_value())
		{
			std::cerr << "Unable to determine home directory";
			return 1;
		}
		else
		{
			homeDirectoryString = userProfileString.value();
		}
	}
	else
	{
		homeDirectoryString = homeString.value();
	}

	namespace fs = std::filesystem; // save a little typing
	fs::path daysPath{ homeDirectoryString };
	daysPath /= ".days"; // append our own directory
	if (!fs::exists(daysPath))
	{
		display(daysPath.string());
		display(" does not exist, please create it");
		newline();
		return 1; // nothing to do anymore, exit program

		// To create the directory:
		// std::filesystem::create_directory(daysPath);
		// See issue: https://github.com/jerekapyaho/days_cpp/issues/4
	}

	// Now we should have a valid path to the `~/.days` directory.
	// Construct a pathname for the `events.csv` file.
	auto eventsPath = daysPath / "events.csv";

	//
	// Read in the CSV file from `eventsPath` using RapidCSV
	// See https://github.com/d99kris/rapidcsv
	//
	rapidcsv::Document document{ eventsPath.string() };
	vector<string> dateStrings{ document.GetColumn<string>("date") };
	vector<string> categoryStrings{ document.GetColumn<string>("category") };
	vector<string> descriptionStrings{ document.GetColumn<string>("description") };

	vector<Event> events;
	for (size_t i{ 0 }; i < dateStrings.size(); i++)
	{
		auto date = getDateFromString(dateStrings.at(i));
		if (!date.has_value())
		{
			cerr << "bad date at row " << i << ": " << dateStrings.at(i) << '\n';
			continue;
		}

		Event event{
			date.value(),
			categoryStrings.at(i),
			descriptionStrings.at(i) };
		events.push_back(event);
	}

	const auto today = chrono::sys_days{
		floor<chrono::days>(chrono::system_clock::now()) };


	// NEW CODE

	string arg_list = "list";
	string arg_today = "--today";
	string arg_before = "--before-date";
	string arg_after = "--after-date";
	string arg_date = "--date";
	string arg_categories = "--categories";
	string arg_exclude = "--exclude";
	string arg_no_category = "--no-category";

	if (argc == 1)
	{
		cout << "No arguments given" << endl;
		return 0;
	}

	// if first argument is list
	if (argv[1] == arg_list)
	{
		// if only list argument, print all events
		if (argc == 2)
		{
			for (auto& event : events)
			{
				const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
				ostringstream line;
				line << event << " - ";
				if (delta < 0)
				{
					line << abs(delta) << " days ago";
				}
				else if (delta > 0)
				{
					line << "in " << delta << " days";
				}
				else
				{
					line << "today";
				}
				display(line.str());
				newline();
			}
			return 0;
		}

		// if argument is today, print today's events
		if (argv[2] == arg_today && argc == 3)
		{
			int count = 0;
			for (auto& event : events)
			{
				const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
				ostringstream line;
				line << event << " - ";
				if (delta == 0)
				{
					line << "today";
					display(line.str());
					newline();
					count++;
				}
			}
			// print nothing if no events today
			if (count == 0)
			{
				cout << "No events today" << endl;
			}
			return 0;
		}

		// if argument is before-date, print events before given date
		if (argv[2] == arg_before && argc <= 4)
		{
			// check if date is given
			if (argc == 3)
			{
				cout << "No date given" << endl;
				return 0;
			}
			auto date = getDateFromString(argv[3]);
			if (!date.has_value())
			{
				cerr << "bad date: " << argv[3] << '\n';
				return 0;
			}
			for (auto& event : events)
			{
				// check if current date is before given date
				if (event.getTimestamp() < date.value())
				{
					const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
					ostringstream line;
					line << event << " - ";
					line << abs(delta) << " days ago";
					display(line.str());
					newline();
				}

			}
			return 0;
		}

		// if argument is after-date, print events after given date
		if (argv[2] == arg_after && argc <= 4)
		{
			// check if date is given
			if (argc == 3)
			{
				cout << "No date given" << endl;
				return 0;
			}

			auto date = getDateFromString(argv[3]);
			if (!date.has_value())
			{
				cerr << "bad date: " << argv[3] << '\n';
				return 0;
			}

			for (auto& event : events)
			{
				// check if current date is after given date
				if (event.getTimestamp() > date.value())
				{
					const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
					ostringstream line;
					line << event << " - ";
					line << "in " << abs(delta) << " days";
					display(line.str());
					newline();
				}
			}
			return 0;
		}

		// combine both before-date and after-date
		if (argv[2] == arg_before && argv[4] == arg_after && argc <= 6)
		{
			// check if date is given
			if (argc != 6)
			{
				cout << "No enough arguments given" << endl;
				return 0;
			}

			auto before_date = getDateFromString(argv[3]);
			if (!before_date.has_value())
			{
				cerr << "bad date: " << argv[3] << '\n';
				return 0;
			}

			auto after_date = getDateFromString(argv[5]);
			if (!after_date.has_value())
			{
				cerr << "bad date: " << argv[5] << '\n';
				return 0;
			}

			int count = 0;
			for (auto& event : events)
			{
				const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
				ostringstream line;
				line << event << " - ";

				if (event.getTimestamp() < before_date.value() || event.getTimestamp() > after_date.value())
				{
					if (delta < 0)
					{
						line << abs(delta) << " days ago";
					}
					else if (delta > 0)
					{
						line << "in " << delta << " days";
					}
					else
					{
						line << "today";
					}
					display(line.str());
					newline();
					count++;
				}
			}

			// print nothing if no events in given date range
			if (count == 0)
			{
				cout << "No events found in the given date range" << endl;
			}
			return 0;
		}

		// if argument is just --date, print events on given date
		if (argv[2] == arg_date && argc <= 4)
		{
			cout << "in date";
			if (argc < 4)
			{
				cout << "No date given" << endl;
			}
			auto date = getDateFromString(argv[3]);
			if (!date.has_value())
			{
				cerr << "bad date: " << argv[3] << '\n';
				return 0;
			}

			int count = 0;

			for (auto& event : events)
			{
				const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
				ostringstream line;
				line << event << " - ";

				if (event.getTimestamp() == date)
				{
					if (delta < 0)
					{
						line << abs(delta) << " days ago";
					}
					else if (delta > 0)
					{
						line << "in " << delta << " days";
					}
					else
					{
						line << "today";
					}
					display(line.str());
					newline();
					count++;
				}
			}
			// print nothing if no events on given date
			if (count == 0)
			{
				cout << "No events found on the given date" << endl;
			}
			return 0;
		}


		// Categories!
		if (argv[2] == arg_categories)
		{
			if (argc < 4)
			{
				cout << "No category given" << endl;
				return 0;
			}

			std::vector arg_categories = remove_commas(argv[3]);
			int count = 0;
			// Exclude events with given categories
			bool exclude = false;
			// Events with no category
			bool no_category = false;

			if (argc > 4 && argv[4] == arg_exclude)
			{
				exclude = true;
			}
			std::cout << "no category" << no_category << std::endl;

			for (auto& event : events)
			{
				ostringstream line;
				line << event << " - ";

				if (exclude)
				{
					if (std::find(arg_categories.begin(), arg_categories.end(), event.getCategory()) == arg_categories.end())
					{
						const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
						if (delta < 0)
						{
							line << abs(delta) << " days ago";
						}
						else if (delta > 0)
						{
							line << "in " << delta << " days";
						}
						else
						{
							line << "today";
						}
						display(line.str());
						newline();
						count++;
					}
				}
				else
				{
					if (std::find(arg_categories.begin(), arg_categories.end(), event.getCategory()) != arg_categories.end())
					{
						const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
						if (delta < 0)
						{
							line << abs(delta) << " days ago";
						}
						else if (delta > 0)
						{
							line << "in " << delta << " days";
						}
						else
						{
							line << "today";
						}
						display(line.str());
						newline();
						count++;
					}
				}
			}

			// print nothing if no events in given category
			if (count == 0)
			{
				cout << "No events found in the given category" << endl;
			}
			return 0;
		}

		if (argc == 3 && argv[2] == arg_no_category)
		{
			int count = 0;
			for (auto& event : events)
			{
				ostringstream line;
				line << event << " - ";
				if (event.getCategory() == "")
				{
					const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
					if (delta < 0)
					{
						line << abs(delta) << " days ago";
					}
					else if (delta > 0)
					{
						line << "in " << delta << " days";
					}
					else
					{
						line << "today";
					}
					display(line.str());
					newline();
					count++;
				}
			}
			if (count == 0)
			{
				cout << "No events found with no category" << endl;
			}
			return 0;
		}
	}

	// ADDING EVENTS
	string arg_add = "add";
	string arg_category = "--category";
	string arg_description = "--description";

	if (argv[1] == arg_add)
	{
		cout << "in add" << endl;
		if (argc < 3)
		{
			cout << "No date given" << endl;
			return 0;
		}
		auto date = getDateFromString(argv[3]);
		if (!date.has_value())
		{
			cerr << "bad date: " << argv[3] << '\n';
			return 0;
		}
		string category = "";
		string description = "";

		for (int i = 4; i < argc; i++)
		{
			if (argv[i] == arg_category)
			{
				category = argv[i + 1];
			}
			if (argv[i] == arg_description)
			{
				description = argv[i + 1];
			}
		}

		Event event(date.value(), category, description);
		std::cout << event << std::endl;
		events.push_back(event);

		// Write to events.csv in events path
		ofstream file;
		std::cout << eventsPath << std::endl;
		file.open(eventsPath, ios::app);
		file << event << endl;

		file.close();

		return 0;
	}


	return 0;
}
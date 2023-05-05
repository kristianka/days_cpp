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
#include "Utilities.h"


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
	Utilities tools;
	os
		<< tools.getStringFromDate(event.getTimestamp()) << ": "
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

void print_day_format(int delta, auto event)
{
	std::ostringstream line;
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



int main(int argc, char* argv[])
{
	using namespace std;
	Utilities tools;
	// Get the current date from the system clock and extract year_month_day.
	// See https://en.cppreference.com/w/cpp/chrono/year_month_day
	const chrono::time_point now = chrono::system_clock::now();
	const chrono::year_month_day currentDate{ chrono::floor<chrono::days>(now) };

	// Check the birthdate and user with generic helper functions
	auto birthdateValue = tools.getEnvironmentVariable("BIRTHDATE");
	if (birthdateValue.has_value())
	{
		auto birthdate = tools.getDateFromString(birthdateValue.value());
		ostringstream message;
		if (birthdate.has_value())
		{
			auto b = birthdate.value();
			if (b.month() == currentDate.month() && b.day() == currentDate.day())
			{
				message << "Happy birthday";
				auto userEnv = tools.getEnvironmentVariable("USER");
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
	auto homeString = tools.getEnvironmentVariable("HOME");
	if (!homeString.has_value())
	{
		// HOME not found, maybe this is Windows? Try USERPROFILE.
		auto userProfileString = tools.getEnvironmentVariable("USERPROFILE");
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
		auto date = tools.getDateFromString(dateStrings.at(i));
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
		int count = 0;
		// if only list argument, print all events
		if (argc == 2)
		{
			for (auto& event : events)
			{
				const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
				print_day_format(delta, event);
			}
			return 0;
		}

		// if argument is today, print today's events
		if (argv[2] == arg_today && argc == 3)
		{
			for (auto& event : events)
			{
				const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
				if (delta == 0)
				{
					print_day_format(delta, event);
					count++;
				}
			}
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
			auto date = tools.getDateFromString(argv[3]);
			if (!date.has_value())
			{
				cerr << "bad date: " << argv[3] << '\n';
				return 0;
			}
			for (auto& event : events)
			{
				const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
				// check if current date is before given date
				if (event.getTimestamp() < date.value())
				{
					print_day_format(delta, event);
					count++;
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

			auto date = tools.getDateFromString(argv[3]);
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
					print_day_format(delta, event);
					count++;
				}
			}
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

			auto before_date = tools.getDateFromString(argv[3]);
			if (!before_date.has_value())
			{
				cerr << "bad date: " << argv[3] << '\n';
				return 0;
			}

			auto after_date = tools.getDateFromString(argv[5]);
			if (!after_date.has_value())
			{
				cerr << "bad date: " << argv[5] << '\n';
				return 0;
			}

			for (auto& event : events)
			{
				if (event.getTimestamp() < before_date.value() || event.getTimestamp() > after_date.value())
				{
					const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
					print_day_format(delta, event);
					count++;
				}
			}
		}

		// if argument is just --date, print events on given date
		if (argv[2] == arg_date && argc <= 4)
		{
			if (argc < 4)
			{
				cout << "No date given" << endl;
			}
			auto date = tools.getDateFromString(argv[3]);
			if (!date.has_value())
			{
				cerr << "bad date: " << argv[3] << '\n';
				return 0;
			}

			for (auto& event : events)
			{
				const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
				if (event.getTimestamp() == date)
				{
					print_day_format(delta, event);
					count++;
				}
			}
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

			// Exclude events with given categories
			bool exclude = false;
			// Events with no category
			bool no_category = false;

			if (argc > 4 && argv[4] == arg_exclude)
			{
				exclude = true;
			}

			for (auto& event : events)
			{
				if (exclude)
				{
					if (std::find(arg_categories.begin(), arg_categories.end(), event.getCategory()) == arg_categories.end())
					{
						const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
						print_day_format(delta, event);
						count++;
					}
				}
				else
				{
					if (std::find(arg_categories.begin(), arg_categories.end(), event.getCategory()) != arg_categories.end())
					{
						const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
						print_day_format(delta, event);
						count++;
					}
				}
			}
		}

		if (argc == 3 && argv[2] == arg_no_category)
		{
			for (auto& event : events)
			{
				if (event.getCategory() == "")
				{
					const auto delta = (chrono::sys_days{ event.getTimestamp() } - today).count();
					print_day_format(delta, event);
					count++;
				}
			}
		}

		// print nothing if no events found
		if (count == 0)
		{
			cout << "No events found" << endl;
		}
		return 0;
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
		auto date = tools.getDateFromString(argv[3]);
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
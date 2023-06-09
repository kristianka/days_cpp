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
#include <fstream>

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

// This functions works by copying the events.csv file to a events.csv.tmp file,
// then deleting the events.csv file and renaming the temp file to events.csv!
void update_csv_file(auto& eventsPath, auto& tempPath, char *argv[], Event& event)
{
	try {
		namespace fs = std::filesystem; // save a little typing
		using std::string;
		std::stringstream ss;
		ss << event.getTimestamp() << "," << event.getCategory() << "," << event.getDescription();
		std::string event_formatted = ss.str(); // get the string from the stringstream

		string deleteline = event_formatted;
		string line;
		string newline;

		// events file
		std::ifstream fin;
		fin.open(eventsPath.string());

		// temp file
		std::ofstream temp;
		temp.open(tempPath.string());

		while (std::getline(fin, line)) {
			if (line.find(deleteline) == std::string::npos) {
				temp << line << "\n";
			}
		}

		temp.close();
		fin.close();

		// delete events file and rename temp file to events file
		fs::remove(eventsPath.string().c_str());
		fs::rename(tempPath.string().c_str(), eventsPath.string().c_str());

		std::cout << "Deleted event " << event << std::endl;
	}
	catch (const std::exception&)
	{
		std::cout << "An error occured while writing to file." << std::endl;
	}
}

int main(int argc, char* argv[])
{
	Utilities tools;
	using std::cout, std::string, std::endl, std::vector, std::ofstream;
	// Get the current date from the system clock and extract year_month_day.
	// See https://en.cppreference.com/w/cpp/chrono/year_month_day
	const std::chrono::time_point now = std::chrono::system_clock::now();
	const std::chrono::year_month_day currentDate{ std::chrono::floor<std::chrono::days>(now) };

	// Construct a path for the events file.
	// If the user's home directory can't be determined, give up.
	std::string homeDirectoryString;
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
	auto tempPath = daysPath / "events.csv.tmp";

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
			std::cerr << "bad date at row " << i << ": " << dateStrings.at(i) << '\n';
			continue;
		}

		Event event{
			date.value(),
			categoryStrings.at(i),
			descriptionStrings.at(i) };
		events.push_back(event);
	}

	if (events.size() == 0)
	{
		cout << "No events found" << endl;
		return 0;
	}

	const auto today = std::chrono::sys_days{
		floor<std::chrono::days>(std::chrono::system_clock::now()) };


	// Command line arguments
	string arg_list = "list";
	string arg_today = "--today";
	string arg_before = "--before-date";
	string arg_after = "--after-date";
	string arg_date = "--date";
	string arg_categories = "--categories";
	string arg_exclude = "--exclude";
	string arg_no_category = "--no-category";

	// Counter for printing not found
	int count = 0;

	// If only command is days, show error
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
				const auto delta = (std::chrono::sys_days{ event.getTimestamp() } - today).count();
				print_day_format(delta, event);
			}
			return 0;
		}

		// if argument after list is today, print today's events
		if (argv[2] == arg_today && argc == 3)
		{
			for (auto& event : events)
			{
				const auto delta = (std::chrono::sys_days{ event.getTimestamp() } - today).count();
				if (delta == 0)
				{
					print_day_format(delta, event);
					count++;
				}
			}
		}

		// List events by before, after or both. Also list events on a specific date
		if (argc > 2 && (argv[2] == arg_before || argv[2] == arg_after || argv[2] == arg_date))
		{
			if (argc < 4)
			{
				cout << "No date given" << endl;
				return 0;
			}

			bool before = false;
			bool after = false;
			bool on_this_date = false;
			auto date1 = tools.getDateFromString(argv[3]);
			auto date2 = tools.getDateFromString(argv[3]);

			if (argv[2] == arg_before)
			{
				before = true;
			}

			if (argv[2] == arg_after)
			{
				after = true;
			}

			if (argc > 4 && argv[2] == arg_before && argv[4] == arg_after)
			{
				before = true;
				after = true;
				date1 = tools.getDateFromString(argv[3]);
				date2 = tools.getDateFromString(argv[5]);
			}

			if (argv[2] == arg_date)
			{
				on_this_date = true;
			}

			for (auto& event : events)
			{
				const auto delta = (std::chrono::sys_days{ event.getTimestamp() } - today).count();
				if (before)
				{
					if (event.getTimestamp() < date1)
					{
						print_day_format(delta, event);
						count++;
					}
				}
				if (after)
				{
					if (event.getTimestamp() > date2)
					{
						print_day_format(delta, event);
						count++;
					}
				}
				if (on_this_date)
				{
					if (event.getTimestamp() == date1)
					{
						print_day_format(delta, event);
						count++;
					}
				}
			}
		}

		// Categories
		// if argument after list is --categories
		if (argv[2] == arg_categories)
		{
			if (argc < 4)
			{
				cout << "No category given" << endl;
				return 0;
			}

			// Put categories to vector and remove commas
			std::vector arg_categories = remove_commas(argv[3]);

			// Exclude events with given categories, check if --exclude is given
			bool exclude = exclude = (argc > 4 && argv[4] == arg_exclude);
			// Events with no category
			bool no_category = false;

			for (auto& event : events)
			{
				if (exclude)
				{
					// If the event category is not in the arg_categories vector, then print it
					if (std::find(arg_categories.begin(), arg_categories.end(), event.getCategory()) == arg_categories.end())
					{
						const auto delta = (std::chrono::sys_days{ event.getTimestamp() } - today).count();
						print_day_format(delta, event);
						count++;
					}
				}
				else
				{
					// If the event category is in the arg_categories vector, then print it
					if (std::find(arg_categories.begin(), arg_categories.end(), event.getCategory()) != arg_categories.end())
					{
						const auto delta = (std::chrono::sys_days{ event.getTimestamp() } - today).count();
						print_day_format(delta, event);
						count++;
					}
				}
			}
		}

		// if argument after list is --no-category
		if (argc == 3 && argv[2] == arg_no_category)
		{
			for (auto& event : events)
			{
				if (event.getCategory() == "")
				{
					const auto delta = (std::chrono::sys_days{ event.getTimestamp() } - today).count();
					print_day_format(delta, event);
					count++;
				}
			}
		}
	}

	// Arguments for adding events
	string arg_add = "add";
	string arg_category = "--category";
	string arg_description = "--description";
	string category = "";
	string description = "";

	// if argument after days is --add
	if (argv[1] == arg_add)
	{
		bool date_given = true;
		// check if enough arguments
		if (argc < 3)
		{
			cout << "No date given or wrong formatting" << endl;
			return 0;
		}

		auto date = tools.getDateFromString(argv[3]);

		// if no date given, use today
		if (argv[2] != arg_date)
		{
			date = today;
			date_given = false;
		}

		// check if date is valid
		if (date_given == true && !date.has_value())
		{
			std::cerr << "bad date: " << argv[3] << '\n';
			return 0;
		}

		for (int i = 2; i < argc; i++)
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

		// Push to event vector, not really necessary since the program will end after this
		Event event(date.value(), category, description);
		events.push_back(event);

		// Build string that has formatted event for the .csv file
		std::stringstream ss;
		ss << event.getTimestamp() << "," << event.getCategory() << "," << event.getDescription();
		std::string event_formatted = ss.str(); // get the string from the stringstream

		// Write to events.csv in events path
		try
		{
			ofstream file;
			file.open(eventsPath.string(), std::ios::out | std::ios::app);
			file << event_formatted << endl;
			file.close();
			std::cout << "Successfully added event " << event << std::endl;
			count++;
		}
		catch (const std::exception&)
		{
			std::cout << "Error opening file" << std::endl;
		}
		
	}

	// Arguments for deleting events
	string arg_delete = "delete";
	string arg_all = "--all";
	string arg_dry_run = "--dry-run";

	// If delete is given as first argument after days
	if (argv[1] == arg_delete)
	{

		if (argc < 3)
		{
			std::cout << "No date given or wrong formatting" << endl;
			return 0;
		}

		int length = argc - 1;

		// If --description or --category is given as first argument after delete
		if (argc > 2 && (argv[2] == arg_description || argv[2] == arg_category))
		{
			bool is_description = (argv[2] == arg_description);
			for (auto& event : events)
			{
				if ((is_description && event.getDescription().starts_with(argv[3]) && argv[length] == arg_dry_run)
					|| (!is_description && event.getCategory() == argv[3] && argv[length] == arg_dry_run))
				{
					std::cout <<  event << " would have been deleted without dry run" << endl;
					count++;
				}

				if ((is_description && event.getDescription().starts_with(argv[3]) && argv[length] != arg_dry_run)
					|| (!is_description && event.getCategory() == argv[3] && argv[length] != arg_dry_run))
				{
					update_csv_file(eventsPath, tempPath, argv, event);
					count++;
				}
			}
		}

		// If --date is given as first argument after delete
		if (argv[2] == arg_date)
		{
			auto date = tools.getDateFromString(argv[3]);
			if (!date.has_value())	
			{
				std::cerr << "bad date: " << argv[3] << '\n';
				return 0;
			}
		
			// Check if arguments have --category and --description
			bool has_category = (argc > 5 && argv[4] == arg_category);
			bool has_description = (argc > 6 && argv[6] == arg_description);

			if (has_category)
			{
				for (int i = 2; i < argc; i++)
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
			}

			for (auto& event : events)
			{
				// If category is given, find events with given date and category
				if (has_category)
				{
					// If description is not given, find events with given date and category
					if (!has_description)
					{
						// Find events with given date and category. If --dry-run is given, print out the events that would have been deleted
						if (event.getTimestamp() == date.value() && event.getCategory() == category && argv[length] == arg_dry_run)
						{
							std::cout << event << " would have been deleted without dry run" << endl;
							count++;
						}

						// If --dry-run is not given, update the csv file
						if (event.getTimestamp() == date.value() && event.getCategory() == category && argv[length] != arg_dry_run)
						{
							update_csv_file(eventsPath, tempPath, argv, event);
							count++;
						}
					}
					// If description is given, find events with given date, category and description
					if (has_description)
					{
						// Same as above, but also checks if event description starts with given description
						if (event.getTimestamp() == date.value() && event.getCategory() == category &&
							event.getDescription().starts_with(description) && argv[length] == arg_dry_run)
						{
							std::cout << event << " would have been deleted without dry run" << endl;
							count++;
						}

						// If --dry-run is not given, update the csv file
						if (event.getTimestamp() == date.value() && event.getCategory() == category &&
							event.getDescription().starts_with(description) && argv[length] != arg_dry_run)
						{
							update_csv_file(eventsPath, tempPath, argv, event);
							count++;
						}
					}
				}

				// If category is not given, find events just with given date
				if (!has_category)
				{
					if (event.getTimestamp() == date.value() && argv[length] == arg_dry_run)
					{
						std::cout << event << " would have been deleted without dry run" << endl;
						count++;
					}

					if (event.getTimestamp() == date.value() && argv[length] != arg_dry_run)
					{
						update_csv_file(eventsPath, tempPath, argv, event);
						count++;
					}
				}
			}	
		}

		// If --all is given as argument after delete
		if (argv[2] == arg_all)
		{
			// If --dry-run is given, just print what would have been deleted
			if (argc > 3 && argv[3] == arg_dry_run)
			{
				for (auto& event : events)
				{
					std::cout << event << " would have been deleted without dry run" << endl;
					count++;
				}
			} 

			// If --dry-run is not given, delete all events
			if (argc == 3)
			{
				// Empty events.csv
				try
				{
					for (auto& event : events)
					{
						update_csv_file(eventsPath, tempPath, argv, event);
						count++;
					}
					std::cout << "Deleted all events" << endl;
				}
				catch (const std::exception&)
				{
					std::cout << "Error opening file" << std::endl;
				}
			}
		}
		string arg_between = "--between";

		// --between command
		if (argv[2] == arg_between)
		{
			if (argc != 5 && argc != 6)
			{
				std::cerr << "No dates given or wrong formatting" << endl;
				return 0;
			}

			auto date1 = tools.getDateFromString(argv[3]);
			auto date2 = tools.getDateFromString(argv[4]);
			if (!date1.has_value() || !date2.has_value())
			{
				std::cerr << "bad date: " << argv[3] << " or " << argv[4] << '\n';
				return 0;
			}

			// If --dry-run is given, just print what would have been deleted
			for (auto& event : events)
			{
				if (event.getTimestamp() >= date1.value() && event.getTimestamp() <= date2.value() && argv[length] == arg_dry_run)
				{
					std::cout << event << " would have been deleted without dry run" << endl;
					count++;
				}

				if (event.getTimestamp() >= date1.value() && event.getTimestamp() <= date2.value() && argv[length] != arg_dry_run)
				{
					update_csv_file(eventsPath, tempPath, argv, event);
					count++;
				}
			}
		
		}
	}

	// If no events were printed, print this
	if (count == 0)
	{
		std::cout << "No events found" << endl;
	}

	return 0;
}

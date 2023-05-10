# days_cpp

Course project for Ohjelmoinnin syventävät teknologiat -course. Original code is from https://github.com/jerekapyaho/days_cpp please read the readme from there to know how to use the program!

## Building and running the program 

### For all platforms:

- Clone the repository

- Go to the cloned folder where ```.cpp``` files are

### On Linux:
- Run this command: ```g++ *.cpp -o days```

- Run the program, for example ```./days list``` will list all the events.

### On Windows: 
- Open ```Developer Command Prompt for VS 2022```, go to the cloned directory that has the ```.cpp``` files and run this command: ```cl /std:c++20 /EHsc days.cpp Event.cpp Utilities.cpp```

- Run the program, for example ```.\days.exe list``` or ```days.exe list```  will list all the events.

---

### Example ```events.csv``` file

```
date,category,description
1985-12-31,computing,C++ released
2010-01-01,sport,Go
2014-11-12,computing,.NET Core released
2020-12-15,computing,C++20 released
2022-09-20,computing,Java SE 19 released
2023-01-10,computing,Rust 1.66.1 released
2023-03-12,games,New game releases
2023-05-09,school,Today is Tuesday
2023-05-10,school,Today is Wednesday
2023-05-10,school,Starting course work
2023-05-10,,This is a event without category
2023-06-23,holidays,Juhannusaatto
2023-06-30,quarters,Quarter 2 ends
2023-12-31,computing,C++23 released
2030-01-01,,No category
2030-01-01,,No category
2038-01-19,computing,Unix clock rolls over
```

### Acknowledgments


RapidCSV

  
- Copyright (c) 2017, Kristofer Berggren

- Licensed under the BSD 3-Clause License. See LICENSE.

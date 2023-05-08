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
- Open ```Developer Command Prompt for VS 2022``` and run this command: ```cl /std:c++20 /EHsc days.cpp Event.cpp Utilities.cpp```

- Run the program, for example ```.\days.exe list``` or ```days.exe list```  will list all the events.

---

### Acknowledgments


RapidCSV

  
- Copyright (c) 2017, Kristofer Berggren

- Licensed under the BSD 3-Clause License. See LICENSE.

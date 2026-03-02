/**
 * ProjectTwo.cpp
 * A command-line program for the ABCU Computer Science department advising system.
 * Uses a hash table to store and retrieve course information.
 * Supports loading course data from a CSV file, listing all courses alphanumerically,
 * and looking up individual course details including prerequisites.
 *
 * Author: Aarorn Summers
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>

// Course Class Definition

// Represents a single course with a number, title, and list of prerequisites.
class Course {
public:
    std::string courseNumber;               
    std::string courseTitle;                
    std::vector<std::string> prerequisites; 

    // Default constructor
    Course() {}

    // Parameterized constructor
    Course(const std::string& number, const std::string& title,
           const std::vector<std::string>& prereqs)
        : courseNumber(number), courseTitle(title), prerequisites(prereqs) {}
};

// Hash Table Implementation

// Node for chaining within a hash table bucket (linked-list chaining).
struct HashNode {
    Course course;
    HashNode* next;

    HashNode(const Course& c) : course(c), next(nullptr) {}
};

// Hash table that maps course numbers (strings) to Course objects.
class HashTable {
private:
    static const int DEFAULT_BUCKETS = 179; 
    int numBuckets;
    std::vector<HashNode*> buckets;

    // Converts a string key into a bucket index using polynomial hashing.
    int hashFunction(const std::string& key) const {
        unsigned long hash = 0;
        for (char c : key) {
            hash = hash * 31 + static_cast<unsigned char>(c);
        }
        return static_cast<int>(hash % numBuckets);
    }

public:
    // Constructor
    HashTable(int buckets = DEFAULT_BUCKETS) : numBuckets(buckets) {
        this->buckets.resize(numBuckets, nullptr);
    }

    // Destructor 
    ~HashTable() {
        for (int i = 0; i < numBuckets; ++i) {
            HashNode* node = buckets[i];
            while (node != nullptr) {
                HashNode* temp = node;
                node = node->next;
                delete temp;
            }
        }
    }

    // Insert a course into the hash table.
    void insert(const Course& course) {
        int index = hashFunction(course.courseNumber);
        HashNode* node = buckets[index];

        // Check for existing entry with same courseNumber (overwrite)
        while (node != nullptr) {
            if (node->course.courseNumber == course.courseNumber) {
                node->course = course; 
                return;
            }
            node = node->next;
        }

        // Prepend new node to bucket chain
        HashNode* newNode = new HashNode(course);
        newNode->next = buckets[index];
        buckets[index] = newNode;
    }

    // Search for a course by its course number.
    Course* search(const std::string& courseNumber) {
        int index = hashFunction(courseNumber);
        HashNode* node = buckets[index];

        while (node != nullptr) {
            if (node->course.courseNumber == courseNumber) {
                return &node->course;
            }
            node = node->next;
        }
        return nullptr; 
    }

    
    // Returns true if the hash table contains no courses.
    bool isEmpty() const {
        for (int i = 0; i < numBuckets; ++i) {
            if (buckets[i] != nullptr) return false;
        }
        return true;
    }

    
    // Retrieve all courses stored in the hash table as a flat vector.
    std::vector<Course> getAllCourses() const {
        std::vector<Course> allCourses;
        for (int i = 0; i < numBuckets; ++i) {
            HashNode* node = buckets[i];
            while (node != nullptr) {
                allCourses.push_back(node->course);
                node = node->next;
            }
        }
        return allCourses;
    }
};

// Trims whitespace from both ends of a string
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// Convert string to uppercase for case-insensitive compare
std::string toUpper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

// File Loading Function

// Loads course data from a CSV file into the provided hash table.
bool loadDataStructure(const std::string& filename, HashTable& hashTable) {
    std::ifstream inFile(filename);

    // Check if file opened successfully
    if (!inFile.is_open()) {
        std::cerr << "Error: File \"" << filename << "\" could not be opened." << std::endl;
        return false;
    }

    // Pass 1: Collect all course numbers for prerequisite validation
    std::unordered_set<std::string> courseNumberSet;
    std::string line;

    while (std::getline(inFile, line)) {
        line = trim(line);
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        std::getline(ss, token, ',');
        std::string courseNum = trim(token);

        if (!courseNum.empty()) {
            // Detect duplicates during pass 1
            if (courseNumberSet.count(courseNum) > 0) {
                std::cerr << "Warning: Duplicate course number detected: \""
                          << courseNum << "\". Later entry will overwrite earlier." << std::endl;
            }
            courseNumberSet.insert(courseNum);
        }
    }

    // Reset file stream for pass 2
    inFile.clear();
    inFile.seekg(0, std::ios::beg);

    // Pass 2: Parse full course data and load into hash table
    bool anyError = false;
    int lineNumber = 0;

    while (std::getline(inFile, line)) {
        ++lineNumber;
        line = trim(line);

        if (line.empty()) continue; 

        // Split the line by commas into tokens
        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(trim(token));
        }

        // Validate: must have at least courseNumber and courseTitle
        if (tokens.size() < 2) {
            std::cerr << "Error: Line " << lineNumber
                      << " has invalid format (fewer than 2 fields). Skipping: \""
                      << line << "\"" << std::endl;
            anyError = true;
            continue;
        }

        std::string courseNumber = tokens[0];
        std::string courseTitle  = tokens[1];

        // Validate course number is not empty
        if (courseNumber.empty()) {
            std::cerr << "Error: Line " << lineNumber
                      << " has an empty course number. Skipping." << std::endl;
            anyError = true;
            continue;
        }

        // Validate course title is not empty
        if (courseTitle.empty()) {
            std::cerr << "Error: Line " << lineNumber
                      << " has an empty course title for course \""
                      << courseNumber << "\". Skipping." << std::endl;
            anyError = true;
            continue;
        }

        // Collect and validate prerequisites
        std::vector<std::string> prereqs;
        for (size_t i = 2; i < tokens.size(); ++i) {
            std::string prereq = tokens[i];
            if (prereq.empty()) continue; // Skip empty trailing commas

            // Check that the prerequisite actually exists as a course
            if (courseNumberSet.count(prereq) == 0) {
                std::cerr << "Warning: Course \"" << courseNumber
                          << "\" lists prerequisite \"" << prereq
                          << "\" which does not exist in the file." << std::endl;
                // We still add it (lenient) but warn the user
            } 
            prereqs.push_back(prereq);
        }

        // Build and insert the course
        Course course(courseNumber, courseTitle, prereqs);
        hashTable.insert(course);
    }

    inFile.close();

    if (courseNumberSet.empty()) {
        std::cerr << "Error: No valid courses were found in the file." << std::endl;
        return false;
    }

    std::cout << "Success: Course data loaded from \"" << filename << "\"." << std::endl;
    if (anyError) {
        std::cout << "Note: Some lines were skipped due to formatting errors (see above)." << std::endl;
    }
    return true;
}

// Display Functions

// Prints a single course's number, title, and prerequisites.
void printCourse(const Course& course) {
    std::cout << course.courseNumber << ", " << course.courseTitle << std::endl;

    if (!course.prerequisites.empty()) {
        std::cout << "Prerequisites: ";
        for (size_t i = 0; i < course.prerequisites.size(); ++i) {
            std::cout << course.prerequisites[i];
            if (i + 1 < course.prerequisites.size()) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << "Prerequisites: None" << std::endl;
    }
}

// Retrieves all courses from the hash table, sorts them alphanumerically by course number, and prints them.
void printSortedCourseList(HashTable& hashTable) {
    // Gather all courses into a temporary list
    std::vector<Course> tempList = hashTable.getAllCourses();

    if (tempList.empty()) {
        std::cout << "No courses available to display." << std::endl;
        return;
    }

    // Sort alphanumerically by courseNumber (ascending)
    std::sort(tempList.begin(), tempList.end(),
              [](const Course& a, const Course& b) {
                  return a.courseNumber < b.courseNumber;
              });

    // Print the sorted list
    std::cout << "\n--- Computer Science Course List ---" << std::endl;
    for (const Course& course : tempList) {
        std::cout << course.courseNumber << ", " << course.courseTitle << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;
}

// Prompts the user for a course number, looks it up, and prints its details.
void printCourseInformation(HashTable& hashTable) {
    std::string courseNumber;
    std::cout << "Enter course number: ";
    std::cin >> courseNumber;

    // Normalize to uppercase for case-insensitive lookup
    courseNumber = toUpper(trim(courseNumber));

    Course* course = hashTable.search(courseNumber);

    if (course == nullptr) {
        std::cout << "Error: Course \"" << courseNumber << "\" was not found." << std::endl;
        return;
    }

    // Print course number and title
    std::cout << "\n" << course->courseNumber << ", " << course->courseTitle << std::endl;

    // Print prerequisites with their titles if resolvable
    if (!course->prerequisites.empty()) {
        std::cout << "Prerequisites: ";
        for (size_t i = 0; i < course->prerequisites.size(); ++i) {
            const std::string& prereqNum = course->prerequisites[i];
            Course* prereqCourse = hashTable.search(prereqNum);

            if (prereqCourse != nullptr) {
                // Show both number and title
                std::cout << prereqCourse->courseNumber << " - " << prereqCourse->courseTitle;
            } else {
                // Prerequisite number only (could not be resolved)
                std::cout << prereqNum << " (title not found)";
            }

            if (i + 1 < course->prerequisites.size()) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << "Prerequisites: None" << std::endl;
    }
}

// Menu and Main

//Displays the main menu options to the user.
void displayMenu() {
    std::cout << "\n=============================" << std::endl;
    std::cout << " ABCU Course Advising System " << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "  1. Load Course Data" << std::endl;
    std::cout << "  2. Print Course List (Alphanumeric)" << std::endl;
    std::cout << "  3. Print Course Information" << std::endl;
    std::cout << "  9. Exit" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Enter your choice: ";
}


// Main entry point. Manages the menu loop and user interaction.
int main() {
    HashTable hashTable;    
    bool dataLoaded = false;

    int choice = 0;

    std::cout << "Welcome to the ABCU Course Advising System." << std::endl;

    // Main menu loop
    while (choice != 9) {
        displayMenu();

        // Validate that the user entered an integer
        if (!(std::cin >> choice)) {
            std::cin.clear();                                                  
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Error: Please enter a valid menu option (1, 2, 3, or 9)." << std::endl;
            continue;
        }

        switch (choice) {
            // Option 1: Load data from file
            case 1: {
                std::string filename;
                std::cout << "Enter the file name containing course data: ";
                std::cin >> filename;
                filename = trim(filename);

                // Reload: clear old table by reinitializing
                hashTable = HashTable();
                dataLoaded = loadDataStructure(filename, hashTable);
                break;
            }

            // Option 2: Print sorted course list 
            case 2: {
                if (!dataLoaded || hashTable.isEmpty()) {
                    std::cout << "Error: No course data loaded. Please select Option 1 first." << std::endl;
                } else {
                    printSortedCourseList(hashTable);
                }
                break;
            }

            // Option 3: Print individual course info
            case 3: {
                if (!dataLoaded || hashTable.isEmpty()) {
                    std::cout << "Error: No course data loaded. Please select Option 1 first." << std::endl;
                } else {
                    printCourseInformation(hashTable);
                }
                break;
            }

            // Option 9: Exit 
            case 9: {
                std::cout << "Thank you for using the ABCU Course Advising System. Goodbye!" << std::endl;
                break;
            }

            // Invalid option 
            default: {
                std::cout << "Error: \"" << choice
                          << "\" is not a valid option. Please choose 1, 2, 3, or 9." << std::endl;
                break;
            }
        }
    }

    return 0;
}
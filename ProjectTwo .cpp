// Name: Aaryana Glover
// CS-300
//============================================================================
// Name        : ProjectTwo.cpp
// Description : Command-line advising program. Loads course data from a CSV
//               file into a binary search tree, prints the courses in
//               alphanumeric order, and looks up a single course with its
//               prerequisites.
//============================================================================

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

//============================================================================
// Course structure
//============================================================================
struct Course {
    string courseNumber;
    string name;
    vector<string> prerequisites;   // course numbers, may be empty
};

//============================================================================
// Small string helpers
//============================================================================

// Remove leading/trailing whitespace (including a stray '\r' from Windows files)
string trim(const string& text) {
    size_t start = 0;
    size_t end = text.size();
    while (start < end && isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    while (end > start && isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(start, end - start);
}

// Convert to upper case so lookups are not case sensitive (csci400 == CSCI400)
string toUpper(string text) {
    transform(text.begin(), text.end(), text.begin(),
              [](unsigned char c) { return static_cast<char>(toupper(c)); });
    return text;
}

// Split a line on commas and trim every token
vector<string> split(const string& line) {
    vector<string> tokens;
    stringstream stream(line);
    string token;
    while (getline(stream, token, ',')) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

//============================================================================
// Binary search tree of courses, ordered by courseNumber
//============================================================================
class BinarySearchTree {
private:
    struct Node {
        Course course;
        Node* left;
        Node* right;
        explicit Node(const Course& aCourse)
            : course(aCourse), left(nullptr), right(nullptr) {}
    };

    Node* root;

    // Recursively place the course where it belongs
    void addNode(Node*& node, const Course& course) {
        if (node == nullptr) {
            node = new Node(course);
        } else if (course.courseNumber < node->course.courseNumber) {
            addNode(node->left, course);
        } else if (course.courseNumber > node->course.courseNumber) {
            addNode(node->right, course);
        } else {
            node->course = course;  // same course number: keep the newest data
        }
    }

    // Left, node, right gives alphanumeric order
    void inOrder(const Node* node) const {
        if (node != nullptr) {
            inOrder(node->left);
            cout << node->course.courseNumber << ", " << node->course.name << endl;
            inOrder(node->right);
        }
    }

    void destroy(Node* node) {
        if (node != nullptr) {
            destroy(node->left);
            destroy(node->right);
            delete node;
        }
    }

public:
    BinarySearchTree() : root(nullptr) {}
    ~BinarySearchTree() { destroy(root); }

    // The tree owns raw pointers, so copying is not allowed
    BinarySearchTree(const BinarySearchTree&) = delete;
    BinarySearchTree& operator=(const BinarySearchTree&) = delete;

    bool Empty() const { return root == nullptr; }

    void Clear() {
        destroy(root);
        root = nullptr;
    }

    void Insert(const Course& course) { addNode(root, course); }

    // Returns a pointer to the course, or nullptr if it is not in the tree
    const Course* Search(const string& courseNumber) const {
        const Node* current = root;
        while (current != nullptr) {
            if (courseNumber == current->course.courseNumber) {
                return &current->course;
            }
            current = (courseNumber < current->course.courseNumber)
                          ? current->left
                          : current->right;
        }
        return nullptr;
    }

    void PrintAll() const { inOrder(root); }
};

//============================================================================
// Load the file: read, parse, validate, then store in the tree
//============================================================================
bool loadCourses(const string& fileName, BinarySearchTree& tree) {
    ifstream file(fileName);
    if (!file.is_open()) {
        cout << "Error: cannot open file " << fileName << endl;
        return false;
    }

    vector<vector<string>> lines;       // tokens for every line
    vector<string> courseNumbers;       // every course number in the file

    // First pass: read each line, split it, and check it has at least 2 values
    string line;
    int lineNumber = 0;
    while (getline(file, line)) {
        ++lineNumber;
        if (trim(line).empty()) {
            continue;                   // skip blank lines
        }

        vector<string> tokens = split(line);
        if (tokens.size() < 2 || tokens[0].empty() || tokens[1].empty()) {
            cout << "Error: line " << lineNumber
                 << " needs a course number and a name" << endl;
            return false;
        }

        tokens[0] = toUpper(tokens[0]);
        for (size_t i = 2; i < tokens.size(); ++i) {
            tokens[i] = toUpper(tokens[i]);
        }

        courseNumbers.push_back(tokens[0]);
        lines.push_back(tokens);
    }
    file.close();

    if (lines.empty()) {
        cout << "Error: file contains no course data" << endl;
        return false;
    }

    // Second pass: every prerequisite must be a course that is in the file
    for (const vector<string>& tokens : lines) {
        for (size_t i = 2; i < tokens.size(); ++i) {
            if (find(courseNumbers.begin(), courseNumbers.end(), tokens[i]) ==
                courseNumbers.end()) {
                cout << "Error: prerequisite " << tokens[i]
                     << " not found in file" << endl;
                return false;
            }
        }
    }

    // File is valid: build one Course per line and store it in the tree.
    // The old data is only cleared now, so a bad file never wipes a good load.
    tree.Clear();
    for (const vector<string>& tokens : lines) {
        Course course;
        course.courseNumber = tokens[0];
        course.name = tokens[1];
        for (size_t i = 2; i < tokens.size(); ++i) {
            course.prerequisites.push_back(tokens[i]);
        }
        tree.Insert(course);
    }

    return true;
}

//============================================================================
// Print one course with its prerequisite numbers and titles
//============================================================================
void printCourse(const BinarySearchTree& tree, const string& input) {
    string courseNumber = toUpper(trim(input));
    const Course* course = tree.Search(courseNumber);

    if (course == nullptr) {
        cout << "Course " << courseNumber << " not found." << endl;
        return;
    }

    cout << course->courseNumber << ", " << course->name << endl;

    if (course->prerequisites.empty()) {
        cout << "Prerequisites: None" << endl;
        return;
    }

    cout << "Prerequisites:" << endl;
    for (const string& prereqNumber : course->prerequisites) {
        const Course* prereq = tree.Search(prereqNumber);
        if (prereq != nullptr) {
            cout << "  " << prereq->courseNumber << ", " << prereq->name << endl;
        } else {
            cout << "  " << prereqNumber << endl;
        }
    }
}

//============================================================================
// Menu
//============================================================================
void displayMenu() {
    cout << "1. Load Data Structure." << endl;
    cout << "2. Print Course List." << endl;
    cout << "3. Print Course." << endl;
    cout << "9. Exit" << endl;
    cout << "What would you like to do? ";
}

int main() {
    BinarySearchTree courseTree;
    bool dataLoaded = false;

    cout << "Welcome to the course planner." << endl;

    string choice;
    while (true) {
        displayMenu();
        if (!getline(cin, choice)) {
            break;                      // input ended (Ctrl+D / Ctrl+Z)
        }
        choice = trim(choice);

        if (choice == "1") {
            cout << "Enter the name of the course data file: ";
            string fileName;
            getline(cin, fileName);
            fileName = trim(fileName);

            if (loadCourses(fileName, courseTree)) {
                dataLoaded = true;
                cout << "Course data loaded." << endl;
            }
        } else if (choice == "2") {
            if (!dataLoaded) {
                cout << "Please load the data structure first (option 1)." << endl;
            } else {
                cout << "Here is a sample schedule:" << endl;
                courseTree.PrintAll();
            }
        } else if (choice == "3") {
            if (!dataLoaded) {
                cout << "Please load the data structure first (option 1)." << endl;
            } else {
                cout << "What course do you want to know about? ";
                string courseNumber;
                getline(cin, courseNumber);
                printCourse(courseTree, courseNumber);
            }
        } else if (choice == "9") {
            cout << "Thank you for using the course planner!" << endl;
            break;
        } else {
            cout << choice << " is not a valid option." << endl;
        }
    }

    return 0;
}

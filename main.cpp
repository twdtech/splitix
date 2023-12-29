#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <windows.h>
#include <limits>
#include <conio.h>

using namespace std;
namespace fs = std::filesystem;

// Function to generate a random string of a given length
static string generateRandomString() {
    const string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const int length = 8;

    string randomString;
    for (int i = 0; i < length; ++i) {
        randomString += characters[rand() % characters.length()];
    }

    return randomString;
}

// Function to split a file into parts
static void splitFile(const string& filename, int fileSizeMB) {
    ifstream inputFile(filename, ios::binary | ios::ate);

    // Check if the input file can be opened
    if (!inputFile.is_open()) {
        cerr << "Could not open the file." << endl;
        return;
    }

    // Get the size of the input file and set the file position to the beginning
    streampos fileSize = inputFile.tellg();
    inputFile.seekg(0, ios::beg);

    // Create a folder for the output files with a random name
    string outputFolderPath = "FileSplit-" + generateRandomString();
    fs::create_directory(outputFolderPath);

    // Calculate the number of parts the file will be split into
    int numParts = static_cast<int>(ceil(static_cast<double>(fileSize) / (fileSizeMB * 1024 * 1024)));
    long long partSize = fileSizeMB * 1024 * 1024;

    // Loop through each part and create an output file for it
    for (int i = 0; i < numParts; ++i) {
        // Generate the output file name
        string outputFileName = outputFolderPath + "/" + filename.substr(filename.find_last_of("\\/") + 1) + "_part" + to_string(i + 1);
        ofstream outputFile(outputFileName, ios::binary);

        // Check if the output file can be created
        if (!outputFile.is_open()) {
            cerr << "Could not create the output file." << endl;
            return;
        }

        // Initialize the remaining size to be written and create a buffer for reading/writing
        long long remainingSize = min(partSize, static_cast<long long>(fileSize) - (i * partSize));
        vector<char> buffer(4096);

        // Read from the input file and write to the output file until the remaining size is zero
        while (remainingSize > 0) {
            int bytesRead = min(remainingSize, static_cast<long long>(buffer.size()));
            inputFile.read(buffer.data(), bytesRead);
            outputFile.write(buffer.data(), bytesRead);
            remainingSize -= bytesRead;
        }

        // Close the output file and print a message
        outputFile.close();
        cout << "Part " << i + 1 << " created: " << outputFileName << endl;
    }

    // Close the input file
    inputFile.close();
}

// Function to merge splits back together
static void mergeFiles(const std::string& folderPath, const std::string& outputFilename) {
    std::ofstream outputFile(outputFilename, std::ios::binary);

    if (!outputFile.is_open()) {
        std::cerr << "Unable to create output file!" << std::endl;
        return;
    }

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA((folderPath + "\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Error while searching for files in folder!" << std::endl;
        return;
    }

    long long totalSize = 0;

    // Berechne die Gesamtgröße der Dateien im Ordner
    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            totalSize += findFileData.nFileSizeLow;
            totalSize += findFileData.nFileSizeHigh * (MAXDWORD + 1LL);
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);

    // Zurücksetzen und erneutes Durchlaufen, um die Dateien zu kopieren
    hFind = FindFirstFileA((folderPath + "\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Error while searching for files in folder!" << std::endl;
        return;
    }

    long long copiedSize = 0;

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::ifstream inputFile((folderPath + "\\" + findFileData.cFileName).c_str(), std::ios::binary);

            if (!inputFile.is_open()) {
                std::cerr << "Unable to open inputfile: " << findFileData.cFileName << std::endl;
                FindClose(hFind);
                return;
            }

            // Dateiinhalt in die Ausgabedatei kopieren
            outputFile << inputFile.rdbuf();

            // Fortschrittsanzeige aktualisieren
            copiedSize += findFileData.nFileSizeLow;
            copiedSize += findFileData.nFileSizeHigh * (MAXDWORD + 1LL);
            int progress = static_cast<int>((static_cast<double>(copiedSize) / totalSize) * 100);
            std::cout << "Progress " << progress << "%\r" << std::flush;

            inputFile.close();
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
    outputFile.close();

    std::cout << "Files successfully merged: " << outputFilename << std::endl;
}

// Main function
int main() {
    // Display program information
    cout << "Splitix\n";
    cout << "Version: 1.0\n";
    cout << "Made by github.com/twdtech\n\n";  // Name, version, developer

    // Seed for random number generation
    srand(static_cast<unsigned>(time(nullptr)));

    int choice;

    // Display menu options
    cout << "Choose Option:" << endl;
    cout << "1. Split" << endl;
    cout << "2. Combine" << endl;
    cout << "Option: ";

    // Read user choice
    choice = _getch() - '0';

    // Clear the input buffer
    while (_kbhit()) _getch(); // Clear the input buffer

    if (choice == 1) {
        // Splitting option
        string filename;
        int fileSizeMB;

        cout << "\n\nFilename (works also with path): ";
        getline(cin, filename);

        cout << "Size of each sub-file (in MB): ";
        cin >> fileSizeMB;

        // Call the splitFile function
        splitFile(filename, fileSizeMB);
    }
    else if (choice == 2) {
        // Combining option
        string folderPath;
        string outputFilename;

        cout << "\n\nFolder path for split files: ";
        getline(cin, folderPath);

        cout << "Name of combined file: ";
        getline(cin, outputFilename);

        // Call the mergeFiles function
        mergeFiles(folderPath, outputFilename);
    }
    else {
        // Invalid option
        cout << "\nInvalid option!" << endl;
    }

    // Prompt user to exit
    cout << "\nPress Enter to exit...";
    cin.ignore(); // Ignore remaining new line

    return 0;
}
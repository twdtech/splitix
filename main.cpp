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
#include <commdlg.h>
#include <shlobj.h>

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

// Console colors
static void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// Set the cursor position
static void setCursorPos(COORD pos) {
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

// Get current cursor position
static COORD getCursorPos() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.dwCursorPosition;
}

// Convert unit to bytes
static long long convertSizeToBytes(int size, const string& unit) {
    if (unit == "kB") {
        return static_cast<long long>(size) * 1024;
    }
    else if (unit == "MB") {
        return static_cast<long long>(size) * 1024 * 1024;
    }
    else if (unit == "GB") {
        return static_cast<long long>(size) * 1024 * 1024 * 1024;
    }
    else {
        cerr << "Invalid unit. Please select kB, MB or GB." << endl;
        return -1;
    }
}

// Function to split a file into parts
static void splitFile(const string& filename, long long fileSizeBytes) {
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
    int numParts = static_cast<int>(ceil(static_cast<double>(fileSize) / fileSizeBytes));

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
        long long remainingSize = min(fileSizeBytes, static_cast<long long>(fileSize) - (i * fileSizeBytes));
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

    // Calculate the total size of the files in the folder
    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            totalSize += findFileData.nFileSizeLow;
            totalSize += findFileData.nFileSizeHigh * (MAXDWORD + 1LL);
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);

    // Reset and run through again to copy the files
    hFind = FindFirstFileA((folderPath + "\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Error while searching for files in folder!" << std::endl;
        return;
    }

    long long copiedSize = 0;
    std::cout << "\n";

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::ifstream inputFile((folderPath + "\\" + findFileData.cFileName).c_str(), std::ios::binary);

            if (!inputFile.is_open()) {
                std::cerr << "Unable to open inputfile: " << findFileData.cFileName << std::endl;
                FindClose(hFind);
                return;
            }

            // Copy file content to the output file
            outputFile << inputFile.rdbuf();

            // Update progress bar
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
    HWND consoleWindow = GetConsoleWindow();
    SetWindowText(consoleWindow, L"Splitix V1.0 @ github.com/twdtech");
    // Seed for random number generation
    srand(static_cast<unsigned>(time(nullptr)));

    int choice = 0;
    const int numOptions = 3;
    const char arrow = '>';

    // Display program information
    setColor(FOREGROUND_GREEN);
    cout << "\nSplitix\n";
    cout << "Version: 1.0\n";
    cout << "Made by github.com/twdtech\n\n";

    setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << "Choose Option:" << endl;

    // Save the cursor position before displaying the menu options
    COORD menuPos = getCursorPos();

    // Display menu options
    while (true) {
        // Reset the cursor position to the saved position
        setCursorPos(menuPos);

        for (int i = 0; i < numOptions; ++i) {
            if (i == choice) {
                cout << arrow << " ";
            }
            else {
                cout << "  ";
            }
            switch (i) {
            case 0:
                cout << "1. Split" << endl;
                break;
            case 1:
                cout << "2. Combine" << endl;
                break;
            case 2:
                cout << "3. Exit" << endl;
                break;
            }
        }

        // Read user input
        int key = _getch();
        if (key == 224) { // Arrow key was pressed
            switch (_getch()) {
            case 72: // a.up
                if (choice > 0) --choice;
                break;
            case 80: // a.down
                if (choice < numOptions - 1) ++choice;
                break;
            }
        }
        else if (key == '\r') {
            break;
        }
    }

    if (choice == 0) {
        // Splittingo ption
        string filename;
        int fileSize;
        string unit;

        // Show file open dialog
        OPENFILENAME ofn;
        WCHAR szFile[260] = L"";

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn) == TRUE) {
            char narrow[260];
            WideCharToMultiByte(CP_ACP, 0, ofn.lpstrFile, -1, narrow, sizeof(narrow), NULL, NULL);
            filename = narrow;
        }

        setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        cout << "\nChosen file: " << filename << endl;

        cout << "Size of each sub-file: ";
        cin >> fileSize;

        cout << "Unit: \n";

        // Save the cursor position before displaying the unit options
        COORD unitPos = getCursorPos();

        long long fileSizeBytes = -1;
        while (fileSizeBytes == -1) {
            // Units
            string units[] = { "kB", "MB", "GB" };
            int unitChoice = 0;

            while (true) {
                // Reset the cursor position to the saved position
                setCursorPos(unitPos);

                for (int i = 0; i < 3; ++i) {
                    if (i == unitChoice) {
                        cout << arrow << " ";
                    }
                    else {
                        cout << "  ";
                    }
                    cout << units[i] << endl;
                }

                int key = _getch();
                if (key == 224) { 
                    switch (_getch()) {
                    case 72:
                        if (unitChoice > 0) --unitChoice;
                        break;
                    case 80:
                        if (unitChoice < 2) ++unitChoice;
                        break;
                    }
                }
                else if (key == '\r') {
                    break;
                }
            }

            unit = units[unitChoice];

            fileSizeBytes = convertSizeToBytes(fileSize, unit);
            if (fileSizeBytes == -1) {
                cout << "Ungültige Einheit. Bitte wählen Sie kB, MB oder GB." << endl;
            }
        }
        std::cout << "\n";

        // Call splitFile function
        splitFile(filename, fileSizeBytes);

        cout << "\nPress Enter to exit...";
        while (true) {
            if (_kbhit()) {
                char ch = _getch();
                if (ch == '\r' || ch == '\n') {
                    break;
                }
            }
        }
    }
    else if (choice == 1) {
        // Combining option
        string folderPath;
        string outputFilename;

        setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);

        // Show folder selection dialog
        BROWSEINFO bi = { 0 };
        bi.lpszTitle = L"Select folder with splits:";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        bi.lpfn = NULL;
        bi.lParam = 0;
        bi.iImage = 0;

        // Save the current working directory
        TCHAR currentDirectory[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, currentDirectory);

        // Set the working directory to the current directory
        SetCurrentDirectory(currentDirectory);

        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (pidl != 0) {
            // Read folder path
            WCHAR path[MAX_PATH];
            if (SHGetPathFromIDList(pidl, path)) {
                char narrow[260];
                WideCharToMultiByte(CP_ACP, 0, path, -1, narrow, sizeof(narrow), NULL, NULL);
                folderPath = narrow;
            }

            // Release memory
            IMalloc* imalloc = 0;
            if (SUCCEEDED(SHGetMalloc(&imalloc))) {
                imalloc->Free(pidl);
                imalloc->Release();
            }
        }

        // Reset the working directory to the original directory
        SetCurrentDirectory(currentDirectory);

        cout << "\nChosen folder: " << folderPath << endl;

        cout << "Name of combined file: ";
        getline(cin, outputFilename);

        // Call mergeFiles function
        mergeFiles(folderPath, outputFilename);
    }
    else if (choice == 2) {
        // Exit option
        cout << "Exiting Splitix..." << endl;
        return 0;
}
    else {
        // Invalid option
        setColor(FOREGROUND_RED);
        cout << "\nInvalid option!" << endl;
    }

    // Prompt user to exit
    setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << "\nPress Enter to exit...";
    cin.ignore();

    return 0;
}
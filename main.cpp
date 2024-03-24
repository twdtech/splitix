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
#include <map>
#include <comdef.h>
#include <iomanip>

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

static void printProgress(double percentage) {
    const int progressBarWidth = 70;
    setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    cout << "   [";
    int pos = progressBarWidth * percentage;
    for (int i = 0; i < progressBarWidth; ++i) {
        if (i < pos) cout << "=";
        else if (i == pos) cout << ">";
        else cout << " ";
    }
    cout << "] " << int(percentage * 100.0) << "%  \r";
    setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout.flush();
}

static string caesarEncrypt(string text, int s) {
    string result = "";

    for (int i = 0; i < text.length(); i++) {
        if (isupper(text[i]))
            result += char(int(text[i] + s - 65) % 26 + 65);
        else if (islower(text[i]))
            result += char(int(text[i] + s - 97) % 26 + 97);
        else
            result += text[i];
    }

    return result;
}

static string caesarDecrypt(string text, int s) {
    string result = "";

    for (int i = 0; i < text.length(); i++) {
        if (isupper(text[i]))
            result += char(int(text[i] - s - 65 + 26) % 26 + 65);
        else if (islower(text[i]))
            result += char(int(text[i] - s - 97 + 26) % 26 + 97);
        else
            result += text[i];
    }

    return result;
}

// Function to split a file into parts
static void splitFile(const string& filename, long long fileSizeBytes) {
    ifstream inputFile(filename, ios::binary | ios::ate);

    if (!inputFile.is_open()) {
        setColor(FOREGROUND_RED);
        cerr << "   Could not open the file." << endl;
        setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        return;
    }

    streampos fileSize = inputFile.tellg();
    inputFile.seekg(0, ios::beg);

    // Progressbar if filesize bigger than 1024MB
    bool showProgressBar = fileSize > 1024 * 1024 * 1024;

    string outputFolderPath;
    while (outputFolderPath.empty()) {
        BROWSEINFO bi = { 0 };
        bi.lpszTitle = L"Select folder with splits:";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        bi.lpfn = NULL;
        bi.lParam = 0;
        bi.iImage = 0;

        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (pidl != 0) {
            WCHAR path[MAX_PATH];
            if (SHGetPathFromIDList(pidl, path)) {
                _bstr_t bstrPath(path);
                outputFolderPath = (char*)bstrPath;
            }

            IMalloc* imalloc = 0;
            if (SUCCEEDED(SHGetMalloc(&imalloc))) {
                imalloc->Free(pidl);
                imalloc->Release();
            }
        }
        if (outputFolderPath.empty()) {
            setColor(FOREGROUND_RED);
            cerr << "   No folder selected. Please select a folder." << endl;
            setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        }
    }

    outputFolderPath += "/FileSplit-" + generateRandomString();
    fs::create_directory(outputFolderPath);

    int numParts = static_cast<int>(ceil(static_cast<double>(fileSize) / fileSizeBytes));

    double lastTotalProgress = 0.0;
    double lastSplitProgress = 0.0;
    COORD initialPos = getCursorPos();
    long long totalSize = static_cast<long long>(fileSize);
    long long processedSize = 0;
    for (int i = 0; i < numParts; ++i) {
        string outputFileName = outputFolderPath + "/" + filename.substr(filename.find_last_of("\\/") + 1) + "_split" + to_string(i + 1);
        ofstream outputFile(outputFileName, ios::binary);
        long long splitSize = min(fileSizeBytes, totalSize);
        long long remainingSize = splitSize;

        if (!outputFile.is_open()) {
            setColor(FOREGROUND_RED);
            cerr << "   Could not create the output file." << endl;
            setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            return;
        }

        string partInfo = "Part: " + to_string(i + 1);
        string encryptedPartInfo = caesarEncrypt(partInfo, 3);  // Verschlüsseln Sie die Informationen
        outputFile << encryptedPartInfo << endl;

        vector<char> buffer(4096);

        while (remainingSize > 0) {
            int bytesRead = min(remainingSize, static_cast<long long>(buffer.size()));
            inputFile.read(buffer.data(), bytesRead);
            outputFile.write(buffer.data(), bytesRead);
            remainingSize -= bytesRead;
            totalSize -= bytesRead;
            processedSize += bytesRead;

            if (showProgressBar) {
                double totalProgress = (double)processedSize / fileSize;
                double splitProgress = (double)(splitSize - remainingSize) / splitSize;
                if (totalProgress - lastTotalProgress >= 0.01 || splitProgress - lastSplitProgress >= 0.01) {
                    setCursorPos(initialPos);
                    setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
                    cout << "   Split: ";
                    setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    printProgress(splitProgress);
                    cout << "\n";
                    setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
                    cout << "   Total: ";
                    setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    printProgress(totalProgress);
                    cout << "\n\n";
                    lastTotalProgress = totalProgress;
                    lastSplitProgress = splitProgress;
                }
            }
        }

        if (showProgressBar) {
            setCursorPos(initialPos);
            setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
            cout << "   Split: ";
            setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            printProgress(1.0);
            cout << "\n";
            setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
            cout << "   Total: ";
            setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            printProgress((double)processedSize / fileSize);
            cout << "\n\n";
        }

        outputFile.close();
        setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::cout << "\r   Part " << i + 1 << " created: " << outputFileName;
        setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }

    if (showProgressBar) {
        setCursorPos(initialPos);
        setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        cout << "   Split: ";
        setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printProgress(1.0); 
        cout << "\n";
        setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        cout << "   Total: ";
        setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printProgress(1.0);
        cout << "\n\n";
    }

    inputFile.close();
}

// Function to merge splits back together
static void mergeFiles(const std::string& folderPath, const std::string& outputFilename) {
    string outputFolderPath;
    while (outputFolderPath.empty()) {
        BROWSEINFO bi = { 0 };
        bi.lpszTitle = L"Select folder for the merged file:";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        bi.lpfn = NULL;
        bi.lParam = 0;
        bi.iImage = 0;

        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (pidl != 0) {
            WCHAR path[MAX_PATH];
            if (SHGetPathFromIDList(pidl, path)) {
                _bstr_t bstrPath(path);
                outputFolderPath = (char*)bstrPath;
            }

            IMalloc* imalloc = 0;
            if (SUCCEEDED(SHGetMalloc(&imalloc))) {
                imalloc->Free(pidl);
                imalloc->Release();
            }
        }
        if (outputFolderPath.empty()) {
            cerr << "   No folder selected. Please select a folder." << endl;
        }
    }

    std::ofstream outputFile(outputFolderPath + "/" + outputFilename, std::ios::binary);

    if (!outputFile.is_open()) {
        std::cerr << "   Unable to create output file!" << std::endl;
        return;
    }

    std::map<int, std::ifstream> fileParts;

    cout << "\n";
    printProgress(0.0);

    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        std::ifstream inputFile(entry.path(), std::ios::binary);
        if (!inputFile.is_open()) {
            std::cerr << "   Unable to open input file: " << entry.path() << std::endl;
            return;
        }

        std::string encryptedPartInfo;
        std::getline(inputFile, encryptedPartInfo);
        string decryptedPartInfo = caesarDecrypt(encryptedPartInfo, 3);  // Entschlüsseln Sie die Informationen
        int partNumber = std::stoi(decryptedPartInfo.substr(6));

        fileParts[partNumber] = std::move(inputFile);
    }

    size_t totalParts = fileParts.size();
    size_t currentPart = 0;

    for (auto& [partNumber, inputFile] : fileParts) {
        outputFile << inputFile.rdbuf();
        inputFile.close();

        // Update progress bar
        printProgress(static_cast<double>(++currentPart) / totalParts);
    }

    outputFile.close();
    std::cout << "\n\n   Files successfully merged: " << outputFolderPath + "/" + outputFilename << std::endl;
}

// Main function
int main() {
    HWND consoleWindow = GetConsoleWindow();
    SetWindowText(consoleWindow, L"Splitix V1.2.1 @ github.com/twdtech");
    // Seed for random number generation
    srand(static_cast<unsigned>(time(nullptr)));

    int choice = 0;
    const int numOptions = 3;
    const char arrow = '>';

    // Display program information
    setColor(FOREGROUND_GREEN);
    std::string prgrm_nme = R"(
   _____/\\\\\\\\\\\__________________/\\\\\\____________________________________________________
   ____/\\\/////////\\\_______________\////\\\___________________________________________________
   ____\//\\\______\///____/\\\\\\\\\_____\/\\\_____/\\\_____/\\\_______/\\\_____________________
   ______\////\\\__________/\\\/////\\\____\/\\\____\///___/\\\\\\\\\\\_\///___/\\\____/\\\______
   __________\////\\\______\/\\\\\\\\\\_____\/\\\_____/\\\_\////\\\////___/\\\_\///\\\/\\\/______
   ______________\////\\\___\/\\\//////______\/\\\____\/\\\____\/\\\______\/\\\___\///\\\/_______
   ________/\\\______\//\\\__\/\\\____________\/\\\____\/\\\____\/\\\_/\\__\/\\\____/\\\/\\\_____
   ________\///\\\\\\\\\\\/___\/\\\__________/\\\\\\\\\_\/\\\____\//\\\\\___\/\\\__/\\\/\///\\\__
   ___________\///////////_____\///__________\/////////__\///______\/////____\///__\///____\///__
    )";
    std::cout << prgrm_nme;
    // cout << "\nSplitix\n";
    cout << "\n   Version: 1.2.1\n";
    cout << "   Made by github.com/twdtech\n\n";

    setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << "   Choose Option:" << endl;

    // Save the cursor position before displaying the menu options
    COORD menuPos = getCursorPos();

    // Display menu options
    while (true) {
        // Reset the cursor position to the saved position
        setCursorPos(menuPos);

        for (int i = 0; i < numOptions; ++i) {
            if (i == choice) {
                cout << "   " << arrow << " ";
            }
            else {
                cout << "     ";
            }
            switch (i) {
            case 0:
                cout << "1. Split " << endl;
                break;
            case 1:
                cout << "2. Combine " << endl;
                break;
            case 2:
                cout << "3. Exit " << endl;
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
        // Splitting option
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

        while (filename.empty()) {
            if (GetOpenFileName(&ofn) == TRUE) {
                char narrow[260];
                WideCharToMultiByte(CP_ACP, 0, ofn.lpstrFile, -1, narrow, sizeof(narrow), NULL, NULL);
                filename = narrow;
            }
            else {
                cout << "Please choose a file!" << endl;
            }
        }

        setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        cout << "\n   Chosen file: " << filename << endl;

        cout << "\n   Size of each sub-file: ";
        cin >> fileSize;

        cout << "\n   Unit: \n";

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
                        cout << "   " << arrow << " ";
                    }
                    else {
                        cout << "     ";
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
                setColor(FOREGROUND_RED);
                cout << "   Invalid unit. Please select kB, MB or GB." << endl;
                setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            }
        }
        std::cout << "\n";

        // Call splitFile function
        splitFile(filename, fileSizeBytes);

        cout << "\n\n   Press Enter to exit...";
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

        while (folderPath.empty()) {
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != 0) {
                // Read folder path
                WCHAR path[MAX_PATH];
                if (SHGetPathFromIDList(pidl, path)) {
                    char narrow[260];
                    WideCharToMultiByte(CP_ACP, 0, path, -1, narrow, sizeof(narrow), NULL, NULL);
                    folderPath = narrow;
                }
                else {
                    cout << "Please choose a folder!" << endl;
                }

                // Release memory
                IMalloc* imalloc = 0;
                if (SUCCEEDED(SHGetMalloc(&imalloc))) {
                    imalloc->Free(pidl);
                    imalloc->Release();
                }
            }
        }

        // Reset the working directory to the original directory
        SetCurrentDirectory(currentDirectory);

        cout << "\n   Chosen folder: " << folderPath << endl;

        cout << "   Name of combined file (leave empty to use the name of the first split): ";
        getline(cin, outputFilename);

        if (outputFilename.empty()) {
            // Get the name of the first split file
            for (const auto& entry : fs::directory_iterator(folderPath)) {
                outputFilename = entry.path().filename().string();
                size_t pos = outputFilename.find("_split");
                if (pos != string::npos) {
                    outputFilename = outputFilename.substr(0, pos);
                    break;
                }
            }
        }

        cout << "   Combined file will be named: " << outputFilename << endl;

        // Call mergeFiles function
        mergeFiles(folderPath, outputFilename);
    }
    else if (choice == 2) {
        // Exit option
        cout << "   Exiting Splitix..." << endl;
        return 0;
    }
    else {
        // Invalid option
        setColor(FOREGROUND_RED);
        cout << "\n   Invalid option!" << endl;
        setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }

    // Prompt user to exit
    setColor(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << "\n\n   Press Enter to exit...";
    cin.ignore();

    return 0;
}
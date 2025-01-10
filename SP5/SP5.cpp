
#include "framework.h"
#include "SP5.h"
#include <queue>
#include <commdlg.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <bitset>
#include <iterator>
#include <chrono>
#include "ThreadPool.h"
#define MAX_LOADSTRING 100
#define ID_BUTTON_ENCODE 1001
#define ID_BUTTON_DECODE 1002
#define THREADS_NUMBER 12

HWND hLabel;

struct Node
{
    char ch;
    int freq;
    Node* left;
    Node* right;

    Node(char c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
};

struct Compare
{
    bool operator()(Node* l, Node* r) { return l->freq > r->freq; }
};

Node* BuildHuffmanTree(const std::map<char, int>& freqMap)
{
    std::priority_queue<Node*, std::vector<Node*>, Compare> pq;
    for (const auto& pair : freqMap)
    {
        pq.push(new Node(pair.first, pair.second));
    }

    while (pq.size() > 1)
    {
        Node* left = pq.top(); pq.pop();
        Node* right = pq.top(); pq.pop();
        Node* parent = new Node('\0', left->freq + right->freq);
        parent->left = left;
        parent->right = right;
        pq.push(parent);
    }

    return pq.top();
}

void GenerateHuffmanCodes(Node* root, const std::string& prefix,
    std::map<char, std::string>& codes)
{
    if (!root) return;
    if (!root->left && !root->right)
    {
        codes[root->ch] = prefix;
    }
    GenerateHuffmanCodes(root->left, prefix + "0", codes);
    GenerateHuffmanCodes(root->right, prefix + "1", codes);
}

void FreeHuffmanTree(Node* root)
{
    if (!root) return;
    FreeHuffmanTree(root->left);
    FreeHuffmanTree(root->right);
    delete root;
}


std::vector<uint8_t> EncodeTextToBinary(const std::string& text,
    const std::map<char, std::string>& huffmanCodes) {
    std::vector<uint8_t> binaryData;
    std::string bitString;

    for (char ch : text) {
        bitString += huffmanCodes.at(ch);
    }

    for (size_t i = 0; i < bitString.size(); i += 8) {
        std::string byteStr = bitString.substr(i, 8);
        while (byteStr.size() < 8) {
            byteStr += '0'; 
        }
        uint8_t byte = std::bitset<8>(byteStr).to_ulong();
        binaryData.push_back(byte);
    }

    return binaryData;
}

void writeBinaryFile(const std::string& filename, const std::vector<uint8_t>& binaryData) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        throw std::runtime_error("Ошибка открытия файла для записи.");
    }
    outFile.write(reinterpret_cast<const char*>(binaryData.data()), binaryData.size());
    outFile.close();
}

std::vector<uint8_t> readBinaryFile(const std::string& filename, std::map<char, int>& freqMap) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile) {
        throw std::runtime_error("Ошибка открытия файла для чтения.");
    }

    uint32_t mapSize = 0;
    inFile.read(reinterpret_cast<char*>(&mapSize), sizeof(uint32_t));

    for (size_t i = 0; i < mapSize; ++i) {
        char ch;
        int freq;
        inFile.read(&ch, sizeof(char));
        inFile.read(reinterpret_cast<char*>(&freq), sizeof(int));
        freqMap[ch] = freq;
    }

    std::vector<uint8_t> binaryData((std::istreambuf_iterator<char>(inFile)),
        std::istreambuf_iterator<char>());

    inFile.close();
    return binaryData;
}


std::string decodeBinaryToText(const std::vector<uint8_t>& binaryData, Node* root) {
    std::string decodedText;
    Node* currentNode = root;

    for (uint8_t byte : binaryData) {
        for (int i = 7; i >= 0; --i) {  
            bool bit = (byte >> i) & 1; 
            currentNode = (bit == 0) ? currentNode->left : currentNode->right;
            
            if (!currentNode->left && !currentNode->right) {
                decodedText += currentNode->ch;
                currentNode = root;  
            }
        }
    }

    return decodedText;
}

void SaveEncodedFileWithFreqMap(const std::string& filename, const std::vector<uint8_t>& encodedData,
    const std::map<char, int>& freqMap)
{
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile)
    {
        throw std::runtime_error("Ошибка записи в файл.");
    }

    uint32_t mapSize = freqMap.size();
    outFile.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));

    for (const auto& pair : freqMap)
    {
        outFile.write(reinterpret_cast<const char*>(&pair.first), sizeof(pair.first)); 
        outFile.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second)); 
    }

    outFile.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());

    outFile.close();
}

std::vector<uint8_t> EncodeHuffman(const std::string& text, Node* root)
{

    std::map<char, std::string> huffmanCodes;
    GenerateHuffmanCodes(root, "", huffmanCodes);

    std::vector<uint8_t> encodedData = EncodeTextToBinary(text, huffmanCodes);

    return encodedData;
}

std::map<char, int> ReadFreqMapFromFile(const std::string& filename)
{
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile)
    {
        throw std::runtime_error("Ошибка открытия файла для чтения.");
    }

    uint32_t mapSize;
    inFile.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));

    std::map<char, int> freqMap;

    for (uint32_t i = 0; i < mapSize; ++i)
    {
        char symbol;
        int frequency;
        inFile.read(reinterpret_cast<char*>(&symbol), sizeof(symbol));
        inFile.read(reinterpret_cast<char*>(&frequency), sizeof(frequency));
        freqMap[symbol] = frequency;
    }

    inFile.close();

    return freqMap;
}

void removeCarriageReturns(std::string& str) {
    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
}



std::vector<std::string> splitFileIntoParts(const std::string& fileContent, int numParts) {
    int partSize = fileContent.size() / numParts;
    int remainder = fileContent.size() % numParts;

    std::vector<std::string> parts;
    size_t start = 0;

    for (int i = 0; i < numParts; ++i) {
        size_t end = start + partSize + (i < remainder ? 1 : 0);
        parts.push_back(fileContent.substr(start, end - start));
        start = end;
    }

    return parts;
}


void OpenAndEncodeFile(HWND hWnd, ThreadPool<std::vector<uint8_t>>& tp)
{
    OPENFILENAME ofn;       
    WCHAR szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"All Files\0*.*\0Text Files\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE)
    {
        std::ifstream inputFile(szFile, std::ios::binary);
        if (!inputFile)
        {
            MessageBox(hWnd, L"Ошибка при открытии файла.", L"Ошибка", MB_OK | MB_ICONERROR);
            return;
        }

        std::string fileContent((std::istreambuf_iterator<char>(inputFile)),
            std::istreambuf_iterator<char>());
        inputFile.close();
        std::vector<std::string> parts = splitFileIntoParts(fileContent, tp.getNumberOfThreads());
        

        using namespace std::chrono;
        auto start_time = high_resolution_clock::now();
        //removeCarriageReturns(fileContent);
        std::map<char, int> freqMap;
        for (char ch : fileContent) freqMap[ch]++;

        Node* root = BuildHuffmanTree(freqMap);
        auto end_time = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end_time - start_time).count();


        start_time = high_resolution_clock::now();
        std::vector<std::future<std::vector<uint8_t>>> futures;

        std::vector<uint8_t> result;

        for (const auto& part : parts) {

            futures.emplace_back(tp.add_task([&]() -> std::vector<uint8_t> { return EncodeHuffman(part, root); }));

        }

        for (auto& future : futures) {
            try {
                auto vec = future.get();
                std::copy(vec.begin(), vec.end(), std::back_inserter(result));
            }
            catch (const std::exception& e) {
                std::string errorMessage = e.what();
                std::wstring wideErrorMessage(errorMessage.begin(), errorMessage.end()); 

                MessageBox(hWnd, wideErrorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);
            }
        }
        FreeHuffmanTree(root);
        SaveEncodedFileWithFreqMap("encoded_file.bin", result, freqMap);

        end_time = high_resolution_clock::now();
        auto duration2 = duration_cast<milliseconds>(end_time - start_time).count();
        std::wstring temp = std::to_wstring(duration2) + L" + " + std::to_wstring(duration);
        SetWindowText(hLabel, temp.c_str());
        MessageBox(hWnd, L"File successfully decoded", L"Success", MB_OK | MB_ICONINFORMATION);
    }
}

std::string WideCharToString(const WCHAR* wideStr)
{
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    if (bufferSize == 0)
    {
        throw std::runtime_error("Cannot cast WCHAR to std::string");
    }

    std::string str(bufferSize - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &str[0], bufferSize - 1, nullptr, nullptr);
    return str;
}

std::vector<std::vector<uint8_t>> splitDataIntoParts(const std::vector<uint8_t>& data, size_t n) {
    std::vector<std::vector<uint8_t>> parts;
    size_t part_size = data.size() / n;
    size_t remainder = data.size() % n;
    size_t start = 0;

    for (size_t i = 0; i < n; ++i) {
        size_t end = start + part_size + (i < remainder ? 1 : 0);
        parts.push_back(std::vector<uint8_t>(data.begin() + start, data.begin() + end));
        start = end;
    }

    return parts;
}

void OpenAndDecodeFile(HWND hWnd, ThreadPool<std::string>& tp)
{
    OPENFILENAME ofn;
    WCHAR szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"All Files\0*.*\0Binary Files\0*.BIN\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE)
    {

            using namespace std::chrono;
            auto start_time = high_resolution_clock::now();
            std::string filePath = WideCharToString(szFile);
            std::map<char, int> freqMap;
            auto encodedData = readBinaryFile(filePath, freqMap);
            std::vector<std::vector<uint8_t>> parts = splitDataIntoParts(encodedData, tp.getNumberOfThreads());
            Node* root = BuildHuffmanTree(freqMap);
            auto end_time = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end_time - start_time).count();

            start_time = high_resolution_clock::now();
            std::vector<std::future<std::string>> futures;

            std::string result;

            for (const auto& part : parts) {

                futures.emplace_back(tp.add_task([&]() -> std::string { return decodeBinaryToText(part, root); }));

            }

            for (auto& future : futures) {
                try {
                    auto vec = future.get();
                    result += vec;
                }
                catch (const std::exception& e) {
                    std::string errorMessage = e.what();
                    std::wstring wideErrorMessage(errorMessage.begin(), errorMessage.end());

                    MessageBox(hWnd, wideErrorMessage.c_str(), L"Error", MB_OK | MB_ICONERROR);
                }
            }

            FreeHuffmanTree(root);

            std::ofstream outFile("decoded_output.txt");

            outFile << result;
            outFile.close();

            end_time = high_resolution_clock::now();
            auto duration2 = duration_cast<milliseconds>(end_time - start_time).count();
            std::wstring temp = std::to_wstring(duration2) + L" + " + std::to_wstring(duration);
            SetWindowText(hLabel, temp.c_str());
            MessageBox(hWnd, L"File successfully decoded", L"Success", MB_OK | MB_ICONINFORMATION);
    }
}



HINSTANCE hInst;                               
WCHAR szTitle[MAX_LOADSTRING];                
WCHAR szWindowClass[MAX_LOADSTRING];         

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SP5, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SP5));

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SP5));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SP5);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; 

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 280, 300, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{


    static ThreadPool<std::vector<uint8_t>> tp(THREADS_NUMBER);

    static ThreadPool<std::string> tpDec(THREADS_NUMBER);

    switch (message) {
    case WM_CREATE: {

        CreateWindow(L"BUTTON", L"Encode",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            50, 50, 150, 30, hWnd, (HMENU)ID_BUTTON_ENCODE, nullptr, nullptr);

        CreateWindow(L"BUTTON", L"Decode",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            50, 100, 150, 30, hWnd, (HMENU)ID_BUTTON_DECODE, nullptr, nullptr);

        CreateWindow(L"STATIC", L"Time",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            50, 150, 150, 30, hWnd, (HMENU)1, nullptr, nullptr);
        hLabel = CreateWindow(L"STATIC", L"",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            50, 200, 150, 30, hWnd, (HMENU)1, nullptr, nullptr);
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_BUTTON_ENCODE:
            OpenAndEncodeFile(hWnd,tp);
            break;
        case ID_BUTTON_DECODE:
            OpenAndDecodeFile(hWnd, tpDec);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

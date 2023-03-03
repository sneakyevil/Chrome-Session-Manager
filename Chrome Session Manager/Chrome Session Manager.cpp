#include "Includes.hpp"
#include "Console.hpp"
#include "SessionManager.hpp"

namespace Program
{
    const char* m_pName = "Chrome Session Manager";

    void Info()
    {
        Console::Clear();
        Console::Print(Console::m_uDefaultColor, '!', m_pName); Console::Print(Console::m_uDefaultColor, " made by "); Console::Print(CLR_BRED, "sneakyevil\n");
        Console::Print(Console::m_uDefaultColor, '!', "Saved sessions: ");  Console::Print(CLR_BYELLOW, std::to_string(SessionManager::m_vList.size()) + "\n\n");
    }

    int Error(std::string m_sError)
    {
        Info();
        Console::Print(CLR_BRED, '!', m_sError); _getch();
        return EXIT_FAILURE;
    }

    int GetSelectionIndex(std::string m_sText, std::string* m_pList, int m_iSize, int m_iDeltaShow, std::string m_sPrefix)
    {
        int m_iIndex = 0;
        bool m_bUpdate = true;

        while (1)
        {
            if (m_bUpdate)
            {
                if (0 > m_iDeltaShow || m_iDeltaShow >= m_iSize) m_iDeltaShow = m_iSize - 1;

                if (0 > m_iIndex) m_iIndex = m_iSize - 1;
                else if (m_iIndex >= m_iSize) m_iIndex = 0;

                m_bUpdate = false;

                Program::Info();
                Console::Print(Console::m_uDefaultColor, '!', m_sText);
                Console::DrawSelection(m_iIndex, m_pList, m_iSize, m_iDeltaShow, "\t");
            }

            int m_iKey = _getch();
            switch (m_iKey)
            {
            case 8: return -1;
            case 13: return m_iIndex;
            case 72:
            {
                m_iIndex--;
                m_bUpdate = true;
            }
            break;
            case 80:
            {
                m_iIndex++;
                m_bUpdate = true;
            }
            break;
            }
        }
    }

    std::vector<std::string> GetFormattedSessionList()
    {
        std::vector<std::string> m_vReturn;
        m_vReturn.emplace_back("< Start saved sessions by sub name >");

        int m_iCount = 1;
        for (std::string& SessionName : SessionManager::m_vList)
        {
            m_vReturn.emplace_back(std::to_string(m_iCount) + ". " + SessionName);
            ++m_iCount;
        }

        return m_vReturn;
    }

    std::string GetClipboardText()
    {
        std::string m_sText = "";

        if (OpenClipboard(0))
        {
            HANDLE m_hData = GetClipboardData(CF_TEXT);
            if (m_hData)
            {
                char* m_pText = static_cast<char*>(GlobalLock(m_hData));
                if (m_pText)
                    m_sText = m_pText;

                GlobalUnlock(m_hData);
            }

            CloseClipboard();
        }

        return m_sText;
    }
}

namespace Chrome
{
    std::string m_ExecutablePath = "";
    bool m_IsChromium = false;

    bool Init()
    {
        wchar_t* m_LocalAppData = nullptr;
        HRESULT m_Res = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, 0, &m_LocalAppData);
        if (m_Res == S_OK)
        {
            std::string m_ChromiumPath(MAX_PATH, '\0');
            m_ChromiumPath.resize(sprintf_s(&m_ChromiumPath[0], m_ChromiumPath.size(), "%ws\\Chromium\\Application\\chrome.exe", m_LocalAppData));
            CoTaskMemFree(m_LocalAppData);

            struct stat m_ChromiumStat;
            if (stat(&m_ChromiumPath[0], &m_ChromiumStat) == 0)
            {
                m_ExecutablePath = m_ChromiumPath;
                m_IsChromium = true;
                return true;
            }
        }

        m_ExecutablePath = Utils::Registry::GetString(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe", "");
        if (m_ExecutablePath.empty() || m_ExecutablePath.find(".exe") == std::string::npos)
            return false; // Check just in-case registry fvcked up

        return true;
    }

    void NewProcess(std::string m_sArgs = "")
    {
        std::string m_CommandLine = m_ExecutablePath + " " + m_sArgs;

        STARTUPINFO m_StartupInfo;
        ZeroMemory(&m_StartupInfo, sizeof(m_StartupInfo));
        m_StartupInfo.cb = sizeof(STARTUPINFO);

        PROCESS_INFORMATION m_ProcessInfo;

        if (!CreateProcessA(0, &m_CommandLine[0], 0, 0, 0, 0, 0, 0, &m_StartupInfo, &m_ProcessInfo))
            return;

        CloseHandle(m_ProcessInfo.hProcess);
        CloseHandle(m_ProcessInfo.hThread);
    }

    void RemoveSwReporter(std::string m_SessionFolder)
    {
        if (m_IsChromium)
            return;

        std::string m_SwReporterPath(m_SessionFolder + "\\SwReporter\\*");
        WIN32_FIND_DATA m_FindData = { 0 };

        HANDLE m_Find = FindFirstFileA(&m_SwReporterPath[0], &m_FindData);
        if (m_Find != INVALID_HANDLE_VALUE)
        {
            while (FindNextFileA(m_Find, &m_FindData) != 0)
            {
                if (!(m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    continue;

                if (!isdigit(m_FindData.cFileName[0]))
                    continue;

                std::string m_SwReporterExeFile(m_SessionFolder + "\\SwReporter\\" + m_FindData.cFileName + "\\software_reporter_tool.exe");
                remove(&m_SwReporterExeFile[0]);
            }

            FindClose(m_Find);
        }
    }

    void LaunchSession(std::string m_sName)
    {
        std::string m_sArgs = "";
        auto AddArgument = [&m_sArgs](std::string m_sArg) { m_sArgs += (m_sArgs.empty() ? m_sArg : " " + m_sArg); };

        // Custom arguments
        {
            if (!m_IsChromium)
                AddArgument("--simulate-outdated-no-au=\"Tue, 31 Dec 2099 23:59 : 59 GMT\""); // Don't update

            if (!SessionManager::m_sArgs.empty())
                AddArgument(SessionManager::m_sArgs);
        }

        std::string m_SessionFolder(MAX_PATH, '\0');
        m_SessionFolder.resize(GetCurrentDirectoryA(m_SessionFolder.size(), &m_SessionFolder[0]));
        m_SessionFolder += "\\" + SessionManager::GetPath(m_sName);
    
        AddArgument("--user-data-dir=\"" + m_SessionFolder + "\"");

        NewProcess(m_sArgs);
        RemoveSwReporter(m_SessionFolder);
    }
}

int main()
{
    Console::Init(Program::m_pName);
    SetConsoleOutputCP(437); // Required for arrow draw...

    if (!Chrome::Init())
        return Program::Error("Couldn't fetch Chrome informations.");

    SessionManager::Init();

    while (1)
    {
        static std::string m_sOptions[] =
        {
            "Select Session", "New Session", "Delete Session", "Exit"
        };
        static constexpr int m_iOptionSize = (sizeof(m_sOptions) / sizeof(m_sOptions[0]));
        static constexpr int m_iExitOption = m_iOptionSize - 1;

        int m_iMainOption = Program::GetSelectionIndex("Select Option:\n", m_sOptions, m_iOptionSize, -1, "\t");

        switch (m_iMainOption)
        {
            // Select Session
            case 0:
            {
                if (SessionManager::m_vList.empty())
                    Program::Error("You need to first add session.");
                else
                {
                    std::vector<std::string> m_vSessionList = Program::GetFormattedSessionList();
                    int m_iSelectedSession = Program::GetSelectionIndex("Select Session:\n", m_vSessionList.data(), m_vSessionList.size(), 5, "\t");
                    switch (m_iSelectedSession)
                    {
                        case 0:
                        {
                            Program::Info();
                            Console::Print(Console::m_uDefaultColor, '!', "Session sub name (empty = all, '-' = back): ");

                            std::string m_sSubName;
                            std::getline(std::cin, m_sSubName);
                            if (m_sSubName == "-") break;

                            Console::Print(Console::m_uDefaultColor, '!', "Launch Args (empty = default, '-c' = clipboard, '-' = back): ");

                            std::string m_sArgs;
                            std::getline(std::cin, m_sArgs);
                            if (m_sArgs == "-") break;
                            if (m_sArgs == "-c") m_sArgs = Program::GetClipboardText();

                            std::string m_sOldArgs = SessionManager::m_sArgs;
                            if (!m_sArgs.empty())
                                SessionManager::m_sArgs = m_sArgs;

                            if (m_sSubName.empty())
                            {
                                Console::Print(CLR_BYELLOW, '~', "Launching all saved sessions...");

                                for (std::string& SessionName : SessionManager::m_vList)
                                    Chrome::LaunchSession(SessionName);
                            }
                            else
                            {
                                Console::Print(CLR_BYELLOW, '~', "Launching sessions with sub name '" + m_sSubName + "'...");

                                for (std::string& SessionName : SessionManager::m_vList)
                                {
                                    if (SessionName.find(m_sSubName) == std::string::npos) continue;

                                    Chrome::LaunchSession(SessionName);
                                }
                            }

                            SessionManager::m_sArgs = m_sOldArgs;
                        }
                        break;
                        default:
                        {
                            if (m_iSelectedSession == -1) break;

                            Program::Info();
                            Console::Print(CLR_BYELLOW, '~', "Launching session...");

                            Chrome::LaunchSession(SessionManager::m_vList[m_iSelectedSession - 1]);
                        }
                        break;
                    }
                }
            }
            break;

            // Delete Session
            case 1:
            {
                Program::Info();
                Console::Print(Console::m_uDefaultColor, '!', "Session name: ");

                std::string m_sSessionName;
                std::getline(std::cin, m_sSessionName);

                if (m_sSessionName.empty())
                    Program::Error("Session name can't be empty!");
                else if (SessionManager::Create(m_sSessionName))
                {
                    Chrome::LaunchSession(m_sSessionName);
                    Console::Print(CLR_BGREEN, '~', "Successfully added new session.\n");
                    Sleep(5000);
                }
                else
                    Program::Error("Couldn't add new session!");
            }
            break;

            // Select Session
            case 2:
            {
                if (SessionManager::m_vList.empty())
                    Program::Error("You can't delete non existing session.");
                else
                {
                    std::vector<std::string> m_vSessionList = Program::GetFormattedSessionList();
                    m_vSessionList.erase(m_vSessionList.begin());

                    int m_iSelectedSession = Program::GetSelectionIndex("Select Session to delete:\n", m_vSessionList.data(), m_vSessionList.size(), 5, "\t");
                    if (m_iSelectedSession != -1)
                    {
                        Program::Info();
                        Console::Print(Console::m_uDefaultColor, '!', "Are you sure, you wanna delete session with name '" + SessionManager::m_vList[m_iSelectedSession] + "'?\n");
                        Console::Print(Console::m_uDefaultColor, '!', "Valid options (Y / 1 / Yikes): ");

                        std::string m_sAnswer;
                        std::getline(std::cin, m_sAnswer);
                        std::transform(m_sAnswer.begin(), m_sAnswer.end(), m_sAnswer.begin(), [](unsigned char c) { return std::tolower(c); });

                        if (m_sAnswer[0] == 'y' || m_sAnswer[0] == '1')
                        {
                            Console::Print(CLR_BYELLOW, '~', "Deleting session...");
                            SessionManager::Remove(SessionManager::m_vList[m_iSelectedSession]);
                        }
                    }
                }
            }
            break;

            case m_iExitOption: exit(0);
        }
    }

    return 0;
}
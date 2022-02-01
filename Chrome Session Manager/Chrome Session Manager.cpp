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
        m_vReturn.emplace_back("< Start all saved sessions >");

        int m_iCount = 1;
        for (std::string& SessionName : SessionManager::m_vList)
        {
            m_vReturn.emplace_back(std::to_string(m_iCount) + ". " + SessionName);
            ++m_iCount;
        }

        return m_vReturn;
    }
}

namespace Chrome
{
    std::string m_sExecutablePath = "";

    bool Init()
    {
        m_sExecutablePath = Utils::Registry::GetString(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe", "");
        if (m_sExecutablePath.empty() || m_sExecutablePath.find(".exe") == std::string::npos)
            return false; // Check just in-case registry fvcked up

        return true;
    }

    void NewProcess(std::string m_sArgs = "")
    {
        std::string m_sCommandLine = m_sExecutablePath + " " + m_sArgs;

        STARTUPINFO m_sStartupInfo;
        ZeroMemory(&m_sStartupInfo, sizeof(m_sStartupInfo));
        m_sStartupInfo.cb = sizeof(STARTUPINFO);

        PROCESS_INFORMATION m_pProcessInfo;

        if (!CreateProcessA(0, &m_sCommandLine[0], 0, 0, 0, 0, 0, 0, &m_sStartupInfo, &m_pProcessInfo))
            return;

        CloseHandle(m_pProcessInfo.hProcess);
        CloseHandle(m_pProcessInfo.hThread);
    }

    void LaunchSession(std::string m_sName)
    {
        std::string m_sArgs = "";
        auto AddArgument = [&m_sArgs](std::string m_sArg) { m_sArgs += (m_sArgs.empty() ? m_sArg : " " + m_sArg); };

        // Custom arguments
        {
            AddArgument("--simulate-outdated-no-au=\"Tue, 31 Dec 2099 23:59 : 59 GMT\""); // Don't update

            if (!SessionManager::m_sArgs.empty())
                AddArgument(SessionManager::m_sArgs);
        }

        std::string m_sUserdataDir(MAX_PATH, '\0');
        m_sUserdataDir.resize(GetCurrentDirectoryA(m_sUserdataDir.size(), &m_sUserdataDir[0]));

        AddArgument("--user-data-dir=\"" + m_sUserdataDir + "\\" + SessionManager::GetPath(m_sName) + "\"");

        NewProcess(m_sArgs);
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
                    int m_iSelectedSession = Program::GetSelectionIndex("Select Session:\n", m_vSessionList.data(), m_vSessionList.size(), 10, "\t");
                    switch (m_iSelectedSession)
                    {
                        case 0:
                        {
                            Program::Info();
                            Console::Print(CLR_BYELLOW, '~', "Launching all saved sessions...");

                            for (std::string& SessionName : SessionManager::m_vList)
                                Chrome::LaunchSession(SessionName);
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

                    int m_iSelectedSession = Program::GetSelectionIndex("Select Session to delete:\n", m_vSessionList.data(), m_vSessionList.size(), 10, "\t");
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
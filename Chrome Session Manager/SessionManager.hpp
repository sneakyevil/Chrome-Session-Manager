#pragma once
#include <algorithm>
// Main handler for session and its information.

namespace SessionManager
{
	const char* m_pDirectoryName = "sessions";
	const char* m_pArgsFileName = "args.txt";
	std::string m_sArgs = "";

	std::string GetPath(std::string m_sName)
	{
		return std::string(m_pDirectoryName) + "\\" + m_sName;
	}

	std::string GetArgsPath()
	{
		return GetPath(m_pArgsFileName);
	}

	std::vector<std::string> m_vList;
	void Add(std::string m_sName) { m_vList.emplace_back(m_sName); }

	void Refresh()
	{
		m_vList.clear();

		std::string m_sPath = std::string(m_pDirectoryName) + "\\*";
		WIN32_FIND_DATA m_wFindData;

		HANDLE m_hFind = FindFirstFileA(&m_sPath[0], &m_wFindData);
		if (m_hFind)
		{
			while (FindNextFileA(m_hFind, &m_wFindData))
			{
				if (m_wFindData.cFileName[0] == '.') continue;
				if (!(m_wFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;

				Add(m_wFindData.cFileName);
			}

			FindClose(m_hFind);
		}
	}

	bool Create(std::string m_sName)
	{
		std::string m_sCheckName = m_sName;
		std::transform(m_sCheckName.begin(), m_sCheckName.end(), m_sCheckName.begin(), [](unsigned char c) { return std::tolower(c); });
		for (std::string m_sOtherName : m_vList)
		{
			std::transform(m_sOtherName.begin(), m_sOtherName.end(), m_sOtherName.begin(), [](unsigned char c) { return std::tolower(c); });
			if (m_sCheckName == m_sOtherName)
				return false;
		}

		CreateDirectoryA(&GetPath(m_sName)[0], 0);

		Refresh();
		return true;
	}

	void Remove(std::string m_sName)
	{
		std::string m_sSessionFolder(MAX_PATH, '\0');
		m_sSessionFolder.resize(GetCurrentDirectoryA(m_sSessionFolder.size(), &m_sSessionFolder[0]));
		m_sSessionFolder += "\\" + GetPath(m_sName);

		SHFILEOPSTRUCTA m_fOperation;
		memset(&m_fOperation, 0, sizeof(SHFILEOPSTRUCTA));
		m_fOperation.wFunc = FO_DELETE;
		m_fOperation.pFrom = &m_sSessionFolder[0];
		m_fOperation.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;

		SHFileOperationA(&m_fOperation);

		Refresh();
	}

	void Init()
	{
		CreateDirectoryA(m_pDirectoryName, 0);

		Refresh();

		// Parse args file
		{
			std::string m_sPath = GetArgsPath();

			FILE* m_pFile = nullptr;
			if (fopen_s(&m_pFile, &m_sPath[0], "r"))
			{
				if (!fopen_s(&m_pFile, &m_sPath[0], "w"))
					fclose(m_pFile);
			}
			else
			{
				constexpr int m_iArgsSize = 2048;
				m_sArgs.resize(m_iArgsSize, '\0');

				m_sArgs.resize(fread(&m_sArgs[0], 1, m_iArgsSize, m_pFile));

				fclose(m_pFile);
			}
		}
	}
}
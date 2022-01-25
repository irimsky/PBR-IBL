#pragma once
#include <string>
#include <vector>

class File
{
public:
	static std::string readText(const std::string& filename);
	static std::vector<char> readBinary(const std::string& filename);
	static std::vector<char*> readAllFilesInDir(const std::string& path);
	static std::vector<char*> readAllDirsInDir(const std::string& path);
	static std::vector<char*> readAllFilesInDirWithExt(const std::string& path);
};

class Utility
{
public:
	template<typename T> static constexpr bool isPowerOfTwo(T value)
	{
		return value != 0 && (value & (value - 1)) == 0;
	}
	template<typename T> static constexpr T roundToPowerOfTwo(T value, int POT)
	{
		return (value + POT - 1) & -POT;
	}

	// ����һ���������ͼ�����ɶ��ٲ㼶��mipmap��ͳ����߶�����λ����
	template<typename T> static constexpr T numMipmapLevels(T width, T height)
	{
		T levels = 1;
		while ((width | height) >> levels) {
			++levels;
		}
		return levels;
	}
};
// Webpfilesystem.cpp: 定義主控台應用程式的進入點。
//

#include "stdafx.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <vector>
#include <iomanip>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class ImageConvertInterface
{
public:
	virtual std::string GetCommand(const fs::path& pSrc, const fs::path& pDes) = 0;
};

class DirectoryConvert
{
private:
	fs::path _pSrc;
	fs::path _pDes;

	std::vector<fs::path> _vSrcDir;
	std::vector<fs::path> _vDesDir;
	std::vector<fs::path> _vSrcFile;
	std::vector<fs::path> _vDesFile; //Unused

	std::vector<std::reference_wrapper<fs::path>> _vSrcImage;
	std::vector<fs::path> _vDesImage;
	std::vector<std::reference_wrapper<fs::path>> _vSrcOther;
	std::vector<fs::path> _vDesOther;
	std::vector<fs::path> _vSrcWebp;
	std::vector<fs::path> _vDesWebp;

	std::vector<boost::system::error_code> _vErrorCode;
	std::vector<fs::path> _vErrorSrc;
	std::vector<fs::path> _vErrorDes;

	std::vector<fs::path> _vExt = { ".bmp", ".png" };
	std::vector<fs::path> _vExclude = { "Camera Roll", "Saved Pictures" };
	std::vector<fs::path> _vExcludeFile = {}; // Unused
	std::vector<ImageConvertInterface> _Converter;
	std::vector<std::string> _vCommand;

	bool _bSrcWebp = false;
	bool _bDesWebp = false;

	bool _bSrcImage = true;
	bool _bDesImage = false;

	bool _bSrcOther = true;
	bool _bDesOther = false;

	bool _RemoveSrcEmpty = false;
	bool _RemoveDesEmpty = true;

	template<typename... Args>
	static auto Category(Args&&... args)
	{
		std::vector<fs::path> vSrcDir;
		std::vector<fs::path> vSrcFile;

		for (const fs::path& path : fs::recursive_directory_iterator(std::forward<Args>(args)...))
		{
			if (fs::is_directory(path))
			{
				vSrcDir.push_back(path);
			}
			else
			{
				vSrcFile.push_back(path);
			}
		}

		return std::make_tuple(std::move(vSrcDir), std::move(vSrcFile));
	}

	auto GetSourceSubdirectoryList()
	{
		if (true)
		{

		}
	}

	template<typename InIt>
	auto Exclude(InIt First, InIt Last)
	{
		std::vector<std::reference_wrapper<fs::path>> vSrcImage;
		std::vector<std::reference_wrapper<fs::path>> vSrcOther;

		auto itExtBeg = std::begin(_vExt);
		auto itExtEnd = std::end(_vExt);

		std::for_each(First, Last, [&](fs::path& path)
		{
			bool isImgExt = itExtEnd != std::find(itExtBeg, itExtEnd, fs::extension(path));
			bool noneExclude = false;
			if (isImgExt)
			{
				auto&& sPath = path.native();
				auto itPathBeg = std::begin(sPath);
				auto itPathEnd = std::end(sPath);

				noneExclude = std::none_of(std::begin(_vExclude), std::end(_vExclude), [&](auto&& pExclude)
				{
					auto&& sExclude = pExclude.native();
					auto itExcludeBeg = std::begin(sExclude);
					auto itExcludeEnd = std::end(sExclude);

					return itPathEnd != std::search(itPathBeg, itPathEnd, itExcludeBeg, itExcludeEnd);
				});
			}

			if (isImgExt && noneExclude)
			{
				vSrcImage.push_back(path);
			}
			else
			{
				vSrcOther.push_back(path);
			}
		});

		return std::make_tuple(std::move(vSrcImage), std::move(vSrcOther));
	}

	template<typename InIt>
	auto ReplaceParent(InIt First, InIt Last)
	{
		std::transform(First, Last, First, [&](const fs::path& path) { return _pDes / fs::relative(path, _pSrc); });
	}

	template<typename InIt>
	static auto ReplaceExtension(InIt First, InIt Last, const fs::path& extension = ".webp")
	{
		std::for_each(First, Last, std::bind(&fs::path::replace_extension, std::placeholders::_1, extension));
	}

	template<typename InIt1, typename InIt2>
	auto WebpCommand(InIt1 itSrcBeg, InIt1 itSrcEnd, InIt2 itDesBeg)
	{
		std::vector<std::string> vCommand(std::distance(itSrcBeg, itSrcEnd));
		std::transform(itSrcBeg, itSrcEnd, itDesBeg, std::begin(vCommand),
			[&](const fs::path& pSrc, const fs::path& pDes)
		{
			auto sSrc = '"' + pSrc.string() + '"';
			auto sDes = '"' + pDes.string() + '"';
			auto sCommand = _cwebpPath + ' ' + _cwebpParm + ' ' + sSrc + " -o " + sDes;

			return sCommand;
		});

		_vCommand = std::move(vCommand);
	}

	template<typename InIt>
	static auto SystemCommand(InIt First, InIt Last)
	{
		auto iCommand = 1;
		auto iCommandCount = std::distance(First, Last);

		std::for_each(First, Last, [&](const std::string& sCommand)
		{
			system(sCommand.c_str());

			std::cout << "schedule: " << iCommand++ << '/' << iCommandCount << "\n\n";
		});
	}

	template<typename InIt1, typename InIt2, typename Fn>
	auto doFile(InIt1 itSrcBeg, InIt1 itSrcEnd, InIt2 itDesBeg, Fn func)
	{
		for (; itSrcBeg != itSrcEnd; itSrcBeg++, itDesBeg++)
		{
			fs::path& pSrc = *itSrcBeg;
			fs::path& pDes = *itDesBeg;
			boost::system::error_code error;

			func(pSrc, pDes, error);
			if (error)
			{
				_vErrorCode.push_back(std::move(error));
				_vErrorSrc.push_back(pSrc);
				_vErrorDes.push_back(pDes);
			}
		}
	}

	template<typename InIt, typename Fn>
	auto doFile(InIt First, InIt Last, Fn func)
	{
		std::for_each(First, Last, [&](fs::path& path)
		{
			boost::system::error_code error;

			func(path, error);
			if (error)
			{
				_vErrorCode.push_back(std::move(error));
				_vErrorSrc.push_back(path);
				_vErrorDes.push_back(fs::path());
			}
		});
	}

	template<typename InIt1, typename InIt2>
	auto CopyFile(InIt1 itSrcBeg, InIt1 itSrcEnd, InIt2 itDesBeg)
	{
		doFile(itSrcBeg, itSrcEnd, itDesBeg, [](auto&&... a)
		{
			return fs::copy_file(std::forward<decltype(a)>(a)...);
		});
	}

	template<typename InIt1, typename InIt2>
	auto MoveFile(InIt1 itSrcBeg, InIt1 itSrcEnd, InIt2 itDesBeg)
	{
		doFile(itSrcBeg, itSrcEnd, itDesBeg, [](auto&&... a)
		{
			return fs::rename(std::forward<decltype(a)>(a)...);
		});
	}

	template<typename InIt>
	auto RemoveFile(InIt itSrcBeg, InIt itSrcEnd)
	{
		doFile(itSrcBeg, itSrcEnd, [](auto&&... a)
		{
			return fs::remove(std::forward<decltype(a)>(a)...);
		});
	}

	auto SourcePath()
	{
		auto[vSrcDir, vSrcFile] = Category(_pSrc);
		auto[vSrcImage, vSrcOther] = Exclude(std::begin(vSrcFile), std::end(vSrcFile));

		std::vector<fs::path> vSrcWebp(std::begin(vSrcImage), std::end(vSrcImage));
		ReplaceExtension(std::begin(vSrcWebp), std::end(vSrcWebp));

		_vSrcDir = std::move(vSrcDir);
		_vSrcFile = std::move(vSrcFile);
		_vSrcImage = std::move(vSrcImage);
		_vSrcOther = std::move(vSrcOther);
		_vSrcWebp = std::move(vSrcWebp);
	}

	auto DestinationPath()
	{
		std::vector<fs::path> vDesDir(std::begin(_vSrcDir), std::end(_vSrcDir));
		ReplaceParent(std::begin(vDesDir), std::end(vDesDir));

		std::vector<fs::path> vDesImage(std::begin(_vSrcImage), std::end(_vSrcImage));
		ReplaceParent(std::begin(vDesImage), std::end(vDesImage));

		std::vector<fs::path> vDesOther(std::begin(_vSrcOther), std::end(_vSrcOther));
		ReplaceParent(std::begin(vDesOther), std::end(vDesOther));

		std::vector<fs::path> vDesWebp(std::begin(vDesImage), std::end(vDesImage));
		ReplaceExtension(std::begin(vDesWebp), std::end(vDesWebp));

		_vDesDir = std::move(vDesDir);
		_vDesFile; //Unused
		_vDesImage = std::move(vDesImage);
		_vDesOther = std::move(vDesOther);
		_vDesWebp = std::move(vDesWebp);
	}

	auto CreateDestinationDirectory()
	{
		fs::create_directories(_pDes);

		for (auto&& path : _vDesDir)
		{
			fs::create_directory(path);
		}
	}

	template<typename InIt>
	auto RemoveEmptyDirectory(InIt rFirst, InIt rLast)
	{
		std::for_each(rFirst, rLast, [&](const fs::path& path)
		{
			if (fs::directory_iterator(path) == fs::directory_iterator())
			{
				boost::system::error_code error;

				fs::remove(path, error);
				if (error)
				{
					_vErrorCode.push_back(std::move(error));
					_vErrorSrc.push_back(path);
					_vErrorDes.push_back(fs::path());
				}
			}
		});
	}

public:
	DirectoryConvert() = default;
	~DirectoryConvert() = default;

	template<typename Path>
	auto SetSource(Path&& pSource)
	{
		_pSrc = std::forward<Path>(pSource);
	}

	template<typename Path>
	auto SetDestination(Path&& pDestination)
	{
		_pDes = std::forward<Path>(pDestination);
	}

	auto SetFlag(
		bool bSrcWebp, bool bDesWebp,
		bool bSrcImage, bool bDesImage,
		bool bSrcOther, bool bDesOther)
	{
		_bSrcWebp = bSrcWebp;
		_bDesWebp = bDesWebp;
		_bSrcImage = bSrcImage;
		_bDesImage = bDesImage;
		_bSrcOther = bSrcOther;
		_bDesOther = bDesOther;
	}

	auto GetError()
	{
		auto ErrorCount = _vErrorCode.size();

		std::vector<std::string> vError(ErrorCount);
		for (size_t i = 0; i < ErrorCount; i++)
		{
			auto&& error = _vErrorCode[i];
			auto&& pSrc = _vErrorSrc[i];
			auto&& pDes = _vErrorDes[i];

			auto sError = error.message() + " path1:" + pSrc.string();
			if (!pDes.empty())
			{
				sError = sError + ", path2: " + pDes.string();
			}

			vError[i] = std::move(sError);
		}

		return vError;
	}

	template<typename Container, typename CharT = char>
	static auto debug(Container&& container, const CharT* delim = "\n")
	{
		typedef typename std::iterator_traits<decltype(std::begin(container))>::value_type value_type;

		return std::copy(
			std::begin(container), std::end(container),
			std::ostream_iterator<value_type>(std::cout, delim));
	}

	auto Test()
	{
		// Check Flag
		bool bSrcParent = !_pSrc.empty();
		bool bSrcRelative = _bSrcWebp || _bSrcImage || _bSrcOther; // Unused
		if (!bSrcParent)
		{
			return;
		}

		bool bDesParent = !_pDes.empty();
		bool bDesRelative = _bDesWebp || _bDesImage || _bDesOther;
		if (!bDesParent && bDesRelative)
		{
			return;
		}

		bool bDesPath = bDesParent && bDesRelative;

		// Folder management
		SourcePath();

		if (bDesPath)
		{
			DestinationPath();
			CreateDestinationDirectory();
		}

		// Webp File Management
		if (_bSrcWebp && _bDesWebp)
		{
			WebpCommand(std::begin(_vSrcImage), std::end(_vSrcImage), std::begin(_vSrcWebp));
			SystemCommand(std::begin(_vCommand), std::end(_vCommand));
			CopyFile(std::begin(_vSrcWebp), std::end(_vSrcWebp), std::begin(_vDesWebp));
		}
		else if (_bSrcWebp && !_bDesWebp)
		{
			WebpCommand(std::begin(_vSrcImage), std::end(_vSrcImage), std::begin(_vSrcWebp));
			SystemCommand(std::begin(_vCommand), std::end(_vCommand));
		}
		else if (!_bSrcWebp && _bDesWebp)
		{
			WebpCommand(std::begin(_vSrcImage), std::end(_vSrcImage), std::begin(_vDesWebp));
			SystemCommand(std::begin(_vCommand), std::end(_vCommand));
		}

		// Image File Management
		if (_bSrcImage && _bDesImage)
		{
			CopyFile(std::begin(_vSrcImage), std::end(_vSrcImage), std::begin(_vDesImage));
		}
		else if (!_bSrcImage && _bDesImage)
		{
			MoveFile(std::begin(_vSrcImage), std::end(_vSrcImage), std::begin(_vDesImage));
		}
		else if (!_bSrcImage && !_bDesImage)
		{
			RemoveFile(std::begin(_vSrcImage), std::end(_vSrcImage));
		}

		// Other File Management
		if (_bSrcOther && _bDesOther)
		{
			CopyFile(std::begin(_vSrcOther), std::end(_vSrcOther), std::begin(_vDesOther));
		}
		else if (!_bSrcOther && _bDesOther)
		{
			MoveFile(std::begin(_vSrcOther), std::end(_vSrcOther), std::begin(_vDesOther));
		}
		else if (!_bSrcOther && !_bDesOther)
		{
			RemoveFile(std::begin(_vSrcOther), std::end(_vSrcOther));
		}

		// Folder management
		if (_RemoveSrcEmpty)
		{
			RemoveEmptyDirectory(std::rbegin(_vSrcDir), std::rend(_vSrcDir));
		}
		if (_RemoveDesEmpty && bDesPath)
		{
			RemoveEmptyDirectory(std::rbegin(_vDesDir), std::rend(_vDesDir));
		}

		auto vError = GetError();
		debug(vError);
	}
};

class WebpConvert : public ImageConvertInterface
{
public:
	WebpConvert() = default;
	~WebpConvert() = default;

	template<typename String>
	WebpConvert(String&& cwebpPath) :
		_cwebpPath(std::forward<String>(cwebpPath))
	{

	}

	template<typename String>
	auto SetWebpParm(String&& cwebpPath)
	{
		_cwebpParm = std::forward<String>(cwebpPath);
	}

	std::string GetCommand(const fs::path& pSrc, const fs::path& pDes) override
	{
		auto sSrc = '"' + pSrc.string() + '"';
		auto sDes = '"' + pDes.string() + '"';
		auto sCommand = _cwebpPath + ' ' + _cwebpParm + ' ' + sSrc + " -o " + sDes;

		return sCommand;
	}

private:
	std::string _cwebpPath = "cwebp.exe";
	std::string _cwebpParm = "-lossless -q 100 -mt";
};

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		return 1;
	}

	fs::path pSrc = argv[1];
	fs::path pDes = argv[2];

	WebpConvert cWebp("cwebp.exe");
	cWebp.SetWebpParm("-lossless -q 100 -mt");

	DirectoryConvert webp;
	webp.SetSource(pSrc);
	webp.SetDestination(pDes);
	webp.Test();

	system("PAUSE");
	return 0;
}


#include "stdafx.h"
#include "system.h"
#include <paths.h>
#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>

bool core::tools::system::is_dir_writable(const std::wstring &_dir_path_str)
{
    if (_dir_path_str.length())
    {
        NSString *str = [[NSString alloc] initWithBytes:_dir_path_str.data()
                                                 length:_dir_path_str.size() * sizeof(_dir_path_str[0])
                                               encoding:NSUTF32LittleEndianStringEncoding];
        return [[NSFileManager defaultManager] isWritableFileAtPath:str];
    }
    return [[NSFileManager defaultManager] isWritableFileAtPath:@""];
}

bool core::tools::system::delete_file(const std::wstring& _file_name)
{
    boost::system::error_code error;
    boost::filesystem::remove(_file_name, error);
    return !error;
}

bool core::tools::system::move_file(const std::wstring& _old_file, const std::wstring& _new_file)
{
	boost::filesystem::path from(_old_file);
	boost::filesystem::path target(_new_file);
	boost::system::error_code error;
	boost::filesystem::rename(from, target, error);

	if (!error)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool core::tools::system::copy_file(const std::wstring& _old_file, const std::wstring& _new_file)
{
    boost::filesystem::path from(_old_file);
	boost::filesystem::path target(_new_file);
    if (from == target)
    {
        return false;
    }

    boost::system::error_code error;
	//boost::filesystem::copy(from, target, error);
    boost::filesystem::copy_file(from, target, boost::filesystem::copy_option::overwrite_if_exists, error); // overwrite is better

	if (!error)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool core::tools::system::compare_dirs(const std::wstring& _dir1, const std::wstring& _dir2)
{
	if (_dir1.empty() || _dir2.empty())
		return false;

    boost::system::error_code error;
    return boost::filesystem::equivalent(boost::filesystem::path(_dir1), boost::filesystem::path(_dir2), error);
}

std::wstring core::tools::system::get_file_directory(const std::wstring& file)
{
    boost::filesystem::wpath p(file);
    return p.parent_path().wstring();
}

std::wstring core::tools::system::get_file_name(const std::wstring& file)
{
    boost::filesystem::wpath p(file);
    return p.filename().wstring();
}

std::wstring core::tools::system::get_temp_directory()
{
    std::string tmpdir;

    if (auto tmpdir_env = ::getenv("TMPDIR"))
        tmpdir = tmpdir_env;

    if (tmpdir.empty())
        tmpdir = _PATH_TMP;

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(tmpdir);

    return wide;
}

std::wstring core::tools::system::get_user_profile()
{
	const char * home = ::getenv("HOME");
	std::string result = home;
	result.append("/Library/Application Support");

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(result);

	return wide;
}

unsigned long core::tools::system::get_current_thread_id()
{
	std::string threadId = boost::lexical_cast<std::string>(boost::this_thread::get_id());
	unsigned long threadNumber = 0;
	sscanf(threadId.c_str(), "%lx", &threadNumber);
	return threadNumber;
}

std::wstring core::tools::system::get_user_downloads_dir()
{
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDownloadsDirectory, NSUserDomainMask, YES);
	NSString *downloadsDir = [paths objectAtIndex:0];
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring str = converter.from_bytes([downloadsDir UTF8String]);
	return str;
}

std::string core::tools::system::to_upper(std::string_view str)
{
    NSString *string = [NSString stringWithBytes:str.data() length:str.size() encoding:NSUTF8StringEncoding];
    if (string == nil)
        return std::string(str);

    string = [string uppercaseString];
    return std::string([string UTF8String]);
}

std::string core::tools::system::to_lower(std::string_view str)
{
    NSString *string = [NSString stringWithBytes:str.data() length:str.size() encoding:NSUTF8StringEncoding];
    if (string == nil)
        return std::string(str);

    string = [string lowercaseString];
    return std::string([string UTF8String]);
}

std::string core::tools::system::get_os_version_string()
{
    SInt32 major, minor, bugfix;
    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);
    Gestalt(gestaltSystemVersionBugFix, &bugfix);

    NSString *systemVersion = [NSString stringWithFormat:@"MacOSX %d.%d", major, minor];
    return std::string([systemVersion UTF8String]);
}

bool core::tools::system::is_do_not_dirturb_on()
{
    NSUserDefaults* defaults = [[NSUserDefaults alloc] initWithSuiteName:@"com.apple.notificationcenterui"];
    return [defaults boolForKey:@"doNotDisturb"];
}

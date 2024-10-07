﻿#pragma once

#include "xvtCV/xvtDefine.h"
#include <string>
#include <vector>


namespace xvt {
   
    template <typename T>
    struct PathTraits {
        static const T mDelimiters;
    };

    // Define static member variable for std::string
    template <>
    const std::string PathTraits<std::string>::mDelimiters = "\\/";

    // Define static member variable for std::wstring
    template <>
    const std::wstring PathTraits<std::wstring>::mDelimiters = L"\\/";

    /// <summary>
    /// Get the file name with the extesion from the path
    /// ex: GetFileName("D\\test.txt") return "test.txt"
    /// </summary>
    /// <typeparam name="T">Any type of string that support find_last_of and substr</typeparam>
    /// <param name="path">File path</param>
    /// <returns></returns>
    template<class T>
    inline T GetFileName(T const& path)
    {
        typename T::size_type const p(path.find_last_of(PathTraits<T>::mDelimiters));
        return p != T::npos ? path.substr(p + 1) : path;
    }

    template<class T>
    inline T GetFileExtension(T const& path)
    {
        T  fileName = GetFileName(path);
        typename T::size_type const p(fileName.find_last_of('.'));
        return p != T::npos ? fileName.substr(p) : T();
    }

    template<class T>
    inline T RemoveExtension(T const& filename)
    {
        typename T::size_type const p(filename.find_last_of('.'));
        return p != T::npos ? filename.substr(0, p) : filename;
    }

    template<class T>
    inline T GetFileNameWithoutExtension(T const& path)
    {
        return RemoveExtension(GetFileName(path));
    }

    template<class T>
    inline T GetParentDirectory(T const& path)
    {
        typename T::size_type const p(path.find_last_of(PathTraits<T>::mDelimiters));
        return p != T::npos ? path.substr(0, p) : path;
    }
    XVT_EXPORTS std::wstring  RemoveFileSpec(std::wstring  lpszDirPath);
    // ::PathCombine method
    // Concatenates two strings that represent properly formed paths into one path
    XVT_EXPORTS std::wstring  CombinePath(std::wstring  lpszPath1, std::wstring  lpszPath2);

    XVT_EXPORTS bool IsPathExist(std::wstring  lpszPath);
    // ::PathAddBackslash
    // Adds a backslash to the end of a string 
    XVT_EXPORTS std::wstring  AddBackslash(std::wstring  lpszPath);

    // Removes a backslash from the end of a string 
    XVT_EXPORTS std::wstring  RemoveBackslash(std::wstring  lpszPath);

    // ::PathIsRelative
    // Searches a path and determines if it is relative.
    XVT_EXPORTS bool IsRelativePath(std::wstring  lpszPath);

    XVT_EXPORTS std::wstring  GetTempFolderPath(void);

    XVT_EXPORTS std::wstring  GetCurrentFolderPath(void);

    XVT_EXPORTS std::wstring  GetCurrentProcessModulePath(void);

    XVT_EXPORTS std::wstring  GetCurrentProcessFolderPath(void);

    XVT_EXPORTS void SplitPath(
        std::wstring  lpszPath,
        std::wstring* pStrDrive, std::wstring* pStrDir, std::wstring* pStrFileTitle, std::wstring* pStrExt);

    XVT_EXPORTS void SplitPath(
        // input path
        std::wstring  lpszPath,
        std::wstring* pStrDirPath, std::wstring* pStrFileName);

    XVT_EXPORTS std::wstring  CreateGUID(void);

    // remove first slash character
    XVT_EXPORTS std::wstring  RemoveFirstSlash(std::wstring  lpszValue);

    // replace all slash characters to dot characters
    XVT_EXPORTS std::wstring  ReplaceSlashToDot(std::wstring  lpszValue);

    // replace all dot characters to slash characters
    XVT_EXPORTS std::wstring  ReplaceDotToSlash(std::wstring  lpszValue);

    // xml path¸¦ 
    XVT_EXPORTS std::wstring  MakeValidXmlPath(std::wstring  lpszPath);
    XVT_EXPORTS void TokenizeString(std::wstring  lpszSource, const char& lpszTokens, std::vector<std::wstring >& arrValues);


    XVT_EXPORTS  bool IsFileExist(const std::wstring& name);

    XVT_EXPORTS bool MakeDirectory(std::wstring  lpszDirPath);

    XVT_EXPORTS bool DeleteDirectory(std::wstring sPath);

    XVT_EXPORTS int CopyDirectory(std::wstring  lpszSrcDirPath, std::wstring  lpszDstDirPath);

    XVT_EXPORTS bool MoveDirectory(std::wstring  lpszDirPath, std::wstring  lpszNewDirPath);

    XVT_EXPORTS bool DeleteFolder(std::wstring  lpszDirPath);

    XVT_EXPORTS bool CopyFolder(std::wstring  lpszSrcDirPath, std::wstring  lpszDstDirPath);

    XVT_EXPORTS bool MoveFolder(std::wstring  lpszDirPath, std::wstring  lpszNewDirPath);

    
} // namespace xvt

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// for convenience
#define XVT_DIR_RemoveFileSpec                      ::xvt::RemoveFileSpec
#define XVT_DIR_CombinePath                         ::xvt::CombinePath
#define XVT_DIR_IsPathExist                         ::xvt::IsPathExist
#define XVT_DIR_AddBackslash                        ::xvt::AddBackslash
#define XVT_DIR_RemoveBackslash                     ::xvt::RemoveBackslash
#define XVT_DIR_IsRelativePath                      ::xvt::IsRelativePath
#define XVT_DIR_GetTempFolderPath                   ::xvt::GetTempFolderPath
#define XVT_DIR_GetCurrentProcessModulePath         ::xvt::GetCurrentProcessModulePath
#define XVT_DIR_GetCurrentProcessFolderPath         ::xvt::GetCurrentProcessFolderPath
#define XVT_DIR_GetCurrentFolderPath                ::xvt::GetCurrentFolderPath
#define XVT_DIR_SplitPath                           ::xvt::SplitPath
#define XVT_DIR_CreateGUID                          ::xvt::CreateGUID
#define XVT_DIR_RemoveFirstSlash                    ::xvt::RemoveFirstSlash
#define XVT_DIR_ReplaceSlashToDot                   ::xvt::ReplaceSlashToDot
#define XVT_DIR_ReplaceDotToSlash                   ::xvt::ReplaceDotToSlash

#define XVT_DIR_MakeDirectory                       ::xvt::MakeDirectory
#define XVT_DIR_DeleteDirectory                     ::xvt::DeleteDirectory
#define XVT_DIR_CopyDirectory                       ::xvt::CopyDirectory
#define XVT_DIR_MoveDirectory                       ::xvt::MoveDirectory
#define XVT_DIR_DeleteFolder                        ::xvt::DeleteFolder
#define XVT_DIR_CopyFolder                          ::xvt::CopyFolder
#define XVT_DIR_MoveFolder                          ::xvt::MoveFolder
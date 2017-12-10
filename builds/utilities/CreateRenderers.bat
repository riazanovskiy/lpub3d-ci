@ECHO OFF &SETLOCAL

Title Build, test and package LPub3D 3rdParty renderers.
rem --
rem  Trevor SANDY <trevor.sandy@gmail.com>
rem  Last Update: December 08, 2017
rem  Copyright (c) 2017 by Trevor SANDY
rem --
rem This script is distributed in the hope that it will be useful,
rem but WITHOUT ANY WARRANTY; without even the implied warranty of
rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

SET start=%time%

rem get the parent folder
FOR %%* IN (.) DO SET SCRIPT_DIR=%%~nx*
rem Variables - change these as required by your build environments
IF "%APPVEYOR%" EQU "True" (
  rem get abs path to build 3rd party packages inside the LPub3D root dir
  IF "%SCRIPT_DIR%" EQU "utilities" (
    CALL :WD_REL_TO_ABS ../../
  ) ELSE (
    CALL :WD_REL_TO_ABS .
  )
  SET BUILD_OUTPUT_PATH=%APPVEYOR_BUILD_FOLDER%
  SET LDRAW_DIR=%APPVEYOR_BUILD_FOLDER%\LDraw
  SET DIST_DIR=%LP3D_DIST_DIR_PATH%
  SET BUILD_ARCH=%LP3D_TARGET_ARCH%
) ELSE (
  rem get abs path to build 3rd party packages outside the LPub3D root dir
  IF "%SCRIPT_DIR%" EQU "utilities" (
    CALL :WD_REL_TO_ABS ../../../
  ) ELSE (
    CALL :WD_REL_TO_ABS ../
  )
  SET BUILD_OUTPUT_PATH=%ABS_WD%
  SET LDRAW_DIR=%USERPROFILE%\LDraw
  SET DIST_DIR=../lpub3d_windows_3rdparty
  SET LP3D_QT32_MSYS2=C:\Msys2\Msys64\mingw32\bin
  SET LP3D_QT64_MSYS2=C:\Msys2\Msys64\mingw64\bin
  SET LP3D_WIN_GIT=%ProgramFiles%\Git\cmd
)
SET MAX_DOWNLOAD_ATTEMPTS=4
SET VER_LDGLITE=ldglite-1.3
SET VER_LDVIEW=ldview-4.3
SET VER_POVRAY=lpub3d_trace_cui-3.8
SET SYS_DIR=%SystemRoot%cls\System32\System32
SET ZIP_DIR_64=C:\program files\7-zip
SET VALID_ZIP=0

rem Check if invalid platform flag
IF NOT [%1]==[] (
  IF NOT "%1"=="x86" (
    IF NOT "%1"=="x86_64" (
      IF NOT "%1"=="-all" (
        IF NOT "%1"=="-help" GOTO :COMMAND_ERROR
      )
    )
  )
)

ECHO.
ECHO   BUILD LPUB3D RENDERERS...
ECHO.
ECHO   WORKING DIRECTORY..............[%ABS_WD%]
ECHO   DISTRIBUTION DIRECTORY.........[%DIST_DIR:/=\%]
ECHO   LDRAW LIBRARY FOLDER...........[%LDRAW_DIR%]

IF EXIST "%ZIP_DIR_64%" (
  ECHO   7-ZIP EXECUTABLE...............[Found at %ZIP_DIR_64%]
  SET VALID_ZIP=1
) ELSE (
  ECHO   7-ZIP EXECUTABLE...............[ERROR - not found at %ZIP_DIR_64%]
  GOTO :END
)

CALL :CHECK_LDRAW_LIB

IF [%1]==[] (
  CALL :BUILD_ALL
)
IF /I "%1"=="-all" (
  CALL :BUILD_ALL
)
IF /I "%1"=="x86" (
  SET BUILD_ARCH=%1
  CALL :BUILD
)
IF /I "%1"=="x86_64" (
  SET BUILD_ARCH=%1
  CALL :BUILD
)
GOTO :END

:BUILD_ALL
FOR %%A IN ( x86, x86_64 ) DO (
  SET BUILD_ARCH=%%A
  CALL :BUILD
)
EXIT /b

:BUILD
IF %BUILD_ARCH% EQU x86 (
  SET LP3D_LDGLITE=%DIST_DIR%\%VER_LDGLITE%\bin\i386\ldglite.exe
  SET LP3D_LDVIEW=%DIST_DIR%\%VER_LDVIEW%\bin\i386\LDView64.exe
  SET LP3D_POVRAY=%DIST_DIR%\%VER_POVRAY%\bin\i386\lpub3d_trace_cui64.exe
  IF "%PATH_PREPENDED%" NEQ "True" (
    rem Qt MinGW 32bit
    SET PATH=%LP3D_QT32_MSYS2%;%SYS_DIR%;%LP3D_WIN_GIT%
    SET PATH_PREPENDED=True
    ECHO.
    SETLOCAL ENABLEDELAYEDEXPANSION
    ECHO(   PATH_PREPEND............[!PATH!]
      ENDLOCAL
    )
  ) ELSE (
    ECHO    PATH_ALREADY_PREPENDED..[%PATH%]
  )
  CALL :SET_BUILD_ARGS
  FOR %%I IN ( LDGLITE, LDVIEW, POVRAY ) DO CALL :%%I_BUILD
) ELSE (
  SET LP3D_LDGLITE=%DIST_DIR%\%VER_LDGLITE%\bin%BUILD_ARCH%\ldglite.exe
  SET LP3D_LDVIEW=%DIST_DIR%\%VER_LDVIEW%\bin\%BUILD_ARCH%\LDView64.exe
  SET LP3D_POVRAY=%DIST_DIR%\%VER_POVRAY%\bin\%BUILD_ARCH%\lpub3d_trace_cui64.exe
  IF "%PATH_PREPENDED%" NEQ "True" (
    rem Qt MinGW 64bit
    SET PATH=%LP3D_QT64_MSYS2%;%SYS_DIR%;%LP3D_WIN_GIT%
    SET PATH_PREPENDED=True
    ECHO.
    SETLOCAL ENABLEDELAYEDEXPANSION
    ECHO(  PATH_PREPEND............[!PATH!]
      ENDLOCAL
    )
  ) ELSE (
    ECHO   PATH_ALREADY_PREPENDED..[%PATH%]
  )
  CALL :SET_BUILD_ARGS
  FOR %%I IN ( LDGLITE, LDVIEW, POVRAY ) DO CALL :%%I_BUILD
)
EXIT /b

:SET_BUILD_ARGS
SET LDGLITE_BUILD_ARGS=%BUILD_ARCH% -ins -chk
SET LDVIEW_BUILD_ARGS=%BUILD_ARCH% -ins -chk -minlog
IF %BUILD_ARCH% EQU x86_64 (
  SET POVRAY_BUILD_ARGS=%BUILD_ARCH% -allins -chk -minlog
) ELSE (
  SET POVRAY_BUILD_ARGS=%BUILD_ARCH% -ins -chk -minlog
)
EXIT /b

:LDGLITE_BUILD
ECHO.
IF NOT EXIST "%LP3D_LDGLITE%" (
  ECHO -Build LDGLite...
  SET BUILD_DIR=ldglite
  SET VALID_SDIR=mui
  SET ARCHIVE_FILE_DIR=ldglite-master
  SET WebNAME=https://github.com/trevorsandy/ldglite/archive/master.zip
  CALL :CONFIGURE_BUILD_ENV
  CALL build.cmd %LDGLITE_BUILD_ARGS%
) ELSE (
  ECHO - Renderer %VER_LDGLITE% exist - build skipped.
)
EXIT /b

:LDVIEW_BUILD
ECHO.
IF NOT EXIST "%LP3D_LDVIEW%" (
  ECHO -Build LDView...
  SET BUILD_DIR=ldview
  SET VALID_SDIR=OSMesa
  SET ARCHIVE_FILE_DIR=ldview-qmake-build
  SET WebNAME=https://github.com/trevorsandy/ldview/archive/qmake-build.zip
  CALL :CONFIGURE_BUILD_ENV
  CALL build.cmd %LDVIEW_BUILD_ARGS%
) ELSE (
  ECHO - Renderer %VER_LDVIEW% exist - build skipped.
)
EXIT /b

:POVRAY_BUILD
ECHO.
IF NOT EXIST "%LP3D_POVRAY%" (
  ECHO -Build LPub3D-Trace ^(POV-Ray^)...
  SET BUILD_DIR=povray
  SET VALID_SDIR=windows
  SET ARCHIVE_FILE_DIR=povray-lpub3d-raytracer-cui
  SET WebNAME=https://github.com/trevorsandy/povray/archive/lpub3d/raytracer-cui.zip
  CALL :CONFIGURE_BUILD_ENV
  SETLOCAL ENABLEDELAYEDEXPANSION
  CD /D !VALID_SDIR!\vs2015
  CALL autobuild.cmd %POVRAY_BUILD_ARGS%
  ENDLOCAL
) ELSE (
  ECHO - Renderer %VER_POVRAY% exist - build skipped.
)
EXIT /b

:CONFIGURE_BUILD_ENV
CD /D %BUILD_OUTPUT_PATH%
SET ARCHIVE_FILE=%ARCHIVE_FILE_DIR%.zip
SET WebCONTENT="%BUILD_OUTPUT_PATH%\%ARCHIVE_FILE%"
ECHO  ----------------------------------------------------
ECHO.
ECHO   BUILD_ARCH.....................[%BUILD_ARCH%]
ECHO   BUILD_DIRECTORY................[%BUILD_DIR%]
ECHO   VALID_SUB_DIRECTORY............[%VALID_SDIR%]
ECHO   ARCHIVE_FILE_DIRECTORY.........[%ARCHIVE_FILE_DIR%]
ECHO   ARCHIVE_FILE...................[%ARCHIVE_FILE%]
ECHO   DOWNLOAD_URL.(WebNAME).........[%WebNAME%]
ECHO   DOWNLOAD_PATH.(WebCONTENT).....[%WebCONTENT%]
IF NOT EXIST "%BUILD_OUTPUT_PATH%\%BUILD_DIR%\%VALID_SDIR%" (
  IF NOT EXIST "%BUILD_OUTPUT_PATH%\%ARCHIVE_FILE%" (
    ECHO.
    ECHO -Directory %BUILD_DIR% does not exist - Downloading %BUILD_DIR% source archive...

    CALL :DOWNLOAD_ARCHIVE
  )

  CALL :EXTRACT_ARCHIVE
  CALL :SETUP_BUILD_DIR

) ELSE (
  ECHO.
  ECHO -Directory exist at [%CD%\%BUILD_DIR%] - Start build...
)
ECHO.
CD /D %BUILD_OUTPUT_PATH%\%BUILD_DIR%
EXIT /b

:CHECK_LDRAW_LIB
ECHO.
ECHO -Check for LDraw library (support image render tests)...
SET BUILD_OUTPUT_PATH_SAVE=%BUILD_OUTPUT_PATH%
IF "%APPVEYOR%" NEQ "True" (
  SET BUILD_OUTPUT_PATH=%USERPROFILE%
)
SET ARCHIVE_FILE_DIR=%LDRAW_DIR%
SET ARCHIVE_FILE=complete.zip
SET VALID_SDIR=parts
SET WebCONTENT="%BUILD_OUTPUT_PATH%\%ARCHIVE_FILE%"
SET WebNAME=http://www.ldraw.org/library/updates/complete.zip
IF NOT EXIST "%LDRAW_DIR%\%VALID_SDIR%" (
  IF NOT EXIST "%BUILD_OUTPUT_PATH%\%ARCHIVE_FILE%" (
    ECHO.
    ECHO -LDraw directory %LDRAW_DIR% does not exist - Downloading LDraw source archive...

    CALL :DOWNLOAD_ARCHIVE
  )

  CALL :EXTRACT_ARCHIVE

  IF EXIST "%LDRAW_DIR%\%VALID_SDIR%" (
    ECHO.
    ECHO -LDraw directory %LDRAW_DIR% successfully extracted.
    ECHO.
    ECHO -Set LDRAWDIR to %LDRAW_DIR%.
    SET LDRAWDIR=%LDRAW_DIR%
    ECHO.
    ECHO -Cleanup %ARCHIVE_FILE%...
    DEL /Q "%BUILD_OUTPUT_PATH%\%ARCHIVE_FILE%"
  ) ELSE (
      ECHO.
      ECHO -[ERROR] LDraw library folder %LDRAW_DIR% is not valid.
      SET BUILD_OUTPUT_PATH=%BUILD_OUTPUT_PATH_SAVE%
      EXIT /b
  )
) ELSE (
  ECHO.
  ECHO -LDraw directory exist at [%CD%\%LDRAW_DIR%].
  ECHO.
  ECHO -Set LDRAWDIR to %LDRAW_DIR%.
  SET LDRAWDIR=%LDRAW_DIR%
)
SET BUILD_OUTPUT_PATH=%BUILD_OUTPUT_PATH_SAVE%
EXIT /b

rem args: $1 = <build dir>, $2 = <valid subdir>
:EXTRACT_ARCHIVE
IF NOT EXIST "%BUILD_OUTPUT_PATH%\%ARCHIVE_FILE_DIR%" (
  IF EXIST "%BUILD_OUTPUT_PATH%\%ARCHIVE_FILE%" (
    ECHO.
    ECHO -Extracting %ARCHIVE_FILE%...
    ECHO.
    IF %VALID_ZIP% EQU 1 "%ZIP_DIR_64%\7z.exe" x -o"%BUILD_OUTPUT_PATH%\" "%BUILD_OUTPUT_PATH%\%ARCHIVE_FILE%" | findstr /i /r /c:"^Extracting\>" /c:"^Everything\>"
  ) ELSE (
    ECHO.
    ECHO -[ERROR] Could not find %BUILD_OUTPUT_PATH%\%ARCHIVE_FILE%.
    GOTO :END
  )
)
EXIT /b

:SETUP_BUILD_DIR
IF EXIST "%BUILD_OUTPUT_PATH%\%ARCHIVE_FILE_DIR%\%VALID_SDIR%" (
  ECHO.
  ECHO -Renaming %ARCHIVE_FILE_DIR% to %BUILD_DIR% at %CD%...
  REN "%ARCHIVE_FILE_DIR%" "%BUILD_DIR%"
  ECHO.
  ECHO -Directory %BUILD_DIR% successfully extracted.
  ECHO.
  ECHO -Cleanup %ARCHIVE_FILE%...
  DEL /Q "%BUILD_OUTPUT_PATH%\%ARCHIVE_FILE%"
) ELSE (
  ECHO.
  ECHO -[ERROR] Build folder %BUILD_OUTPUT_PATH%\%ARCHIVE_FILE_DIR% is not valid.
  GOTO :END
)
EXIT /b

:DOWNLOAD_ARCHIVE
ECHO.
ECHO - Download ARCHIVE %ARCHIVE_FILE%...
ECHO.
ECHO - Prepare BATCH to VBS to Web Content Downloader...

IF NOT EXIST "%TEMP%\$" (
  MD "%TEMP%\$"
)

SET retries=0
SET vbs=WebContentDownload.vbs
SET t=%TEMP%\$\%vbs% ECHO

IF EXIST %TEMP%\$\%vbs% (
 DEL %TEMP%\$\%vbs%
)

:WEB CONTENT SAVE-AS Download-- VBS
>%t% Option Explicit
>>%t% On Error Resume Next
>>%t%.
>>%t% Dim args, http, fileSystem, adoStream, url, target, status
>>%t%.
>>%t% Set args = Wscript.Arguments
>>%t% Set http = CreateObject("WinHttp.WinHttpRequest.5.1")
>>%t% url = args(0)
>>%t% target = args(1)
>>%t% WScript.Echo "- Getting '" ^& target ^& "' from '" ^& url ^& "'...", vbLF
>>%t%.
>>%t% http.Open "GET", url, False
>>%t% http.Send
>>%t% status = http.Status
>>%t%.
>>%t% If status ^<^> 200 Then
>>%t% WScript.Echo "- FAILED to download: HTTP Status " ^& status, vbLF
>>%t% WScript.Quit 1
>>%t% End If
>>%t%.
>>%t% Set adoStream = CreateObject("ADODB.Stream")
>>%t% adoStream.Open
>>%t% adoStream.Type = 1
>>%t% adoStream.Write http.ResponseBody
>>%t% adoStream.Position = 0
>>%t%.
>>%t% Set fileSystem = CreateObject("Scripting.FileSystemObject")
>>%t% If fileSystem.FileExists(target) Then fileSystem.DeleteFile target
>>%t% If Err.Number ^<^> 0 Then
>>%t%   WScript.Echo "- Error - CANNOT DELETE: '" ^& target ^& "', " ^& Err.Description
>>%t%   WScript.Echo " The file may be in use by another process.", vbLF
>>%t%   adoStream.Close
>>%t%   Err.Clear
>>%t% Else
>>%t%  adoStream.SaveToFile target
>>%t%  adoStream.Close
>>%t%  WScript.Echo "- Download successful!"
>>%t% End If
>>%t%.
>>%t% 'WebContentDownload.vbs
>>%t% 'Title: BATCH to VBS to Web Content Downloader
>>%t% 'CMD ^> cscript //Nologo %TEMP%\$\%vbs% WebNAME WebCONTENT
>>%t% 'VBS Created on %date% at %time%
>>%t%.

ECHO.
ECHO - VBS file "%vbs%" is done compiling

:DO_DOWNLOAD
ECHO.
ECHO - File download path: %BUILD_OUTPUT_PATH%
ECHO.
ECHO - [%date% %time%] Download file: %WebCONTENT%...

IF EXIST %WebCONTENT% (
 DEL %WebCONTENT%
)

ECHO.
cscript //Nologo %TEMP%\$\%vbs% %WebNAME% %WebCONTENT% && @ECHO off

IF ERRORLEVEL 1 GOTO :RETRY_DOWNLOAD

SET retries=0

IF EXIST %ARCHIVE_FILE% (
  ECHO.
  ECHO - Archive %ARCHIVE_FILE% available
)
EXIT /b

:RETRY_DOWNLOAD
SET /a retries=%retries%+1
IF %retries% LSS %MAX_DOWNLOAD_ATTEMPTS% (
  ECHO.
  ECHO - Download %ARCHIVE_FILE% failed. Retrying...
  ECHO - Retry number %retries%.
  GOTO :DO_DOWNLOAD
)
IF %retries% EQU %MAX_DOWNLOAD_ATTEMPTS% (GOTO :DOWNLOAD_ERROR)

:DOWNLOAD_ERROR
ECHO.
ECHO ECHO - [%date% %time%] - ERROR - Download failed after %retries% retries.
EXIT /b

:WD_REL_TO_ABS
IF [%1] EQU [] (EXIT /b) ELSE (SET REL_WD=%1)
SET REL_WD=%REL_WD:/=\%
SET ABS_WD=
PUSHD %REL_WD%
SET ABS_WD=%CD%
POPD
EXIT /b

:COMMAND_ERROR
ECHO.
CALL :USAGE
ECHO.
ECHO -01. (COMMAND ERROR) Invalid command string [%~nx0 %*].
ECHO      See Usage.
GOTO :END

:USAGE
ECHO ----------------------------------------------------------------
ECHO.
ECHO Build, test and package LPub3D 3rdParty renderers.
ECHO.
ECHO ----------------------------------------------------------------
ECHO Usage:
ECHO  build [ -help]
ECHO  build [ x86 ^| x86_64 ^| -all ]
ECHO.
ECHO ----------------------------------------------------------------
ECHO Build 64bit, Releases
ECHO build x86_64
ECHO.
ECHO Build 32bit, Releases
ECHO build x86
ECHO.
ECHO Build 64bit and32bit, Releases
ECHO build -all
ECHO.
ECHO Commands:
ECHO ----------------------------------------------------------------
ECHO [PowerShell]: cmd.exe /c builds\utilities\CreateRenderers.bat
ECHO [cmd.exe]   : builds\utilities\CreateRenderers.bat
ECHO.
ECHO Flags:
ECHO ----------------------------------------------------------------
ECHO ^| Flag    ^| Pos ^| Type             ^| Description
ECHO ----------------------------------------------------------------
ECHO  -help......1......Useage flag         [Default=Off] Display useage.
ECHO  x86........1......Platform flag       [Default=Off] Build 32bit architecture.
ECHO  x86_64.....1......Platform flag       [Default=Off] Build 64bit architecture.
ECHO  -all.......1......Configuraiton flag  [Default=On ] Build both 32bit and 64bit architectures
ECHO.
ECHO Flags are case sensitive, use lowere case.
ECHO.
ECHO If no flag is supplied, 64bit platform, Release Configuration built by default.
ECHO ----------------------------------------------------------------
EXIT /b

:END
ECHO.
ECHO -%~nx0 finished.
SET end=%time%
SET options="tokens=1-4 delims=:.,"
FOR /f %options% %%a IN ("%start%") DO SET start_h=%%a&SET /a start_m=100%%b %% 100&SET /a start_s=100%%c %% 100&SET /a start_ms=100%%d %% 100
FOR /f %options% %%a IN ("%end%") DO SET end_h=%%a&SET /a end_m=100%%b %% 100&SET /a end_s=100%%c %% 100&SET /a end_ms=100%%d %% 100

SET /a hours=%end_h%-%start_h%
SET /a mins=%end_m%-%start_m%
SET /a secs=%end_s%-%start_s%
SET /a ms=%end_ms%-%start_ms%
IF %ms% lss 0 SET /a secs = %secs% - 1 & SET /a ms = 100%ms%
IF %secs% lss 0 SET /a mins = %mins% - 1 & SET /a secs = 60%secs%
IF %mins% lss 0 SET /a hours = %hours% - 1 & SET /a mins = 60%mins%
IF %hours% lss 0 SET /a hours = 24%hours%
IF 1%ms% lss 100 SET ms=0%ms%
ECHO -Elapsed build time %hours%:%mins%:%secs%.%ms%
ENDLOCAL
EXIT /b

Name "linuxtrack-wine64"
OutFile "linuxtrack-wine.exe"
InstallDir $PROGRAMFILES\Linuxtrack
InstallDirRegKey HKLM "Software\Linuxtrack" "InstallDir"

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "Linuxtrack"
  SectionIn RO
  SetOutPath $INSTDIR
  File "Controller.exe"
  File "Controller.sh"
  File "Tester.exe"
  File "Tester.sh"
  File "Tester64.exe"
  File "Tester64.sh"
  File "NPClient.dll"
  File "NPClient64.dll"
  File "linuxtrack.ico"
  WriteRegStr HKLM SOFTWARE\Linuxtrack "Install_dir" "$INSTDIR"
  WriteRegStr HKCU "Software\NaturalPoint\NATURALPOINT\NPClient Location" "Path" "$INSTDIR\"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Linuxtrack" "DisplayName" "Linuxtrack wine bridge"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Linuxtrack" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Linuxtrack" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Linuxtrack" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\Linuxtrack"
  CreateShortCut "$SMPROGRAMS\Linuxtrack\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Linuxtrack\Controller.lnk" "$INSTDIR\Controller.exe" "" "$INSTDIR\linuxtrack.ico" 0
  CreateShortCut "$SMPROGRAMS\Linuxtrack\Tester.lnk" "$INSTDIR\Tester.exe" "" "$INSTDIR\Tester.exe"
SectionEnd

Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Linuxtrack"
  DeleteRegKey HKLM SOFTWARE\Linuxtrack
  Delete $INSTDIR\Controller.exe
  Delete $INSTDIR\Controller.sh
  Delete $INSTDIR\Tester.exe
  Delete $INSTDIR\Tester.sh
  Delete $INSTDIR\Tester64.exe
  Delete $INSTDIR\Tester64.sh
  Delete $INSTDIR\NPClient.dll
  Delete $INSTDIR\NPClient64.dll
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\linuxtrack.ico
  Delete "$SMPROGRAMS\Linuxtrack\*.*"
  RMDir "$SMPROGRAMS\Linuxtrack"
  RMDir "$INSTDIR"
SectionEnd




@echo off
REM ===== UE5.6 Windows ビルド & 実機デプロイ & 実行 =====

REM UE の RunUAT.bat を呼び出す
 "D:\Engine\UnrealEngine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
 -project="..\Redemption.uproject" -target="Redemption" ^
 -platform=Win64 -clientconfig=Shipping ^
 -cook -stage -pak -iostore -compressed -build ^
 -package -archive -archivedirectory="..\Builds\" ^
 -utf8output -CrashForUAT -iterate

pause



rmdir /q /s ..\\Intermediate
rmdir /q /s ..\\Saved\\Autosaves
rmdir /q /s ..\\Saved\\Backup
rmdir /q /s ..\\Saved\\Collections
rmdir /q /s ..\\Saved\\Config
rmdir /q /s ..\\Saved\\Crashes
rmdir /q /s ..\\Saved\\Logs

rmdir /q /s ..\\.vs
rmdir /q /s ..\\Binaries
rmdir /q /s ..\\DerivedDataCache

rmdir /q /s ..\\Plugins\\BrokenGlassEffects\\Binaries
rmdir /q /s ..\\Plugins\\BrokenGlassEffects\\Intermediate
rmdir /q /s ..\\Plugins\\RopeCutting\\Binaries
rmdir /q /s ..\\Plugins\\RopeCutting\\Intermediate
rmdir /q /s ..\\Plugins\\QuadrupedIK\\Binaries
rmdir /q /s ..\\Plugins\\QuadrupedIK\\Intermediate
rmdir /q /s ..\\Plugins\\WvAbilitySystem\\Binaries
rmdir /q /s ..\\Plugins\\WvAbilitySystem\\Intermediate
rmdir /q /s ..\\Plugins\\WvPostProcess\\Binaries
rmdir /q /s ..\\Plugins\\WvPostProcess\\Intermediate

del ..\\.vsconfig
del ..\\Redemption.sln

pause
exit 0

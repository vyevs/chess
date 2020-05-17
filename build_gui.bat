@echo off
cl /D _CRT_SECURE_NO_WARNINGS /I include\sdl sdl_gui.c chess.c chess_utils.c engine.c /W3 /DEBUG /Z7 /link SDL2.lib SDL2main.lib SDL2_image.lib SDL2_ttf.lib /LIBPATH:lib /SUBSYSTEM:CONSOLE 
set PATH=%PATH%;lib
del *.obj
del *.ilk
del *.pdb
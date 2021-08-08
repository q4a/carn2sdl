# Carn2-Menu-SDL

This branch is an experiment to port the Menu code over to use [SDL2](https://libsdl.org) to make it cross platform. If this works out, the next step is doing the same with th main Carnivores-CPE code using the software renderer first.

Please note: This version of menu writes SDL_SCANCODE_xxx key codes to the configuration file, meaning that remapping keys will screw up your in-game keymapping.
Until the Carnivores core is ported over to SDL, don't touch the keymaps ;)

# Building

Notes on building this project on different platforms:

## Windows

Normal Visual Studio build instructions apply, SDL distribution is included in repository.

## MacOS

Make sure to have the SDL2 Runtime & Development libraries/headers installed, for example using brew:

  brew install sdl2

# TODO

This lists the things that "got lost" in the conversion to SDL2, and need to be fixed before we're on equal footing with the original menu app.

* Set window icon (and fix window title handling)
* Sleeping when the game is running
* Handle mouse buttons in key mapping
* Use SDL_GetScancodeName for showing key names in keymapping
* Consider adding some kind of versioning field in the config file, to detect older ones and convert/reset keymap.

# Wishlist

These are items I'd like to see done, but are outside the SDL port

* More configurability (either by extending the _RES file or adding a seperate "menu config")
* Make it possible to use other image formats besides RGB565/TGA (SDL_Surface / SDL_Texture can abstract it all away)
* Remove custom image loading with SDL_img
* .... find time to rest :P

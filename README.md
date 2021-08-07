# Carn2-Menu-SDL

This branch is an experiment to port the Menu code over to use [SDL2](https://libsdl.org) to make it cross platform. If this works out, the next step is doing the same with th main Carnivores-CPE code using the software renderer first.

# Building

Notes on building this project on different platforms:

## Windows

Normal Visual Studio build instructions apply, SDL distribution is included in repository.

## MacOS

Make sure to have the SDL2 Runtime & Development libraries/headers installed, for example using brew:

  brew install sdl2

# Things left to do

This lists the things that "got lost" in the conversion to SDL2, and need to be fixed before we're on equal footing with the original menu app.

* Set window icon
* Sleeping when the game is running
* Handle mouse buttons in key mapping
* Decide on what to do with keycodes between the windows VK <=> SDL_SCANCODE_xxxx


#ifndef VERSION_H
#define VERSION_H

#define _WSTR(x) _WSTR_(x)
#define _WSTR_(x) L ## #x
#define _STR(x) _STR_(x)
#define _STR_(x) #x

// Build number. Incremented on each released build.
#define SIDE_BUILD_NUMBER 1

// Nightly defines GIT_COMMIT and GIT_BRANCH in GH Actions

#define SAVEGAME_ID ((SIDE_BUILD_NUMBER << 24) | (SIDE_BUILD_NUMBER << 12) | (SIDE_BUILD_NUMBER))
#define FILE_DESCRIPTION "Build of Phobos engine extension side project " _STR(SIDE_PROJECT_NAME)
#define FILE_VERSION_STR "Build #" _STR(SIDE_BUILD_NUMBER) " of side project " _STR(SIDE_PROJECT_NAME)
#define FILE_VERSION 0,0,0,SIDE_BUILD_NUMBER
#define PRODUCT_VERSION "Build #" _STR(SIDE_BUILD_NUMBER) " of side project " _STR(SIDE_PROJECT_NAME)

#endif // VERSION_H

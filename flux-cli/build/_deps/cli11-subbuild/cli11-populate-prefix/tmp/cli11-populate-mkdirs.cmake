# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/p20000415/Music/Flux-cpp/flux-cli/build/_deps/cli11-src")
  file(MAKE_DIRECTORY "C:/Users/p20000415/Music/Flux-cpp/flux-cli/build/_deps/cli11-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/p20000415/Music/Flux-cpp/flux-cli/build/_deps/cli11-build"
  "C:/Users/p20000415/Music/Flux-cpp/flux-cli/build/_deps/cli11-subbuild/cli11-populate-prefix"
  "C:/Users/p20000415/Music/Flux-cpp/flux-cli/build/_deps/cli11-subbuild/cli11-populate-prefix/tmp"
  "C:/Users/p20000415/Music/Flux-cpp/flux-cli/build/_deps/cli11-subbuild/cli11-populate-prefix/src/cli11-populate-stamp"
  "C:/Users/p20000415/Music/Flux-cpp/flux-cli/build/_deps/cli11-subbuild/cli11-populate-prefix/src"
  "C:/Users/p20000415/Music/Flux-cpp/flux-cli/build/_deps/cli11-subbuild/cli11-populate-prefix/src/cli11-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/p20000415/Music/Flux-cpp/flux-cli/build/_deps/cli11-subbuild/cli11-populate-prefix/src/cli11-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/p20000415/Music/Flux-cpp/flux-cli/build/_deps/cli11-subbuild/cli11-populate-prefix/src/cli11-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()

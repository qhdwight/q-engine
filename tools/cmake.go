package main

import (
	"encoding/json"
	"fmt"
	"github.com/go-git/go-git/v5"
	"io/ioutil"
	"os"
	"path/filepath"

	"github.com/go-git/go-git/v5/plumbing"
)

type Package struct {
	URI       string   `json:"uri"`
	Reference string   `json:"reference"`
	Includes  []string `json:"includes"`
	Libraries []string `json:"libraries"`
	Source    []string `json:"source"`
}

func main() {
	handleError := func(err error) {
		if err != nil {
			panic(err)
		}
	}

	pkgFile, err := os.Open("pkg.json")
	handleError(err)
	bytes, _ := ioutil.ReadAll(pkgFile)
	err = pkgFile.Close()
	handleError(err)

	var packages map[string]Package
	err = json.Unmarshal(bytes, &packages)
	handleError(err)

	for pkgName, pkg := range packages {
		path := filepath.Join("pkg", pkgName)
		repo, err := git.PlainOpen(path)
		if err == git.ErrRepositoryNotExists {
			repo, err = git.PlainClone(path, false, &git.CloneOptions{
				URL:           pkg.URI,
				Progress:      os.Stdout,
				SingleBranch:  true,
				ReferenceName: plumbing.ReferenceName(pkg.Reference),
			})
			handleError(err)
		} else if err != nil {
			handleError(err)
		}
		head, _ := repo.Head()
		fmt.Printf("[I] Package directory \"%s\" @%s\n", path, head)
		handleError(err)
	}

	cmakeFile, err := os.OpenFile(filepath.Join("src", "CMakeLists.txt"), os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0644)
	handleError(err)

	_, _ = cmakeFile.WriteString(`cmake_minimum_required(VERSION 3.10)
project(game)
set(CMAKE_CXX_STANDARD 20)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)

set(TINYGLTF_HEADER_ONLY ON CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE BOOL "" FORCE)

set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)

find_package(Vulkan REQUIRED)

`)
	_, _ = cmakeFile.WriteString(`file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "*.cpp")
`)
	for pkgName, pkg := range packages {
		for _, source := range pkg.Source {
			_, _ = cmakeFile.WriteString(fmt.Sprintf("list(APPEND SOURCE_FILES ../pkg/%s/%s)\n", pkgName, source))
		}
	}
	_, _ = cmakeFile.WriteString("add_executable(${PROJECT_NAME} ${SOURCE_FILES})\n\n")

	for pkgName := range packages {
		if _, err := os.Stat(filepath.Join("pkg", pkgName, "CMakeLists.txt")); err == nil {
			_, _ = cmakeFile.WriteString(fmt.Sprintf("add_subdirectory(../pkg/%[1]s ../build/%[1]s)\n", pkgName))
		}
	}

	_, _ = cmakeFile.WriteString(`
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR})
target_precompile_headers(${PROJECT_NAME} PRIVATE "game_pch.hpp" "../pkg/imgui/imgui.h")

`)

	for pkgName, pkg := range packages {
		for _, include := range pkg.Includes {
			_, _ = cmakeFile.WriteString(fmt.Sprintf("target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ../pkg/%s/%s)\n", pkgName, include))
		}
	}

	_, _ = cmakeFile.WriteString(`
if (MSVC)
    target_compile_options(Edyn PRIVATE /w)
endif ()
target_compile_definitions(Edyn PUBLIC EDYN_DOUBLE_PRECISION)
target_compile_definitions(tinygltf INTERFACE TINYGLTF_USE_CPP14)

`)
	for _, pkg := range packages {
		for _, library := range pkg.Libraries {
			_, _ = cmakeFile.WriteString(fmt.Sprintf("target_link_libraries(${PROJECT_NAME} %s)\n", library))
		}
	}
	_, _ = cmakeFile.WriteString(`
if (MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
else ()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif ()

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
target_compile_definitions(${PROJECT_NAME} PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX VK_USE_PLATFORM_WIN32_KHR)
elseif (APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC VK_USE_PLATFORM_MACOS_MVK)
elseif (UNIX)
	target_compile_definitions(${PROJECT_NAME} PUBLIC VK_USE_PLATFORM_XCB_KHR)
endif ()
`)

	err = cmakeFile.Close()
	handleError(err)
}

package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"regexp"
)

func rglob(dir string, ext string) ([]string, error) {
	files := []string{}
	err := filepath.Walk(dir, func(path string, f os.FileInfo, err error) error {
		if filepath.Ext(path) == ext {
			files = append(files, path)
		}
		return nil
	})
	return files, err
}

func main() {
	handleError := func(err error) {
		if err != nil {
			panic(err)
		}
	}

	matches, err := rglob(filepath.Join("..", "src"), ".hpp")
	handleError(err)

	for _, match := range matches {
		text, err := ioutil.ReadFile(match)
		r := regexp.MustCompile(`struct (.*) {([.\s\S]*)};`)
		struct_defs := r.FindAllString(string(text), -1)
		for _, struct_def := range struct_defs {
			fmt.Printf("struct_def: %s\n", struct_def)
		}
		handleError(err)
	}
}

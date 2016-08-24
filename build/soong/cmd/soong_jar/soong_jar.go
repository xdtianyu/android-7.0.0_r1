// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package main

import (
	"archive/zip"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
	"time"
)

type fileArg struct {
	relativeRoot, file string
}

type fileArgs []fileArg

func (l *fileArgs) String() string {
	return `""`
}

func (l *fileArgs) Set(s string) error {
	if *relativeRoot == "" {
		return fmt.Errorf("must pass -C before -f")
	}

	*l = append(*l, fileArg{*relativeRoot, s})
	return nil
}

func (l *fileArgs) Get() interface{} {
	return l
}

var (
	out          = flag.String("o", "", "file to write jar file to")
	manifest     = flag.String("m", "", "input manifest file name")
	directories  = flag.Bool("d", false, "include directories in jar")
	relativeRoot = flag.String("C", "", "path to use as relative root of files in next -f or -l argument")
	listFiles    fileArgs
	files        fileArgs
)

func init() {
	flag.Var(&listFiles, "l", "file containing list of .class files")
	flag.Var(&files, "f", "file to include in jar")
}

func usage() {
	fmt.Fprintf(os.Stderr, "usage: soong_jar -o jarfile [-m manifest] -C dir [-f|-l file]...\n")
	flag.PrintDefaults()
	os.Exit(2)
}

type zipWriter struct {
	time        time.Time
	createdDirs map[string]bool
	directories bool

	w *zip.Writer
}

func main() {
	flag.Parse()

	if *out == "" {
		fmt.Fprintf(os.Stderr, "error: -o is required\n")
		usage()
	}

	w := &zipWriter{
		time:        time.Now(),
		createdDirs: make(map[string]bool),
		directories: *directories,
	}

	// TODO: Go's zip implementation doesn't support increasing the compression level yet
	err := w.write(*out, listFiles, *manifest)
	if err != nil {
		fmt.Fprintln(os.Stderr, err.Error())
		os.Exit(1)
	}
}

func (z *zipWriter) write(out string, listFiles fileArgs, manifest string) error {
	f, err := os.Create(out)
	if err != nil {
		return err
	}

	defer f.Close()
	defer func() {
		if err != nil {
			os.Remove(out)
		}
	}()

	z.w = zip.NewWriter(f)
	defer z.w.Close()

	for _, listFile := range listFiles {
		err = z.writeListFile(listFile)
		if err != nil {
			return err
		}
	}

	for _, file := range files {
		err = z.writeRelFile(file.relativeRoot, file.file)
		if err != nil {
			return err
		}
	}

	if manifest != "" {
		err = z.writeFile("META-INF/MANIFEST.MF", manifest)
		if err != nil {
			return err
		}
	}

	return nil
}

func (z *zipWriter) writeListFile(listFile fileArg) error {
	list, err := ioutil.ReadFile(listFile.file)
	if err != nil {
		return err
	}

	files := strings.Split(string(list), "\n")

	for _, file := range files {
		file = strings.TrimSpace(file)
		if file == "" {
			continue
		}
		err = z.writeRelFile(listFile.relativeRoot, file)
		if err != nil {
			return err
		}
	}

	return nil
}

func (z *zipWriter) writeRelFile(root, file string) error {
	rel, err := filepath.Rel(root, file)
	if err != nil {
		return err
	}

	err = z.writeFile(rel, file)
	if err != nil {
		return err
	}

	return nil
}

func (z *zipWriter) writeFile(rel, file string) error {
	if s, _ := os.Stat(file); s.IsDir() {
		if z.directories {
			return z.writeDirectory(file)
		}
		return nil
	}

	if z.directories {
		dir, _ := filepath.Split(rel)
		err := z.writeDirectory(dir)
		if err != nil {
			return err
		}
	}

	fileHeader := &zip.FileHeader{
		Name:   rel,
		Method: zip.Deflate,
	}
	fileHeader.SetModTime(z.time)

	out, err := z.w.CreateHeader(fileHeader)
	if err != nil {
		return err
	}

	in, err := os.Open(file)
	if err != nil {
		return err
	}
	defer in.Close()

	_, err = io.Copy(out, in)
	if err != nil {
		return err
	}

	return nil
}

func (z *zipWriter) writeDirectory(dir string) error {
	for dir != "" && !z.createdDirs[dir] {
		z.createdDirs[dir] = true

		dirHeader := &zip.FileHeader{
			Name: dir,
		}
		dirHeader.SetMode(os.ModeDir)
		dirHeader.SetModTime(z.time)

		_, err := z.w.CreateHeader(dirHeader)
		if err != nil {
			return err
		}

		dir, _ = filepath.Split(dir)
	}

	return nil
}

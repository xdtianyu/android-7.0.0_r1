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

package cc

// This file generates the final rules for compiling all C/C++.  All properties related to
// compiling should have been translated into builderFlags or another argument to the Transform*
// functions.

import (
	"android/soong/common"
	"fmt"
	"runtime"
	"strconv"

	"path/filepath"
	"strings"

	"github.com/google/blueprint"
)

const (
	objectExtension        = ".o"
	staticLibraryExtension = ".a"
)

var (
	pctx = common.NewPackageContext("android/soong/cc")

	cc = pctx.StaticRule("cc",
		blueprint.RuleParams{
			Depfile:     "${out}.d",
			Deps:        blueprint.DepsGCC,
			Command:     "$relPwd $ccCmd -c $cFlags -MD -MF ${out}.d -o $out $in",
			CommandDeps: []string{"$ccCmd"},
			Description: "cc $out",
		},
		"ccCmd", "cFlags")

	ld = pctx.StaticRule("ld",
		blueprint.RuleParams{
			Command: "$ldCmd ${ldDirFlags} ${crtBegin} @${out}.rsp " +
				"${libFlags} ${crtEnd} -o ${out} ${ldFlags}",
			CommandDeps:    []string{"$ldCmd"},
			Description:    "ld $out",
			Rspfile:        "${out}.rsp",
			RspfileContent: "${in}",
		},
		"ldCmd", "ldDirFlags", "crtBegin", "libFlags", "crtEnd", "ldFlags")

	partialLd = pctx.StaticRule("partialLd",
		blueprint.RuleParams{
			Command:     "$ldCmd -nostdlib -Wl,-r ${in} -o ${out} ${ldFlags}",
			CommandDeps: []string{"$ldCmd"},
			Description: "partialLd $out",
		},
		"ldCmd", "ldFlags")

	ar = pctx.StaticRule("ar",
		blueprint.RuleParams{
			Command:        "rm -f ${out} && $arCmd $arFlags $out @${out}.rsp",
			CommandDeps:    []string{"$arCmd"},
			Description:    "ar $out",
			Rspfile:        "${out}.rsp",
			RspfileContent: "${in}",
		},
		"arCmd", "arFlags")

	darwinAr = pctx.StaticRule("darwinAr",
		blueprint.RuleParams{
			Command:     "rm -f ${out} && $arCmd $arFlags $out $in",
			CommandDeps: []string{"$arCmd"},
			Description: "ar $out",
		},
		"arCmd", "arFlags")

	darwinAppendAr = pctx.StaticRule("darwinAppendAr",
		blueprint.RuleParams{
			Command:     "cp -f ${inAr} ${out}.tmp && $arCmd $arFlags ${out}.tmp $in && mv ${out}.tmp ${out}",
			CommandDeps: []string{"$arCmd"},
			Description: "ar $out",
		},
		"arCmd", "arFlags", "inAr")

	prefixSymbols = pctx.StaticRule("prefixSymbols",
		blueprint.RuleParams{
			Command:     "$objcopyCmd --prefix-symbols=${prefix} ${in} ${out}",
			CommandDeps: []string{"$objcopyCmd"},
			Description: "prefixSymbols $out",
		},
		"objcopyCmd", "prefix")

	copyGccLibPath = pctx.SourcePathVariable("copyGccLibPath", "build/soong/copygcclib.sh")

	copyGccLib = pctx.StaticRule("copyGccLib",
		blueprint.RuleParams{
			Depfile:     "${out}.d",
			Deps:        blueprint.DepsGCC,
			Command:     "$copyGccLibPath $out $ccCmd $cFlags -print-file-name=${libName}",
			CommandDeps: []string{"$copyGccLibPath", "$ccCmd"},
			Description: "copy gcc $out",
		},
		"ccCmd", "cFlags", "libName")
)

func init() {
	// We run gcc/clang with PWD=/proc/self/cwd to remove $TOP from the
	// debug output. That way two builds in two different directories will
	// create the same output.
	if runtime.GOOS != "darwin" {
		pctx.StaticVariable("relPwd", "PWD=/proc/self/cwd")
	} else {
		// Darwin doesn't have /proc
		pctx.StaticVariable("relPwd", "")
	}
}

type builderFlags struct {
	globalFlags string
	asFlags     string
	cFlags      string
	conlyFlags  string
	cppFlags    string
	ldFlags     string
	yaccFlags   string
	nocrt       bool
	toolchain   Toolchain
	clang       bool
}

// Generate rules for compiling multiple .c, .cpp, or .S files to individual .o files
func TransformSourceToObj(ctx common.AndroidModuleContext, subdir string, srcFiles common.Paths,
	flags builderFlags, deps common.Paths) (objFiles common.Paths) {

	objFiles = make(common.Paths, len(srcFiles))

	cflags := flags.globalFlags + " " + flags.cFlags + " " + flags.conlyFlags
	cppflags := flags.globalFlags + " " + flags.cFlags + " " + flags.cppFlags
	asflags := flags.globalFlags + " " + flags.asFlags

	if flags.clang {
		cflags += " ${noOverrideClangGlobalCflags}"
		cppflags += " ${noOverrideClangGlobalCflags}"
	} else {
		cflags += " ${noOverrideGlobalCflags}"
		cppflags += " ${noOverrideGlobalCflags}"
	}

	for i, srcFile := range srcFiles {
		objFile := common.ObjPathWithExt(ctx, srcFile, subdir, "o")

		objFiles[i] = objFile

		var moduleCflags string
		var ccCmd string

		switch srcFile.Ext() {
		case ".S", ".s":
			ccCmd = "gcc"
			moduleCflags = asflags
		case ".c":
			ccCmd = "gcc"
			moduleCflags = cflags
		case ".cpp", ".cc":
			ccCmd = "g++"
			moduleCflags = cppflags
		default:
			ctx.ModuleErrorf("File %s has unknown extension", srcFile)
			continue
		}

		if flags.clang {
			switch ccCmd {
			case "gcc":
				ccCmd = "clang"
			case "g++":
				ccCmd = "clang++"
			default:
				panic("unrecoginzied ccCmd")
			}

			ccCmd = "${clangPath}/" + ccCmd
		} else {
			ccCmd = gccCmd(flags.toolchain, ccCmd)
		}

		ctx.ModuleBuild(pctx, common.ModuleBuildParams{
			Rule:      cc,
			Output:    objFile,
			Input:     srcFile,
			Implicits: deps,
			Args: map[string]string{
				"cFlags": moduleCflags,
				"ccCmd":  ccCmd,
			},
		})
	}

	return objFiles
}

// Generate a rule for compiling multiple .o files to a static library (.a)
func TransformObjToStaticLib(ctx common.AndroidModuleContext, objFiles common.Paths,
	flags builderFlags, outputFile common.ModuleOutPath) {

	arCmd := gccCmd(flags.toolchain, "ar")
	arFlags := "crsPD"

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:   ar,
		Output: outputFile,
		Inputs: objFiles,
		Args: map[string]string{
			"arFlags": arFlags,
			"arCmd":   arCmd,
		},
	})
}

// Generate a rule for compiling multiple .o files to a static library (.a) on
// darwin.  The darwin ar tool doesn't support @file for list files, and has a
// very small command line length limit, so we have to split the ar into multiple
// steps, each appending to the previous one.
func TransformDarwinObjToStaticLib(ctx common.AndroidModuleContext, objFiles common.Paths,
	flags builderFlags, outputPath common.ModuleOutPath) {

	arCmd := "${macArPath}"
	arFlags := "cqs"

	// ARG_MAX on darwin is 262144, use half that to be safe
	objFilesLists, err := splitListForSize(objFiles.Strings(), 131072)
	if err != nil {
		ctx.ModuleErrorf("%s", err.Error())
	}

	outputFile := outputPath.String()

	var in, out string
	for i, l := range objFilesLists {
		in = out
		out = outputFile
		if i != len(objFilesLists)-1 {
			out += "." + strconv.Itoa(i)
		}

		if in == "" {
			ctx.Build(pctx, blueprint.BuildParams{
				Rule:    darwinAr,
				Outputs: []string{out},
				Inputs:  l,
				Args: map[string]string{
					"arFlags": arFlags,
					"arCmd":   arCmd,
				},
			})
		} else {
			ctx.Build(pctx, blueprint.BuildParams{
				Rule:      darwinAppendAr,
				Outputs:   []string{out},
				Inputs:    l,
				Implicits: []string{in},
				Args: map[string]string{
					"arFlags": arFlags,
					"arCmd":   arCmd,
					"inAr":    in,
				},
			})
		}
	}
}

// Generate a rule for compiling multiple .o files, plus static libraries, whole static libraries,
// and shared libraires, to a shared library (.so) or dynamic executable
func TransformObjToDynamicBinary(ctx common.AndroidModuleContext,
	objFiles, sharedLibs, staticLibs, lateStaticLibs, wholeStaticLibs, deps common.Paths,
	crtBegin, crtEnd common.OptionalPath, groupLate bool, flags builderFlags, outputFile common.WritablePath) {

	var ldCmd string
	if flags.clang {
		ldCmd = "${clangPath}/clang++"
	} else {
		ldCmd = gccCmd(flags.toolchain, "g++")
	}

	var ldDirs []string
	var libFlagsList []string

	if len(wholeStaticLibs) > 0 {
		if ctx.Host() && ctx.Darwin() {
			libFlagsList = append(libFlagsList, common.JoinWithPrefix(wholeStaticLibs.Strings(), "-force_load "))
		} else {
			libFlagsList = append(libFlagsList, "-Wl,--whole-archive ")
			libFlagsList = append(libFlagsList, wholeStaticLibs.Strings()...)
			libFlagsList = append(libFlagsList, "-Wl,--no-whole-archive ")
		}
	}

	libFlagsList = append(libFlagsList, staticLibs.Strings()...)

	if groupLate && len(lateStaticLibs) > 0 {
		libFlagsList = append(libFlagsList, "-Wl,--start-group")
	}
	libFlagsList = append(libFlagsList, lateStaticLibs.Strings()...)
	if groupLate && len(lateStaticLibs) > 0 {
		libFlagsList = append(libFlagsList, "-Wl,--end-group")
	}

	for _, lib := range sharedLibs {
		dir, file := filepath.Split(lib.String())
		if !strings.HasPrefix(file, "lib") {
			panic("shared library " + lib.String() + " does not start with lib")
		}
		if !strings.HasSuffix(file, flags.toolchain.ShlibSuffix()) {
			panic("shared library " + lib.String() + " does not end with " + flags.toolchain.ShlibSuffix())
		}
		libFlagsList = append(libFlagsList,
			"-l"+strings.TrimSuffix(strings.TrimPrefix(file, "lib"), flags.toolchain.ShlibSuffix()))
		ldDirs = append(ldDirs, dir)
	}

	deps = append(deps, sharedLibs...)
	deps = append(deps, staticLibs...)
	deps = append(deps, lateStaticLibs...)
	deps = append(deps, wholeStaticLibs...)
	if crtBegin.Valid() {
		deps = append(deps, crtBegin.Path(), crtEnd.Path())
	}

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:      ld,
		Output:    outputFile,
		Inputs:    objFiles,
		Implicits: deps,
		Args: map[string]string{
			"ldCmd":      ldCmd,
			"ldDirFlags": ldDirsToFlags(ldDirs),
			"crtBegin":   crtBegin.String(),
			"libFlags":   strings.Join(libFlagsList, " "),
			"ldFlags":    flags.ldFlags,
			"crtEnd":     crtEnd.String(),
		},
	})
}

// Generate a rule for compiling multiple .o files to a .o using ld partial linking
func TransformObjsToObj(ctx common.AndroidModuleContext, objFiles common.Paths,
	flags builderFlags, outputFile common.WritablePath) {

	var ldCmd string
	if flags.clang {
		ldCmd = "${clangPath}clang++"
	} else {
		ldCmd = gccCmd(flags.toolchain, "g++")
	}

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:   partialLd,
		Output: outputFile,
		Inputs: objFiles,
		Args: map[string]string{
			"ldCmd":   ldCmd,
			"ldFlags": flags.ldFlags,
		},
	})
}

// Generate a rule for runing objcopy --prefix-symbols on a binary
func TransformBinaryPrefixSymbols(ctx common.AndroidModuleContext, prefix string, inputFile common.Path,
	flags builderFlags, outputFile common.WritablePath) {

	objcopyCmd := gccCmd(flags.toolchain, "objcopy")

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:   prefixSymbols,
		Output: outputFile,
		Input:  inputFile,
		Args: map[string]string{
			"objcopyCmd": objcopyCmd,
			"prefix":     prefix,
		},
	})
}

func CopyGccLib(ctx common.AndroidModuleContext, libName string,
	flags builderFlags, outputFile common.WritablePath) {

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:   copyGccLib,
		Output: outputFile,
		Args: map[string]string{
			"ccCmd":   gccCmd(flags.toolchain, "gcc"),
			"cFlags":  flags.globalFlags,
			"libName": libName,
		},
	})
}

func gccCmd(toolchain Toolchain, cmd string) string {
	return filepath.Join(toolchain.GccRoot(), "bin", toolchain.GccTriple()+"-"+cmd)
}

func splitListForSize(list []string, limit int) (lists [][]string, err error) {
	var i int

	start := 0
	bytes := 0
	for i = range list {
		l := len(list[i])
		if l > limit {
			return nil, fmt.Errorf("list element greater than size limit (%d)", limit)
		}
		if bytes+l > limit {
			lists = append(lists, list[start:i])
			start = i
			bytes = 0
		}
		bytes += l + 1 // count a space between each list element
	}

	lists = append(lists, list[start:])

	totalLen := 0
	for _, l := range lists {
		totalLen += len(l)
	}
	if totalLen != len(list) {
		panic(fmt.Errorf("Failed breaking up list, %d != %d", len(list), totalLen))
	}
	return lists, nil
}

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

package java

// This file generates the final rules for compiling all Java.  All properties related to
// compiling should have been translated into javaBuilderFlags or another argument to the Transform*
// functions.

import (
	"path/filepath"
	"strings"

	"android/soong/common"

	"github.com/google/blueprint"
	_ "github.com/google/blueprint/bootstrap"
)

var (
	pctx = common.NewPackageContext("android/soong/java")

	// Compiling java is not conducive to proper dependency tracking.  The path-matches-class-name
	// requirement leads to unpredictable generated source file names, and a single .java file
	// will get compiled into multiple .class files if it contains inner classes.  To work around
	// this, all java rules write into separate directories and then a post-processing step lists
	// the files in the the directory into a list file that later rules depend on (and sometimes
	// read from directly using @<listfile>)
	javac = pctx.StaticRule("javac",
		blueprint.RuleParams{
			Command: `rm -rf "$outDir" && mkdir -p "$outDir" && ` +
				`$javacCmd -encoding UTF-8 $javacFlags $bootClasspath $classpath ` +
				`-extdirs "" -d $outDir @$out.rsp || ( rm -rf "$outDir"; exit 41 ) && ` +
				`find $outDir -name "*.class" > $out`,
			Rspfile:        "$out.rsp",
			RspfileContent: "$in",
			Description:    "javac $outDir",
		},
		"javacCmd", "javacFlags", "bootClasspath", "classpath", "outDir")

	jar = pctx.StaticRule("jar",
		blueprint.RuleParams{
			Command:     `$jarCmd -o $out $jarArgs`,
			CommandDeps: []string{"$jarCmd"},
			Description: "jar $out",
		},
		"jarCmd", "jarArgs")

	dx = pctx.StaticRule("dx",
		blueprint.RuleParams{
			Command: `rm -rf "$outDir" && mkdir -p "$outDir" && ` +
				`$dxCmd --dex --output=$outDir $dxFlags $in || ( rm -rf "$outDir"; exit 41 ) && ` +
				`find "$outDir" -name "classes*.dex" > $out`,
			CommandDeps: []string{"$dxCmd"},
			Description: "dex $out",
		},
		"outDir", "dxFlags")

	jarjar = pctx.StaticRule("jarjar",
		blueprint.RuleParams{
			Command:     "java -jar $jarjarCmd process $rulesFile $in $out",
			CommandDeps: []string{"$jarjarCmd", "$rulesFile"},
			Description: "jarjar $out",
		},
		"rulesFile")

	extractPrebuilt = pctx.StaticRule("extractPrebuilt",
		blueprint.RuleParams{
			Command: `rm -rf $outDir && unzip -qo $in -d $outDir && ` +
				`find $outDir -name "*.class" > $classFile && ` +
				`find $outDir -type f -a \! -name "*.class" -a \! -name "MANIFEST.MF" > $resourceFile || ` +
				`(rm -rf $outDir; exit 42)`,
			Description: "extract java prebuilt $outDir",
		},
		"outDir", "classFile", "resourceFile")
)

func init() {
	pctx.Import("github.com/google/blueprint/bootstrap")
	pctx.StaticVariable("commonJdkFlags", "-source 1.7 -target 1.7 -Xmaxerrs 9999999")
	pctx.StaticVariable("javacCmd", "javac -J-Xmx1024M $commonJdkFlags")
	pctx.StaticVariable("jarCmd", filepath.Join("${bootstrap.BinDir}", "soong_jar"))
	pctx.HostBinToolVariable("dxCmd", "dx")
	pctx.HostJavaToolVariable("jarjarCmd", "jarjar.jar")
}

type javaBuilderFlags struct {
	javacFlags    string
	dxFlags       string
	bootClasspath string
	classpath     string
	aidlFlags     string
}

type jarSpec struct {
	fileList, dir common.Path
}

func (j jarSpec) soongJarArgs() string {
	return "-C " + j.dir.String() + " -l " + j.fileList.String()
}

func TransformJavaToClasses(ctx common.AndroidModuleContext, srcFiles common.Paths, srcFileLists common.Paths,
	flags javaBuilderFlags, deps common.Paths) jarSpec {

	classDir := common.PathForModuleOut(ctx, "classes")
	classFileList := common.PathForModuleOut(ctx, "classes.list")

	javacFlags := flags.javacFlags + common.JoinWithPrefix(srcFileLists.Strings(), "@")

	deps = append(deps, srcFileLists...)

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:      javac,
		Output:    classFileList,
		Inputs:    srcFiles,
		Implicits: deps,
		Args: map[string]string{
			"javacFlags":    javacFlags,
			"bootClasspath": flags.bootClasspath,
			"classpath":     flags.classpath,
			"outDir":        classDir.String(),
		},
	})

	return jarSpec{classFileList, classDir}
}

func TransformClassesToJar(ctx common.AndroidModuleContext, classes []jarSpec,
	manifest common.OptionalPath) common.Path {

	outputFile := common.PathForModuleOut(ctx, "classes-full-debug.jar")

	deps := common.Paths{}
	jarArgs := []string{}

	for _, j := range classes {
		deps = append(deps, j.fileList)
		jarArgs = append(jarArgs, j.soongJarArgs())
	}

	if manifest.Valid() {
		deps = append(deps, manifest.Path())
		jarArgs = append(jarArgs, "-m "+manifest.String())
	}

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:      jar,
		Output:    outputFile,
		Implicits: deps,
		Args: map[string]string{
			"jarArgs": strings.Join(jarArgs, " "),
		},
	})

	return outputFile
}

func TransformClassesJarToDex(ctx common.AndroidModuleContext, classesJar common.Path,
	flags javaBuilderFlags) jarSpec {

	outDir := common.PathForModuleOut(ctx, "dex")
	outputFile := common.PathForModuleOut(ctx, "dex.filelist")

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:   dx,
		Output: outputFile,
		Input:  classesJar,
		Args: map[string]string{
			"dxFlags": flags.dxFlags,
			"outDir":  outDir.String(),
		},
	})

	return jarSpec{outputFile, outDir}
}

func TransformDexToJavaLib(ctx common.AndroidModuleContext, resources []jarSpec,
	dexJarSpec jarSpec) common.Path {

	outputFile := common.PathForModuleOut(ctx, "javalib.jar")
	var deps common.Paths
	var jarArgs []string

	for _, j := range resources {
		deps = append(deps, j.fileList)
		jarArgs = append(jarArgs, j.soongJarArgs())
	}

	deps = append(deps, dexJarSpec.fileList)
	jarArgs = append(jarArgs, dexJarSpec.soongJarArgs())

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:      jar,
		Output:    outputFile,
		Implicits: deps,
		Args: map[string]string{
			"jarArgs": strings.Join(jarArgs, " "),
		},
	})

	return outputFile
}

func TransformJarJar(ctx common.AndroidModuleContext, classesJar common.Path, rulesFile common.Path) common.Path {
	outputFile := common.PathForModuleOut(ctx, "classes-jarjar.jar")
	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:     jarjar,
		Output:   outputFile,
		Input:    classesJar,
		Implicit: rulesFile,
		Args: map[string]string{
			"rulesFile": rulesFile.String(),
		},
	})

	return outputFile
}

func TransformPrebuiltJarToClasses(ctx common.AndroidModuleContext,
	prebuilt common.Path) (classJarSpec, resourceJarSpec jarSpec) {

	classDir := common.PathForModuleOut(ctx, "extracted/classes")
	classFileList := common.PathForModuleOut(ctx, "extracted/classes.list")
	resourceFileList := common.PathForModuleOut(ctx, "extracted/resources.list")

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:    extractPrebuilt,
		Outputs: common.WritablePaths{classFileList, resourceFileList},
		Input:   prebuilt,
		Args: map[string]string{
			"outDir":       classDir.String(),
			"classFile":    classFileList.String(),
			"resourceFile": resourceFileList.String(),
		},
	})

	return jarSpec{classFileList, classDir}, jarSpec{resourceFileList, classDir}
}

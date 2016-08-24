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
	"strings"

	"github.com/google/blueprint"

	"android/soong/common"
)

var (
	aaptCreateResourceJavaFile = pctx.StaticRule("aaptCreateResourceJavaFile",
		blueprint.RuleParams{
			Command: `rm -rf "$javaDir" && mkdir -p "$javaDir" && ` +
				`$aaptCmd package -m $aaptFlags -P $publicResourcesFile -G $proguardOptionsFile ` +
				`-J $javaDir || ( rm -rf "$javaDir/*"; exit 41 ) && ` +
				`find $javaDir -name "*.java" > $javaFileList`,
			CommandDeps: []string{"$aaptCmd"},
			Description: "aapt create R.java $out",
		},
		"aaptFlags", "publicResourcesFile", "proguardOptionsFile", "javaDir", "javaFileList")

	aaptCreateAssetsPackage = pctx.StaticRule("aaptCreateAssetsPackage",
		blueprint.RuleParams{
			Command:     `rm -f $out && $aaptCmd package $aaptFlags -F $out`,
			CommandDeps: []string{"$aaptCmd"},
			Description: "aapt export package $out",
		},
		"aaptFlags", "publicResourcesFile", "proguardOptionsFile", "javaDir", "javaFileList")

	aaptAddResources = pctx.StaticRule("aaptAddResources",
		blueprint.RuleParams{
			// TODO: add-jni-shared-libs-to-package
			Command:     `cp -f $in $out.tmp && $aaptCmd package -u $aaptFlags -F $out.tmp && mv $out.tmp $out`,
			CommandDeps: []string{"$aaptCmd"},
			Description: "aapt package $out",
		},
		"aaptFlags")

	signapk = pctx.StaticRule("signapk",
		blueprint.RuleParams{
			Command:     `java -jar $signapkCmd $certificates $in $out`,
			CommandDeps: []string{"$signapkCmd"},
			Description: "signapk $out",
		},
		"certificates")

	androidManifestMerger = pctx.StaticRule("androidManifestMerger",
		blueprint.RuleParams{
			Command: "java -classpath $androidManifestMergerCmd com.android.manifmerger.Main merge " +
				"--main $in --libs $libsManifests --out $out",
			CommandDeps: []string{"$androidManifestMergerCmd"},
			Description: "merge manifest files $out",
		},
		"libsManifests")
)

func init() {
	pctx.SourcePathVariable("androidManifestMergerCmd", "prebuilts/devtools/tools/lib/manifest-merger.jar")
	pctx.HostBinToolVariable("aaptCmd", "aapt")
	pctx.HostJavaToolVariable("signapkCmd", "signapk.jar")
}

func CreateResourceJavaFiles(ctx common.AndroidModuleContext, flags []string,
	deps common.Paths) (common.Path, common.Path, common.Path) {
	javaDir := common.PathForModuleGen(ctx, "R")
	javaFileList := common.PathForModuleOut(ctx, "R.filelist")
	publicResourcesFile := common.PathForModuleOut(ctx, "public_resources.xml")
	proguardOptionsFile := common.PathForModuleOut(ctx, "proguard.options")

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:      aaptCreateResourceJavaFile,
		Outputs:   common.WritablePaths{publicResourcesFile, proguardOptionsFile, javaFileList},
		Implicits: deps,
		Args: map[string]string{
			"aaptFlags":           strings.Join(flags, " "),
			"publicResourcesFile": publicResourcesFile.String(),
			"proguardOptionsFile": proguardOptionsFile.String(),
			"javaDir":             javaDir.String(),
			"javaFileList":        javaFileList.String(),
		},
	})

	return publicResourcesFile, proguardOptionsFile, javaFileList
}

func CreateExportPackage(ctx common.AndroidModuleContext, flags []string, deps common.Paths) common.ModuleOutPath {
	outputFile := common.PathForModuleOut(ctx, "package-export.apk")

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:      aaptCreateAssetsPackage,
		Output:    outputFile,
		Implicits: deps,
		Args: map[string]string{
			"aaptFlags": strings.Join(flags, " "),
		},
	})

	return outputFile
}

func CreateAppPackage(ctx common.AndroidModuleContext, flags []string, jarFile common.Path,
	certificates []string) common.Path {

	resourceApk := common.PathForModuleOut(ctx, "resources.apk")

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:   aaptAddResources,
		Output: resourceApk,
		Input:  jarFile,
		Args: map[string]string{
			"aaptFlags": strings.Join(flags, " "),
		},
	})

	outputFile := common.PathForModuleOut(ctx, "package.apk")

	var certificateArgs []string
	for _, c := range certificates {
		certificateArgs = append(certificateArgs, c+".x509.pem", c+".pk8")
	}

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:   signapk,
		Output: outputFile,
		Input:  resourceApk,
		Args: map[string]string{
			"certificates": strings.Join(certificateArgs, " "),
		},
	})

	return outputFile
}

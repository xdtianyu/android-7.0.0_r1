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
	"github.com/google/blueprint"

	"android/soong/common"
)

func init() {
	pctx.SourcePathVariable("lexCmd", "prebuilts/misc/${HostPrebuiltTag}/flex/flex-2.5.39")
	pctx.SourcePathVariable("yaccCmd", "prebuilts/misc/${HostPrebuiltTag}/bison/bison")
	pctx.SourcePathVariable("yaccDataDir", "external/bison/data")
}

var (
	yacc = pctx.StaticRule("yacc",
		blueprint.RuleParams{
			Command:     "BISON_PKGDATADIR=$yaccDataDir $yaccCmd -d $yaccFlags --defines=$hFile -o $cFile $in",
			CommandDeps: []string{"$yaccCmd"},
			Description: "yacc $out",
		},
		"yaccFlags", "cFile", "hFile")

	lex = pctx.StaticRule("lex",
		blueprint.RuleParams{
			Command:     "$lexCmd -o$out $in",
			CommandDeps: []string{"$lexCmd"},
			Description: "lex $out",
		})
)

func genYacc(ctx common.AndroidModuleContext, yaccFile common.Path, outFile common.ModuleGenPath, yaccFlags string) (headerFile common.ModuleGenPath) {
	headerFile = common.GenPathWithExt(ctx, yaccFile, "h")

	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:    yacc,
		Outputs: common.WritablePaths{outFile, headerFile},
		Input:   yaccFile,
		Args: map[string]string{
			"yaccFlags": yaccFlags,
			"cFile":     outFile.String(),
			"hFile":     headerFile.String(),
		},
	})

	return headerFile
}

func genLex(ctx common.AndroidModuleContext, lexFile common.Path, outFile common.ModuleGenPath) {
	ctx.ModuleBuild(pctx, common.ModuleBuildParams{
		Rule:   lex,
		Output: outFile,
		Input:  lexFile,
	})
}

func genSources(ctx common.AndroidModuleContext, srcFiles common.Paths,
	buildFlags builderFlags) (common.Paths, common.Paths) {

	var deps common.Paths

	for i, srcFile := range srcFiles {
		switch srcFile.Ext() {
		case ".y":
			cFile := common.GenPathWithExt(ctx, srcFile, "c")
			srcFiles[i] = cFile
			deps = append(deps, genYacc(ctx, srcFile, cFile, buildFlags.yaccFlags))
		case ".yy":
			cppFile := common.GenPathWithExt(ctx, srcFile, "cpp")
			srcFiles[i] = cppFile
			deps = append(deps, genYacc(ctx, srcFile, cppFile, buildFlags.yaccFlags))
		case ".l":
			cFile := common.GenPathWithExt(ctx, srcFile, "c")
			srcFiles[i] = cFile
			genLex(ctx, srcFile, cFile)
		case ".ll":
			cppFile := common.GenPathWithExt(ctx, srcFile, "cpp")
			srcFiles[i] = cppFile
			genLex(ctx, srcFile, cppFile)
		}
	}

	return srcFiles, deps
}

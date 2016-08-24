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

package common

import (
	"fmt"
	"path/filepath"

	"github.com/google/blueprint"

	"android/soong/glob"
)

// This file supports globbing source files in Blueprints files.
//
// The build.ninja file needs to be regenerated any time a file matching the glob is added
// or removed.  The naive solution is to have the build.ninja file depend on all the
// traversed directories, but this will cause the regeneration step to run every time a
// non-matching file is added to a traversed directory, including backup files created by
// editors.
//
// The solution implemented here optimizes out regenerations when the directory modifications
// don't match the glob by having the build.ninja file depend on an intermedate file that
// is only updated when a file matching the glob is added or removed.  The intermediate file
// depends on the traversed directories via a depfile.  The depfile is used to avoid build
// errors if a directory is deleted - a direct dependency on the deleted directory would result
// in a build failure with a "missing and no known rule to make it" error.

var (
	globCmd = filepath.Join("${bootstrap.BinDir}", "soong_glob")

	// globRule rule traverses directories to produce a list of files that match $glob
	// and writes it to $out if it has changed, and writes the directories to $out.d
	globRule = pctx.StaticRule("globRule",
		blueprint.RuleParams{
			Command:     fmt.Sprintf(`%s -o $out $excludes "$glob"`, globCmd),
			CommandDeps: []string{globCmd},
			Description: "glob $glob",

			Restat:  true,
			Deps:    blueprint.DepsGCC,
			Depfile: "$out.d",
		},
		"glob", "excludes")
)

func hasGlob(in []string) bool {
	for _, s := range in {
		if glob.IsGlob(s) {
			return true
		}
	}

	return false
}

// The subset of ModuleContext and SingletonContext needed by Glob
type globContext interface {
	Build(pctx blueprint.PackageContext, params blueprint.BuildParams)
	AddNinjaFileDeps(deps ...string)
}

func Glob(ctx globContext, outDir string, globPattern string, excludes []string) ([]string, error) {
	fileListFile := filepath.Join(outDir, "glob", globToString(globPattern))
	depFile := fileListFile + ".d"

	// Get a globbed file list, and write out fileListFile and depFile
	files, err := glob.GlobWithDepFile(globPattern, fileListFile, depFile, excludes)
	if err != nil {
		return nil, err
	}

	GlobRule(ctx, globPattern, excludes, fileListFile, depFile)

	// Make build.ninja depend on the fileListFile
	ctx.AddNinjaFileDeps(fileListFile)

	return files, nil
}

func GlobRule(ctx globContext, globPattern string, excludes []string,
	fileListFile, depFile string) {

	// Create a rule to rebuild fileListFile if a directory in depFile changes.  fileListFile
	// will only be rewritten if it has changed, preventing unnecesary build.ninja regenerations.
	ctx.Build(pctx, blueprint.BuildParams{
		Rule:    globRule,
		Outputs: []string{fileListFile},
		Args: map[string]string{
			"glob":     globPattern,
			"excludes": JoinWithPrefixAndQuote(excludes, "-e "),
		},
	})
}

func globToString(glob string) string {
	ret := ""
	for _, c := range glob {
		if c >= 'a' && c <= 'z' ||
			c >= 'A' && c <= 'Z' ||
			c >= '0' && c <= '9' ||
			c == '_' || c == '-' || c == '/' {
			ret += string(c)
		}
	}

	return ret
}

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

import (
	"fmt"
	"io"
	"path/filepath"
	"strings"

	"android/soong/common"
)

func (c *Module) AndroidMk() (ret common.AndroidMkData, err error) {
	ret.OutputFile = c.outputFile
	ret.Extra = append(ret.Extra, func(w io.Writer, outputFile common.Path) (err error) {
		if len(c.deps.SharedLibs) > 0 {
			fmt.Fprintln(w, "LOCAL_SHARED_LIBRARIES := "+strings.Join(c.deps.SharedLibs, " "))
		}
		return nil
	})

	callSubAndroidMk := func(obj interface{}) {
		if obj != nil {
			if androidmk, ok := obj.(interface {
				AndroidMk(*common.AndroidMkData)
			}); ok {
				androidmk.AndroidMk(&ret)
			}
		}
	}

	for _, feature := range c.features {
		callSubAndroidMk(feature)
	}

	callSubAndroidMk(c.compiler)
	callSubAndroidMk(c.linker)
	callSubAndroidMk(c.installer)

	return ret, nil
}

func (library *baseLinker) AndroidMk(ret *common.AndroidMkData) {
	if library.static() {
		ret.Class = "STATIC_LIBRARIES"
	} else {
		ret.Class = "SHARED_LIBRARIES"
	}
}

func (library *libraryLinker) AndroidMk(ret *common.AndroidMkData) {
	library.baseLinker.AndroidMk(ret)

	ret.Extra = append(ret.Extra, func(w io.Writer, outputFile common.Path) error {
		exportedIncludes := library.exportedFlags()
		for _, flag := range library.exportedFlags() {
			if flag != "" {
				exportedIncludes = append(exportedIncludes, strings.TrimPrefix(flag, "-I"))
			}
		}
		if len(exportedIncludes) > 0 {
			fmt.Fprintln(w, "LOCAL_EXPORT_C_INCLUDE_DIRS :=", strings.Join(exportedIncludes, " "))
		}

		fmt.Fprintln(w, "LOCAL_MODULE_SUFFIX := "+outputFile.Ext())

		// These are already included in LOCAL_SHARED_LIBRARIES
		fmt.Fprintln(w, "LOCAL_CXX_STL := none")
		fmt.Fprintln(w, "LOCAL_SYSTEM_SHARED_LIBRARIES :=")

		return nil
	})
}

func (object *objectLinker) AndroidMk(ret *common.AndroidMkData) {
	ret.Custom = func(w io.Writer, name, prefix string) error {
		out := ret.OutputFile.Path()

		fmt.Fprintln(w, "\n$("+prefix+"OUT_INTERMEDIATE_LIBRARIES)/"+name+objectExtension+":", out.String(), "| $(ACP)")
		fmt.Fprintln(w, "\t$(copy-file-to-target)")

		return nil
	}
}

func (binary *binaryLinker) AndroidMk(ret *common.AndroidMkData) {
	ret.Class = "EXECUTABLES"
	ret.Extra = append(ret.Extra, func(w io.Writer, outputFile common.Path) error {
		fmt.Fprintln(w, "LOCAL_CXX_STL := none")
		fmt.Fprintln(w, "LOCAL_SYSTEM_SHARED_LIBRARIES :=")
		return nil
	})
}

func (test *testLinker) AndroidMk(ret *common.AndroidMkData) {
	test.binaryLinker.AndroidMk(ret)
	if Bool(test.Properties.Test_per_src) {
		ret.SubName = test.binaryLinker.Properties.Stem
	}
}

func (installer *baseInstaller) AndroidMk(ret *common.AndroidMkData) {
	ret.Extra = append(ret.Extra, func(w io.Writer, outputFile common.Path) error {
		path := installer.path.RelPathString()
		dir, file := filepath.Split(path)
		stem := strings.TrimSuffix(file, filepath.Ext(file))
		fmt.Fprintln(w, "LOCAL_MODULE_PATH := $(OUT_DIR)/"+dir)
		fmt.Fprintln(w, "LOCAL_MODULE_STEM := "+stem)
		return nil
	})
}

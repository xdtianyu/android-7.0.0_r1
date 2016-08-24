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
	"bytes"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"sort"

	"android/soong"

	"github.com/google/blueprint"
)

func init() {
	soong.RegisterSingletonType("androidmk", AndroidMkSingleton)
}

type AndroidMkDataProvider interface {
	AndroidMk() (AndroidMkData, error)
}

type AndroidMkData struct {
	Class      string
	SubName    string
	OutputFile OptionalPath
	Disabled   bool

	Custom func(w io.Writer, name, prefix string) error

	Extra []func(w io.Writer, outputFile Path) error
}

func AndroidMkSingleton() blueprint.Singleton {
	return &androidMkSingleton{}
}

type androidMkSingleton struct{}

func (c *androidMkSingleton) GenerateBuildActions(ctx blueprint.SingletonContext) {
	if !ctx.Config().(Config).EmbeddedInMake() {
		return
	}

	ctx.SetNinjaBuildDir(pctx, filepath.Join(ctx.Config().(Config).buildDir, ".."))

	var androidMkModulesList []AndroidModule

	ctx.VisitAllModules(func(module blueprint.Module) {
		if amod, ok := module.(AndroidModule); ok {
			androidMkModulesList = append(androidMkModulesList, amod)
		}
	})

	sort.Sort(AndroidModulesByName{androidMkModulesList, ctx})

	transMk := PathForOutput(ctx, "Android.mk")
	if ctx.Failed() {
		return
	}

	err := translateAndroidMk(ctx, transMk.String(), androidMkModulesList)
	if err != nil {
		ctx.Errorf(err.Error())
	}

	ctx.Build(pctx, blueprint.BuildParams{
		Rule:     blueprint.Phony,
		Outputs:  []string{transMk.String()},
		Optional: true,
	})
}

func translateAndroidMk(ctx blueprint.SingletonContext, mkFile string, mods []AndroidModule) error {
	buf := &bytes.Buffer{}

	fmt.Fprintln(buf, "LOCAL_PATH := $(TOP)")
	fmt.Fprintln(buf, "LOCAL_MODULE_MAKEFILE := $(lastword $(MAKEFILE_LIST))")

	for _, mod := range mods {
		err := translateAndroidMkModule(ctx, buf, mod)
		if err != nil {
			os.Remove(mkFile)
			return err
		}
	}

	// Don't write to the file if it hasn't changed
	if _, err := os.Stat(mkFile); !os.IsNotExist(err) {
		if data, err := ioutil.ReadFile(mkFile); err == nil {
			matches := buf.Len() == len(data)

			if matches {
				for i, value := range buf.Bytes() {
					if value != data[i] {
						matches = false
						break
					}
				}
			}

			if matches {
				return nil
			}
		}
	}

	return ioutil.WriteFile(mkFile, buf.Bytes(), 0666)
}

func translateAndroidMkModule(ctx blueprint.SingletonContext, w io.Writer, mod blueprint.Module) error {
	name := ctx.ModuleName(mod)

	provider, ok := mod.(AndroidMkDataProvider)
	if !ok {
		return nil
	}

	amod := mod.(AndroidModule).base()
	data, err := provider.AndroidMk()
	if err != nil {
		return err
	}

	if !amod.Enabled() {
		return err
	}

	if data.SubName != "" {
		name += "_" + data.SubName
	}

	hostCross := false
	if amod.Host() && amod.HostType() != CurrentHostType() {
		hostCross = true
	}

	if data.Custom != nil {
		prefix := ""
		if amod.Host() {
			if hostCross {
				prefix = "HOST_CROSS_"
			} else {
				prefix = "HOST_"
			}
			if amod.Arch().ArchType != ctx.Config().(Config).HostArches[amod.HostType()][0].ArchType {
				prefix = "2ND_" + prefix
			}
		} else {
			prefix = "TARGET_"
			if amod.Arch().ArchType != ctx.Config().(Config).DeviceArches[0].ArchType {
				prefix = "2ND_" + prefix
			}
		}

		return data.Custom(w, name, prefix)
	}

	if data.Disabled {
		return nil
	}

	if !data.OutputFile.Valid() {
		return err
	}

	fmt.Fprintln(w, "\ninclude $(CLEAR_VARS)")
	fmt.Fprintln(w, "LOCAL_MODULE :=", name)
	fmt.Fprintln(w, "LOCAL_MODULE_CLASS :=", data.Class)
	fmt.Fprintln(w, "LOCAL_MULTILIB :=", amod.commonProperties.Compile_multilib)
	fmt.Fprintln(w, "LOCAL_SRC_FILES :=", data.OutputFile.String())

	archStr := amod.Arch().ArchType.String()
	if amod.Host() {
		if hostCross {
			fmt.Fprintln(w, "LOCAL_MODULE_HOST_CROSS_ARCH :=", archStr)
		} else {
			fmt.Fprintln(w, "LOCAL_MODULE_HOST_ARCH :=", archStr)

			// TODO: this isn't true for every module, only dependencies of ACP
			fmt.Fprintln(w, "LOCAL_ACP_UNAVAILABLE := true")
		}
		fmt.Fprintln(w, "LOCAL_MODULE_HOST_OS :=", amod.HostType().String())
		fmt.Fprintln(w, "LOCAL_IS_HOST_MODULE := true")
	} else {
		fmt.Fprintln(w, "LOCAL_MODULE_TARGET_ARCH :=", archStr)
	}

	for _, extra := range data.Extra {
		err = extra(w, data.OutputFile.Path())
		if err != nil {
			return err
		}
	}

	fmt.Fprintln(w, "include $(BUILD_PREBUILT)")

	return err
}

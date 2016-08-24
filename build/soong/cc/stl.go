// Copyright 2016 Google Inc. All rights reserved.
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
	"android/soong/common"
	"fmt"
)

type StlProperties struct {
	// select the STL library to use.  Possible values are "libc++", "libc++_static",
	// "stlport", "stlport_static", "ndk", "libstdc++", or "none".  Leave blank to select the
	// default
	Stl string

	SelectedStl string `blueprint:"mutated"`
}

type stlFeature struct {
	Properties StlProperties
}

var _ feature = (*stlFeature)(nil)

func (stl *stlFeature) props() []interface{} {
	return []interface{}{&stl.Properties}
}

func (stl *stlFeature) begin(ctx BaseModuleContext) {
	stl.Properties.SelectedStl = func() string {
		if ctx.sdk() && ctx.Device() {
			switch stl.Properties.Stl {
			case "":
				return "ndk_system"
			case "c++_shared", "c++_static",
				"stlport_shared", "stlport_static",
				"gnustl_static":
				return "ndk_lib" + stl.Properties.Stl
			default:
				ctx.ModuleErrorf("stl: %q is not a supported STL with sdk_version set", stl.Properties.Stl)
				return ""
			}
		} else if ctx.HostType() == common.Windows {
			switch stl.Properties.Stl {
			case "libc++", "libc++_static", "libstdc++", "":
				// libc++ is not supported on mingw
				return "libstdc++"
			case "none":
				return ""
			default:
				ctx.ModuleErrorf("stl: %q is not a supported STL", stl.Properties.Stl)
				return ""
			}
		} else {
			switch stl.Properties.Stl {
			case "libc++", "libc++_static",
				"libstdc++":
				return stl.Properties.Stl
			case "none":
				return ""
			case "":
				if ctx.static() {
					return "libc++_static"
				} else {
					return "libc++"
				}
			default:
				ctx.ModuleErrorf("stl: %q is not a supported STL", stl.Properties.Stl)
				return ""
			}
		}
	}()
}

func (stl *stlFeature) deps(ctx BaseModuleContext, deps Deps) Deps {
	switch stl.Properties.SelectedStl {
	case "libstdc++":
		if ctx.Device() {
			deps.SharedLibs = append(deps.SharedLibs, stl.Properties.SelectedStl)
		}
	case "libc++", "libc++_static":
		if stl.Properties.SelectedStl == "libc++" {
			deps.SharedLibs = append(deps.SharedLibs, stl.Properties.SelectedStl)
		} else {
			deps.StaticLibs = append(deps.StaticLibs, stl.Properties.SelectedStl)
		}
		if ctx.Device() {
			if ctx.Arch().ArchType == common.Arm {
				deps.StaticLibs = append(deps.StaticLibs, "libunwind_llvm")
			}
			if ctx.staticBinary() {
				deps.StaticLibs = append(deps.StaticLibs, "libdl")
			} else {
				deps.SharedLibs = append(deps.SharedLibs, "libdl")
			}
		}
	case "":
		// None or error.
	case "ndk_system":
		// TODO: Make a system STL prebuilt for the NDK.
		// The system STL doesn't have a prebuilt (it uses the system's libstdc++), but it does have
		// its own includes. The includes are handled in CCBase.Flags().
		deps.SharedLibs = append([]string{"libstdc++"}, deps.SharedLibs...)
	case "ndk_libc++_shared", "ndk_libstlport_shared":
		deps.SharedLibs = append(deps.SharedLibs, stl.Properties.SelectedStl)
	case "ndk_libc++_static", "ndk_libstlport_static", "ndk_libgnustl_static":
		deps.StaticLibs = append(deps.StaticLibs, stl.Properties.SelectedStl)
	default:
		panic(fmt.Errorf("Unknown stl: %q", stl.Properties.SelectedStl))
	}

	return deps
}

func (stl *stlFeature) flags(ctx ModuleContext, flags Flags) Flags {
	switch stl.Properties.SelectedStl {
	case "libc++", "libc++_static":
		flags.CFlags = append(flags.CFlags, "-D_USING_LIBCXX")
		if ctx.Host() {
			flags.CppFlags = append(flags.CppFlags, "-nostdinc++")
			flags.LdFlags = append(flags.LdFlags, "-nodefaultlibs")
			flags.LdFlags = append(flags.LdFlags, "-lpthread", "-lm")
			if ctx.staticBinary() {
				flags.LdFlags = append(flags.LdFlags, hostStaticGccLibs[ctx.HostType()]...)
			} else {
				flags.LdFlags = append(flags.LdFlags, hostDynamicGccLibs[ctx.HostType()]...)
			}
		} else {
			if ctx.Arch().ArchType == common.Arm {
				flags.LdFlags = append(flags.LdFlags, "-Wl,--exclude-libs,libunwind_llvm.a")
			}
		}
	case "libstdc++":
		// Using bionic's basic libstdc++. Not actually an STL. Only around until the
		// tree is in good enough shape to not need it.
		// Host builds will use GNU libstdc++.
		if ctx.Device() {
			flags.CFlags = append(flags.CFlags, "-I"+common.PathForSource(ctx, "bionic/libstdc++/include").String())
		} else {
			// Host builds will use the system C++. libc++ on Darwin, GNU libstdc++ everywhere else
			flags.CppFlags = append(flags.CppFlags, flags.Toolchain.SystemCppCppflags())
			flags.LdFlags = append(flags.LdFlags, flags.Toolchain.SystemCppLdflags())
		}
	case "ndk_system":
		ndkSrcRoot := common.PathForSource(ctx, "prebuilts/ndk/current/sources/cxx-stl/system/include")
		flags.CFlags = append(flags.CFlags, "-isystem "+ndkSrcRoot.String())
	case "ndk_libc++_shared", "ndk_libc++_static":
		// TODO(danalbert): This really shouldn't be here...
		flags.CppFlags = append(flags.CppFlags, "-std=c++11")
	case "ndk_libstlport_shared", "ndk_libstlport_static", "ndk_libgnustl_static":
		// Nothing
	case "":
		// None or error.
		if ctx.Host() {
			flags.CppFlags = append(flags.CppFlags, "-nostdinc++")
			flags.LdFlags = append(flags.LdFlags, "-nodefaultlibs")
			if ctx.staticBinary() {
				flags.LdFlags = append(flags.LdFlags, hostStaticGccLibs[ctx.HostType()]...)
			} else {
				flags.LdFlags = append(flags.LdFlags, hostDynamicGccLibs[ctx.HostType()]...)
			}
		}
	default:
		panic(fmt.Errorf("Unknown stl: %q", stl.Properties.SelectedStl))
	}

	return flags
}

var hostDynamicGccLibs, hostStaticGccLibs map[common.HostType][]string

func init() {
	hostDynamicGccLibs = map[common.HostType][]string{
		common.Linux:  []string{"-lgcc_s", "-lgcc", "-lc", "-lgcc_s", "-lgcc"},
		common.Darwin: []string{"-lc", "-lSystem"},
		common.Windows: []string{"-lmsvcr110", "-lmingw32", "-lgcc", "-lmoldname",
			"-lmingwex", "-lmsvcrt", "-ladvapi32", "-lshell32", "-luser32",
			"-lkernel32", "-lmingw32", "-lgcc", "-lmoldname", "-lmingwex",
			"-lmsvcrt"},
	}
	hostStaticGccLibs = map[common.HostType][]string{
		common.Linux:   []string{"-Wl,--start-group", "-lgcc", "-lgcc_eh", "-lc", "-Wl,--end-group"},
		common.Darwin:  []string{"NO_STATIC_HOST_BINARIES_ON_DARWIN"},
		common.Windows: []string{"NO_STATIC_HOST_BINARIES_ON_WINDOWS"},
	}
}

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
	"strings"

	"android/soong/common"
)

var (
	windowsCflags = []string{
		"-fno-exceptions", // from build/core/combo/select.mk
		"-Wno-multichar",  // from build/core/combo/select.mk

		"-DUSE_MINGW",
		"-DWIN32_LEAN_AND_MEAN",
		"-Wno-unused-parameter",
		"-m32",

		// Workaround differences in inttypes.h between host and target.
		//See bug 12708004.
		"-D__STDC_FORMAT_MACROS",
		"-D__STDC_CONSTANT_MACROS",

		// Use C99-compliant printf functions (%zd).
		"-D__USE_MINGW_ANSI_STDIO=1",
		// Admit to using >= Win2K. Both are needed because of <_mingw.h>.
		"-D_WIN32_WINNT=0x0500",
		"-DWINVER=0x0500",
		// Get 64-bit off_t and related functions.
		"-D_FILE_OFFSET_BITS=64",

		// HOST_RELEASE_CFLAGS
		"-O2", // from build/core/combo/select.mk
		"-g",  // from build/core/combo/select.mk
		"-fno-strict-aliasing", // from build/core/combo/select.mk
	}

	windowsIncludeFlags = []string{
		"-I${windowsGccRoot}/${windowsGccTriple}/include",
		"-I${windowsGccRoot}/lib/gcc/${windowsGccTriple}/4.8.3/include",
	}

	windowsLdflags = []string{
		"-L${windowsGccRoot}/${windowsGccTriple}",
		"--enable-stdcall-fixup",
	}

	windowsX86Cflags = []string{
		"-m32",
	}

	windowsX8664Cflags = []string{
		"-m64",
	}

	windowsX86Ldflags = []string{
		"-m32",
	}

	windowsX8664Ldflags = []string{
		"-m64",
	}
)

const (
	windowsGccVersion = "4.8"
)

func init() {
	pctx.StaticVariable("windowsGccVersion", windowsGccVersion)

	pctx.SourcePathVariable("windowsGccRoot",
		"prebuilts/gcc/${HostPrebuiltTag}/host/x86_64-w64-mingw32-${windowsGccVersion}")

	pctx.StaticVariable("windowsGccTriple", "x86_64-w64-mingw32")

	pctx.StaticVariable("windowsCflags", strings.Join(windowsCflags, " "))
	pctx.StaticVariable("windowsLdflags", strings.Join(windowsLdflags, " "))

	pctx.StaticVariable("windowsX86Cflags", strings.Join(windowsX86Cflags, " "))
	pctx.StaticVariable("windowsX8664Cflags", strings.Join(windowsX8664Cflags, " "))
	pctx.StaticVariable("windowsX86Ldflags", strings.Join(windowsX86Ldflags, " "))
	pctx.StaticVariable("windowsX8664Ldflags", strings.Join(windowsX8664Ldflags, " "))
}

type toolchainWindows struct {
	cFlags, ldFlags string
}

type toolchainWindowsX86 struct {
	toolchain32Bit
	toolchainWindows
}

type toolchainWindowsX8664 struct {
	toolchain64Bit
	toolchainWindows
}

func (t *toolchainWindowsX86) Name() string {
	return "x86"
}

func (t *toolchainWindowsX8664) Name() string {
	return "x86_64"
}

func (t *toolchainWindows) GccRoot() string {
	return "${windowsGccRoot}"
}

func (t *toolchainWindows) GccTriple() string {
	return "${windowsGccTriple}"
}

func (t *toolchainWindows) GccVersion() string {
	return windowsGccVersion
}

func (t *toolchainWindowsX86) Cflags() string {
	return "${windowsCflags} ${windowsX86Cflags}"
}

func (t *toolchainWindowsX8664) Cflags() string {
	return "${windowsCflags} ${windowsX8664Cflags}"
}

func (t *toolchainWindows) Cppflags() string {
	return ""
}

func (t *toolchainWindowsX86) Ldflags() string {
	return "${windowsLdflags} ${windowsX86Ldflags}"
}

func (t *toolchainWindowsX8664) Ldflags() string {
	return "${windowsLdflags} ${windowsX8664Ldflags}"
}

func (t *toolchainWindows) IncludeFlags() string {
	return ""
}

func (t *toolchainWindows) ClangSupported() bool {
	return false
}

func (t *toolchainWindows) ClangTriple() string {
	panic("Clang is not supported under mingw")
}

func (t *toolchainWindows) ClangCflags() string {
	panic("Clang is not supported under mingw")
}

func (t *toolchainWindows) ClangCppflags() string {
	panic("Clang is not supported under mingw")
}

func (t *toolchainWindows) ClangLdflags() string {
	panic("Clang is not supported under mingw")
}

func (t *toolchainWindows) ShlibSuffix() string {
	return ".dll"
}

func (t *toolchainWindows) ExecutableSuffix() string {
	return ".exe"
}

var toolchainWindowsX86Singleton Toolchain = &toolchainWindowsX86{}
var toolchainWindowsX8664Singleton Toolchain = &toolchainWindowsX8664{}

func windowsX86ToolchainFactory(arch common.Arch) Toolchain {
	return toolchainWindowsX86Singleton
}

func windowsX8664ToolchainFactory(arch common.Arch) Toolchain {
	return toolchainWindowsX8664Singleton
}

func init() {
	registerHostToolchainFactory(common.Windows, common.X86, windowsX86ToolchainFactory)
	registerHostToolchainFactory(common.Windows, common.X86_64, windowsX8664ToolchainFactory)
}

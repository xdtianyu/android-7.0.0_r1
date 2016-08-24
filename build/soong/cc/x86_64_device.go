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
	x86_64Cflags = []string{
		"-fno-exceptions", // from build/core/combo/select.mk
		"-Wno-multichar",  // from build/core/combo/select.mk
		"-O2",
		"-Wa,--noexecstack",
		"-Werror=format-security",
		"-D_FORTIFY_SOURCE=2",
		"-Wstrict-aliasing=2",
		"-ffunction-sections",
		"-finline-functions",
		"-finline-limit=300",
		"-fno-short-enums",
		"-fstrict-aliasing",
		"-funswitch-loops",
		"-funwind-tables",
		"-fstack-protector-strong",
		"-no-canonical-prefixes",
		"-fno-canonical-system-headers",

		// Help catch common 32/64-bit errors.
		"-Werror=pointer-to-int-cast",
		"-Werror=int-to-pointer-cast",
		"-Werror=implicit-function-declaration",

		// TARGET_RELEASE_CFLAGS from build/core/combo/select.mk
		"-O2",
		"-g",
		"-fno-strict-aliasing",
	}

	x86_64Cppflags = []string{}

	x86_64Ldflags = []string{
		"-Wl,-z,noexecstack",
		"-Wl,-z,relro",
		"-Wl,-z,now",
		"-Wl,--build-id=md5",
		"-Wl,--warn-shared-textrel",
		"-Wl,--fatal-warnings",
		"-Wl,--gc-sections",
		"-Wl,--hash-style=gnu",
		"-Wl,--no-undefined-version",
	}

	x86_64ArchVariantCflags = map[string][]string{
		"": []string{
			"-march=x86-64",
		},
		"haswell": []string{
			"-march=core-avx2",
		},
		"ivybridge": []string{
			"-march=core-avx-i",
		},
		"sandybridge": []string{
			"-march=corei7",
		},
		"silvermont": []string{
			"-march=slm",
		},
	}

	x86_64ArchFeatureCflags = map[string][]string{
		"ssse3":  []string{"-DUSE_SSSE3", "-mssse3"},
		"sse4":   []string{"-msse4"},
		"sse4_1": []string{"-msse4.1"},
		"sse4_2": []string{"-msse4.2"},
		"avx":    []string{"-mavx"},
		"aes_ni": []string{"-maes"},
	}
)

const (
	x86_64GccVersion = "4.9"
)

func init() {
	common.RegisterArchFeatures(common.X86_64, "",
		"ssse3",
		"sse4",
		"sse4_1",
		"sse4_2",
		"popcnt")
	common.RegisterArchFeatures(common.X86_64, "haswell",
		"ssse3",
		"sse4",
		"sse4_1",
		"sse4_2",
		"aes_ni",
		"avx",
		"popcnt")
	common.RegisterArchFeatures(common.X86_64, "ivybridge",
		"ssse3",
		"sse4",
		"sse4_1",
		"sse4_2",
		"aes_ni",
		"avx",
		"popcnt")
	common.RegisterArchFeatures(common.X86_64, "sandybridge",
		"ssse3",
		"sse4",
		"sse4_1",
		"sse4_2",
		"popcnt")
	common.RegisterArchFeatures(common.X86_64, "silvermont",
		"ssse3",
		"sse4",
		"sse4_1",
		"sse4_2",
		"aes_ni",
		"popcnt")

	pctx.StaticVariable("x86_64GccVersion", x86_64GccVersion)

	pctx.SourcePathVariable("x86_64GccRoot",
		"prebuilts/gcc/${HostPrebuiltTag}/x86/x86_64-linux-android-${x86_64GccVersion}")

	pctx.StaticVariable("x86_64GccTriple", "x86_64-linux-android")

	pctx.StaticVariable("x86_64ToolchainCflags", "-m64")
	pctx.StaticVariable("x86_64ToolchainLdflags", "-m64")

	pctx.StaticVariable("x86_64Cflags", strings.Join(x86_64Cflags, " "))
	pctx.StaticVariable("x86_64Ldflags", strings.Join(x86_64Ldflags, " "))
	pctx.StaticVariable("x86_64Cppflags", strings.Join(x86_64Cppflags, " "))
	pctx.StaticVariable("x86_64IncludeFlags", strings.Join([]string{
		"-isystem ${LibcRoot}/arch-x86_64/include",
		"-isystem ${LibcRoot}/include",
		"-isystem ${LibcRoot}/kernel/uapi",
		"-isystem ${LibcRoot}/kernel/uapi/asm-x86",
		"-isystem ${LibmRoot}/include",
		"-isystem ${LibmRoot}/include/amd64",
	}, " "))

	// Clang cflags
	pctx.StaticVariable("x86_64ClangCflags", strings.Join(clangFilterUnknownCflags(x86_64Cflags), " "))
	pctx.StaticVariable("x86_64ClangLdflags", strings.Join(clangFilterUnknownCflags(x86_64Ldflags), " "))
	pctx.StaticVariable("x86_64ClangCppflags", strings.Join(clangFilterUnknownCflags(x86_64Cppflags), " "))

	// Extended cflags

	// Architecture variant cflags
	for variant, cflags := range x86_64ArchVariantCflags {
		pctx.StaticVariable("x86_64"+variant+"VariantCflags", strings.Join(cflags, " "))
		pctx.StaticVariable("x86_64"+variant+"VariantClangCflags",
			strings.Join(clangFilterUnknownCflags(cflags), " "))
	}
}

type toolchainX86_64 struct {
	toolchain64Bit
	toolchainCflags, toolchainClangCflags string
}

func (t *toolchainX86_64) Name() string {
	return "x86_64"
}

func (t *toolchainX86_64) GccRoot() string {
	return "${x86_64GccRoot}"
}

func (t *toolchainX86_64) GccTriple() string {
	return "${x86_64GccTriple}"
}

func (t *toolchainX86_64) GccVersion() string {
	return x86_64GccVersion
}

func (t *toolchainX86_64) ToolchainLdflags() string {
	return "${x86_64ToolchainLdflags}"
}

func (t *toolchainX86_64) ToolchainCflags() string {
	return t.toolchainCflags
}

func (t *toolchainX86_64) Cflags() string {
	return "${x86_64Cflags}"
}

func (t *toolchainX86_64) Cppflags() string {
	return "${x86_64Cppflags}"
}

func (t *toolchainX86_64) Ldflags() string {
	return "${x86_64Ldflags}"
}

func (t *toolchainX86_64) IncludeFlags() string {
	return "${x86_64IncludeFlags}"
}

func (t *toolchainX86_64) ClangTriple() string {
	return "${x86_64GccTriple}"
}

func (t *toolchainX86_64) ToolchainClangCflags() string {
	return t.toolchainClangCflags
}

func (t *toolchainX86_64) ClangCflags() string {
	return "${x86_64ClangCflags}"
}

func (t *toolchainX86_64) ClangCppflags() string {
	return "${x86_64ClangCppflags}"
}

func (t *toolchainX86_64) ClangLdflags() string {
	return "${x86_64Ldflags}"
}

func x86_64ToolchainFactory(arch common.Arch) Toolchain {
	toolchainCflags := []string{
		"${x86_64ToolchainCflags}",
		"${x86_64" + arch.ArchVariant + "VariantCflags}",
	}

	toolchainClangCflags := []string{
		"${x86_64ToolchainCflags}",
		"${x86_64" + arch.ArchVariant + "VariantClangCflags}",
	}

	for _, feature := range arch.ArchFeatures {
		toolchainCflags = append(toolchainCflags, x86_64ArchFeatureCflags[feature]...)
		toolchainClangCflags = append(toolchainClangCflags, x86_64ArchFeatureCflags[feature]...)
	}

	return &toolchainX86_64{
		toolchainCflags:      strings.Join(toolchainCflags, " "),
		toolchainClangCflags: strings.Join(toolchainClangCflags, " "),
	}
}

func init() {
	registerDeviceToolchainFactory(common.X86_64, x86_64ToolchainFactory)
}

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
	x86Cflags = []string{
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

		// TARGET_RELEASE_CFLAGS from build/core/combo/select.mk
		"-O2",
		"-g",
		"-fno-strict-aliasing",
	}

	x86Cppflags = []string{}

	x86Ldflags = []string{
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

	x86ArchVariantCflags = map[string][]string{
		"": []string{
			"-march=prescott",
		},
		"atom": []string{
			"-march=atom",
			"-mfpmath=sse",
		},
		"haswell": []string{
			"-march=core-avx2",
			"-mfpmath=sse",
		},
		"ivybridge": []string{
			"-march=core-avx-i",
			"-mfpmath=sse",
		},
		"sandybridge": []string{
			"-march=corei7",
			"-mfpmath=sse",
		},
		"silvermont": []string{
			"-march=slm",
			"-mfpmath=sse",
		},
	}

	x86ArchFeatureCflags = map[string][]string{
		"ssse3":  []string{"-DUSE_SSSE3", "-mssse3"},
		"sse4":   []string{"-msse4"},
		"sse4_1": []string{"-msse4.1"},
		"sse4_2": []string{"-msse4.2"},
		"avx":    []string{"-mavx"},
		"aes_ni": []string{"-maes"},
	}
)

const (
	x86GccVersion = "4.9"
)

func init() {
	common.RegisterArchFeatures(common.X86, "atom",
		"ssse3",
		"movbe")
	common.RegisterArchFeatures(common.X86, "haswell",
		"ssse3",
		"sse4",
		"sse4_1",
		"sse4_2",
		"aes_ni",
		"avx",
		"popcnt",
		"movbe")
	common.RegisterArchFeatures(common.X86, "ivybridge",
		"ssse3",
		"sse4",
		"sse4_1",
		"sse4_2",
		"aes_ni",
		"avx",
		"popcnt")
	common.RegisterArchFeatures(common.X86, "sandybridge",
		"ssse3",
		"sse4",
		"sse4_1",
		"sse4_2",
		"popcnt")
	common.RegisterArchFeatures(common.X86, "silvermont",
		"ssse3",
		"sse4",
		"sse4_1",
		"sse4_2",
		"aes_ni",
		"popcnt",
		"movbe")

	pctx.StaticVariable("x86GccVersion", x86GccVersion)

	pctx.SourcePathVariable("x86GccRoot",
		"prebuilts/gcc/${HostPrebuiltTag}/x86/x86_64-linux-android-${x86GccVersion}")

	pctx.StaticVariable("x86GccTriple", "x86_64-linux-android")

	pctx.StaticVariable("x86ToolchainCflags", "-m32")
	pctx.StaticVariable("x86ToolchainLdflags", "-m32")

	pctx.StaticVariable("x86Cflags", strings.Join(x86Cflags, " "))
	pctx.StaticVariable("x86Ldflags", strings.Join(x86Ldflags, " "))
	pctx.StaticVariable("x86Cppflags", strings.Join(x86Cppflags, " "))
	pctx.StaticVariable("x86IncludeFlags", strings.Join([]string{
		"-isystem ${LibcRoot}/arch-x86/include",
		"-isystem ${LibcRoot}/include",
		"-isystem ${LibcRoot}/kernel/uapi",
		"-isystem ${LibcRoot}/kernel/uapi/asm-x86",
		"-isystem ${LibmRoot}/include",
		"-isystem ${LibmRoot}/include/i387",
	}, " "))

	// Clang cflags
	pctx.StaticVariable("x86ClangCflags", strings.Join(clangFilterUnknownCflags(x86Cflags), " "))
	pctx.StaticVariable("x86ClangLdflags", strings.Join(clangFilterUnknownCflags(x86Ldflags), " "))
	pctx.StaticVariable("x86ClangCppflags", strings.Join(clangFilterUnknownCflags(x86Cppflags), " "))

	// Extended cflags

	// Architecture variant cflags
	for variant, cflags := range x86ArchVariantCflags {
		pctx.StaticVariable("x86"+variant+"VariantCflags", strings.Join(cflags, " "))
		pctx.StaticVariable("x86"+variant+"VariantClangCflags",
			strings.Join(clangFilterUnknownCflags(cflags), " "))
	}
}

type toolchainX86 struct {
	toolchain32Bit
	toolchainCflags, toolchainClangCflags string
}

func (t *toolchainX86) Name() string {
	return "x86"
}

func (t *toolchainX86) GccRoot() string {
	return "${x86GccRoot}"
}

func (t *toolchainX86) GccTriple() string {
	return "${x86GccTriple}"
}

func (t *toolchainX86) GccVersion() string {
	return x86GccVersion
}

func (t *toolchainX86) ToolchainLdflags() string {
	return "${x86ToolchainLdflags}"
}

func (t *toolchainX86) ToolchainCflags() string {
	return t.toolchainCflags
}

func (t *toolchainX86) Cflags() string {
	return "${x86Cflags}"
}

func (t *toolchainX86) Cppflags() string {
	return "${x86Cppflags}"
}

func (t *toolchainX86) Ldflags() string {
	return "${x86Ldflags}"
}

func (t *toolchainX86) IncludeFlags() string {
	return "${x86IncludeFlags}"
}

func (t *toolchainX86) ClangTriple() string {
	return "${x86GccTriple}"
}

func (t *toolchainX86) ToolchainClangCflags() string {
	return t.toolchainClangCflags
}

func (t *toolchainX86) ClangCflags() string {
	return "${x86ClangCflags}"
}

func (t *toolchainX86) ClangCppflags() string {
	return "${x86ClangCppflags}"
}

func (t *toolchainX86) ClangLdflags() string {
	return "${x86Ldflags}"
}

func x86ToolchainFactory(arch common.Arch) Toolchain {
	toolchainCflags := []string{
		"${x86ToolchainCflags}",
		"${x86" + arch.ArchVariant + "VariantCflags}",
	}

	toolchainClangCflags := []string{
		"${x86ToolchainCflags}",
		"${x86" + arch.ArchVariant + "VariantClangCflags}",
	}

	for _, feature := range arch.ArchFeatures {
		toolchainCflags = append(toolchainCflags, x86ArchFeatureCflags[feature]...)
		toolchainClangCflags = append(toolchainClangCflags, x86ArchFeatureCflags[feature]...)
	}

	return &toolchainX86{
		toolchainCflags:      strings.Join(toolchainCflags, " "),
		toolchainClangCflags: strings.Join(toolchainClangCflags, " "),
	}
}

func init() {
	registerDeviceToolchainFactory(common.X86, x86ToolchainFactory)
}

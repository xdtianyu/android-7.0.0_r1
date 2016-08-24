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
	mips64Cflags = []string{
		"-fno-exceptions", // from build/core/combo/select.mk
		"-Wno-multichar",  // from build/core/combo/select.mk
		"-O2",
		"-fomit-frame-pointer",
		"-fno-strict-aliasing",
		"-funswitch-loops",
		"-U__unix",
		"-U__unix__",
		"-Umips",
		"-ffunction-sections",
		"-fdata-sections",
		"-funwind-tables",
		"-fstack-protector-strong",
		"-Wa,--noexecstack",
		"-Werror=format-security",
		"-D_FORTIFY_SOURCE=2",
		"-no-canonical-prefixes",
		"-fno-canonical-system-headers",

		// Help catch common 32/64-bit errors.
		"-Werror=pointer-to-int-cast",
		"-Werror=int-to-pointer-cast",
		"-Werror=implicit-function-declaration",

		// TARGET_RELEASE_CFLAGS
		"-DNDEBUG",
		"-g",
		"-Wstrict-aliasing=2",
		"-fgcse-after-reload",
		"-frerun-cse-after-loop",
		"-frename-registers",
	}

	mips64Cppflags = []string{
		"-fvisibility-inlines-hidden",
	}

	mips64Ldflags = []string{
		"-Wl,-z,noexecstack",
		"-Wl,-z,relro",
		"-Wl,-z,now",
		"-Wl,--build-id=md5",
		"-Wl,--warn-shared-textrel",
		"-Wl,--fatal-warnings",
		"-Wl,--allow-shlib-undefined",
		"-Wl,--no-undefined-version",
	}

	mips64ArchVariantCflags = map[string][]string{
		"mips64r2": []string{
			"-mips64r2",
			"-msynci",
		},
		"mips64r6": []string{
			"-mips64r6",
			"-msynci",
		},
	}
)

const (
	mips64GccVersion = "4.9"
)

func init() {
	common.RegisterArchFeatures(common.Mips64, "mips64r6",
		"rev6")

	pctx.StaticVariable("mips64GccVersion", mips64GccVersion)

	pctx.SourcePathVariable("mips64GccRoot",
		"prebuilts/gcc/${HostPrebuiltTag}/mips/mips64el-linux-android-${mips64GccVersion}")

	pctx.StaticVariable("mips64GccTriple", "mips64el-linux-android")

	pctx.StaticVariable("mips64Cflags", strings.Join(mips64Cflags, " "))
	pctx.StaticVariable("mips64Ldflags", strings.Join(mips64Ldflags, " "))
	pctx.StaticVariable("mips64Cppflags", strings.Join(mips64Cppflags, " "))
	pctx.StaticVariable("mips64IncludeFlags", strings.Join([]string{
		"-isystem ${LibcRoot}/arch-mips64/include",
		"-isystem ${LibcRoot}/include",
		"-isystem ${LibcRoot}/kernel/uapi",
		"-isystem ${LibcRoot}/kernel/uapi/asm-mips",
		"-isystem ${LibmRoot}/include",
		"-isystem ${LibmRoot}/include/mips",
	}, " "))

	// Clang cflags
	pctx.StaticVariable("mips64ClangTriple", "mips64el-linux-android")
	pctx.StaticVariable("mips64ClangCflags", strings.Join(clangFilterUnknownCflags(mips64Cflags), " "))
	pctx.StaticVariable("mips64ClangLdflags", strings.Join(clangFilterUnknownCflags(mips64Ldflags), " "))
	pctx.StaticVariable("mips64ClangCppflags", strings.Join(clangFilterUnknownCflags(mips64Cppflags), " "))

	// Extended cflags

	// Architecture variant cflags
	for variant, cflags := range mips64ArchVariantCflags {
		pctx.StaticVariable("mips64"+variant+"VariantCflags", strings.Join(cflags, " "))
		pctx.StaticVariable("mips64"+variant+"VariantClangCflags",
			strings.Join(clangFilterUnknownCflags(cflags), " "))
	}
}

type toolchainMips64 struct {
	toolchain64Bit
	cflags, clangCflags                   string
	toolchainCflags, toolchainClangCflags string
}

func (t *toolchainMips64) Name() string {
	return "mips64"
}

func (t *toolchainMips64) GccRoot() string {
	return "${mips64GccRoot}"
}

func (t *toolchainMips64) GccTriple() string {
	return "${mips64GccTriple}"
}

func (t *toolchainMips64) GccVersion() string {
	return mips64GccVersion
}

func (t *toolchainMips64) ToolchainLdflags() string {
	return ""
}

func (t *toolchainMips64) ToolchainCflags() string {
	return t.toolchainCflags
}

func (t *toolchainMips64) Cflags() string {
	return t.cflags
}

func (t *toolchainMips64) Cppflags() string {
	return "${mips64Cppflags}"
}

func (t *toolchainMips64) Ldflags() string {
	return "${mips64Ldflags}"
}

func (t *toolchainMips64) IncludeFlags() string {
	return "${mips64IncludeFlags}"
}

func (t *toolchainMips64) ClangTriple() string {
	return "${mips64ClangTriple}"
}

func (t *toolchainMips64) ToolchainClangCflags() string {
	return t.toolchainClangCflags
}

func (t *toolchainMips64) ClangCflags() string {
	return t.clangCflags
}

func (t *toolchainMips64) ClangCppflags() string {
	return "${mips64ClangCppflags}"
}

func (t *toolchainMips64) ClangLdflags() string {
	return "${mips64ClangLdflags}"
}

func mips64ToolchainFactory(arch common.Arch) Toolchain {
	return &toolchainMips64{
		cflags:               "${mips64Cflags}",
		clangCflags:          "${mips64ClangCflags}",
		toolchainCflags:      "${mips64" + arch.ArchVariant + "VariantCflags}",
		toolchainClangCflags: "${mips64" + arch.ArchVariant + "VariantClangCflags}",
	}
}

func init() {
	registerDeviceToolchainFactory(common.Mips64, mips64ToolchainFactory)
}

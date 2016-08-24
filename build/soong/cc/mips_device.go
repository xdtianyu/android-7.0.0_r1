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
	mipsCflags = []string{
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

		// TARGET_RELEASE_CFLAGS
		"-DNDEBUG",
		"-g",
		"-Wstrict-aliasing=2",
		"-fgcse-after-reload",
		"-frerun-cse-after-loop",
		"-frename-registers",
	}

	mipsCppflags = []string{
		"-fvisibility-inlines-hidden",
	}

	mipsLdflags = []string{
		"-Wl,-z,noexecstack",
		"-Wl,-z,relro",
		"-Wl,-z,now",
		"-Wl,--build-id=md5",
		"-Wl,--warn-shared-textrel",
		"-Wl,--fatal-warnings",
		"-Wl,--allow-shlib-undefined",
		"-Wl,--no-undefined-version",
	}

	mipsToolchainLdflags = []string{
		"-Wl,-melf32ltsmip",
	}

	mipsArchVariantCflags = map[string][]string{
		"mips32-fp": []string{
			"-mips32",
			"-mfp32",
			"-modd-spreg",
			"-mno-synci",
		},
		"mips32r2-fp": []string{
			"-mips32r2",
			"-mfp32",
			"-modd-spreg",
			"-mno-synci",
		},
		"mips32r2-fp-xburst": []string{
			"-mips32r2",
			"-mfp32",
			"-modd-spreg",
			"-mno-fused-madd",
			"-Wa,-mmxu",
			"-mno-synci",
		},
		"mips32r2dsp-fp": []string{
			"-mips32r2",
			"-mfp32",
			"-modd-spreg",
			"-mdsp",
			"-msynci",
		},
		"mips32r2dspr2-fp": []string{
			"-mips32r2",
			"-mfp32",
			"-modd-spreg",
			"-mdspr2",
			"-msynci",
		},
		"mips32r6": []string{
			"-mips32r6",
			"-mfp64",
			"-mno-odd-spreg",
			"-msynci",
		},
	}
)

const (
	mipsGccVersion = "4.9"
)

func init() {
	common.RegisterArchFeatures(common.Mips, "mips32r6",
		"rev6")

	pctx.StaticVariable("mipsGccVersion", mipsGccVersion)

	pctx.SourcePathVariable("mipsGccRoot",
		"prebuilts/gcc/${HostPrebuiltTag}/mips/mips64el-linux-android-${mipsGccVersion}")

	pctx.StaticVariable("mipsGccTriple", "mips64el-linux-android")

	pctx.StaticVariable("mipsToolchainLdflags", strings.Join(mipsToolchainLdflags, " "))
	pctx.StaticVariable("mipsCflags", strings.Join(mipsCflags, " "))
	pctx.StaticVariable("mipsLdflags", strings.Join(mipsLdflags, " "))
	pctx.StaticVariable("mipsCppflags", strings.Join(mipsCppflags, " "))
	pctx.StaticVariable("mipsIncludeFlags", strings.Join([]string{
		"-isystem ${LibcRoot}/arch-mips/include",
		"-isystem ${LibcRoot}/include",
		"-isystem ${LibcRoot}/kernel/uapi",
		"-isystem ${LibcRoot}/kernel/uapi/asm-mips",
		"-isystem ${LibmRoot}/include",
		"-isystem ${LibmRoot}/include/mips",
	}, " "))

	// Clang cflags
	pctx.StaticVariable("mipsClangTriple", "mipsel-linux-android")
	pctx.StaticVariable("mipsClangCflags", strings.Join(clangFilterUnknownCflags(mipsCflags), " "))
	pctx.StaticVariable("mipsClangLdflags", strings.Join(clangFilterUnknownCflags(mipsLdflags), " "))
	pctx.StaticVariable("mipsClangCppflags", strings.Join(clangFilterUnknownCflags(mipsCppflags), " "))

	// Extended cflags

	// Architecture variant cflags
	for variant, cflags := range mipsArchVariantCflags {
		pctx.StaticVariable("mips"+variant+"VariantCflags", strings.Join(cflags, " "))
		pctx.StaticVariable("mips"+variant+"VariantClangCflags",
			strings.Join(clangFilterUnknownCflags(cflags), " "))
	}
}

type toolchainMips struct {
	toolchain32Bit
	cflags, clangCflags                   string
	toolchainCflags, toolchainClangCflags string
}

func (t *toolchainMips) Name() string {
	return "mips"
}

func (t *toolchainMips) GccRoot() string {
	return "${mipsGccRoot}"
}

func (t *toolchainMips) GccTriple() string {
	return "${mipsGccTriple}"
}

func (t *toolchainMips) GccVersion() string {
	return mipsGccVersion
}

func (t *toolchainMips) ToolchainLdflags() string {
	return "${mipsToolchainLdflags}"
}

func (t *toolchainMips) ToolchainCflags() string {
	return t.toolchainCflags
}

func (t *toolchainMips) Cflags() string {
	return t.cflags
}

func (t *toolchainMips) Cppflags() string {
	return "${mipsCppflags}"
}

func (t *toolchainMips) Ldflags() string {
	return "${mipsLdflags}"
}

func (t *toolchainMips) IncludeFlags() string {
	return "${mipsIncludeFlags}"
}

func (t *toolchainMips) ClangTriple() string {
	return "${mipsClangTriple}"
}

func (t *toolchainMips) ToolchainClangCflags() string {
	return t.toolchainClangCflags
}

func (t *toolchainMips) ClangAsflags() string {
	return "-fPIC"
}

func (t *toolchainMips) ClangCflags() string {
	return t.clangCflags
}

func (t *toolchainMips) ClangCppflags() string {
	return "${mipsClangCppflags}"
}

func (t *toolchainMips) ClangLdflags() string {
	return "${mipsClangLdflags}"
}

func mipsToolchainFactory(arch common.Arch) Toolchain {
	return &toolchainMips{
		cflags:               "${mipsCflags}",
		clangCflags:          "${mipsClangCflags}",
		toolchainCflags:      "${mips" + arch.ArchVariant + "VariantCflags}",
		toolchainClangCflags: "${mips" + arch.ArchVariant + "VariantClangCflags}",
	}
}

func init() {
	registerDeviceToolchainFactory(common.Mips, mipsToolchainFactory)
}

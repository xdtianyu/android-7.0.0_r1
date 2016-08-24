package cc

import (
	"sort"
	"strings"
)

// Cflags that should be filtered out when compiling with clang
var clangUnknownCflags = sorted([]string{
	"-finline-functions",
	"-finline-limit=64",
	"-fno-canonical-system-headers",
	"-Wno-clobbered",
	"-fno-devirtualize",
	"-fno-tree-sra",
	"-fprefetch-loop-arrays",
	"-funswitch-loops",
	"-Wmaybe-uninitialized",
	"-Wno-error=clobbered",
	"-Wno-error=maybe-uninitialized",
	"-Wno-error=unused-but-set-parameter",
	"-Wno-error=unused-but-set-variable",
	"-Wno-free-nonheap-object",
	"-Wno-literal-suffix",
	"-Wno-maybe-uninitialized",
	"-Wno-old-style-declaration",
	"-Wno-psabi",
	"-Wno-unused-but-set-parameter",
	"-Wno-unused-but-set-variable",
	"-Wno-unused-local-typedefs",
	"-Wunused-but-set-parameter",
	"-Wunused-but-set-variable",
	"-fdiagnostics-color",

	// arm + arm64 + mips + mips64
	"-fgcse-after-reload",
	"-frerun-cse-after-loop",
	"-frename-registers",
	"-fno-strict-volatile-bitfields",

	// arm + arm64
	"-fno-align-jumps",

	// arm
	"-mthumb-interwork",
	"-fno-builtin-sin",
	"-fno-caller-saves",
	"-fno-early-inlining",
	"-fno-move-loop-invariants",
	"-fno-partial-inlining",
	"-fno-tree-copy-prop",
	"-fno-tree-loop-optimize",

	// mips + mips64
	"-msynci",
	"-mno-synci",
	"-mno-fused-madd",

	// x86 + x86_64
	"-finline-limit=300",
	"-fno-inline-functions-called-once",
	"-mfpmath=sse",
	"-mbionic",
})

func init() {
	pctx.StaticVariable("clangExtraCflags", strings.Join([]string{
		"-D__compiler_offsetof=__builtin_offsetof",

		// Help catch common 32/64-bit errors.
		"-Werror=int-conversion",

		// Disable overly aggressive warning for macros defined with a leading underscore
		// This happens in AndroidConfig.h, which is included nearly everywhere.
		// TODO: can we remove this now?
		"-Wno-reserved-id-macro",

		// Disable overly aggressive warning for format strings.
		// Bug: 20148343
		"-Wno-format-pedantic",

		// Workaround for ccache with clang.
		// See http://petereisentraut.blogspot.com/2011/05/ccache-and-clang.html.
		"-Wno-unused-command-line-argument",

		// Force clang to always output color diagnostics. Ninja will strip the ANSI
		// color codes if it is not running in a terminal.
		"-fcolor-diagnostics",
	}, " "))

	pctx.StaticVariable("clangExtraConlyflags", strings.Join([]string{
		"-std=gnu99",
	}, " "))

	pctx.StaticVariable("clangExtraCppflags", strings.Join([]string{
		// Disable -Winconsistent-missing-override until we can clean up the existing
		// codebase for it.
		"-Wno-inconsistent-missing-override",
	}, " "))

	pctx.StaticVariable("clangExtraTargetCflags", strings.Join([]string{
		"-nostdlibinc",
	}, " "))

	pctx.StaticVariable("clangExtraNoOverrideCflags", strings.Join([]string{
		"-Werror=address-of-temporary",
		"-Werror=null-dereference",
		"-Werror=return-type",
	}, " "))
}

func clangFilterUnknownCflags(cflags []string) []string {
	ret := make([]string, 0, len(cflags))
	for _, f := range cflags {
		if !inListSorted(f, clangUnknownCflags) {
			ret = append(ret, f)
		}
	}

	return ret
}

func inListSorted(s string, list []string) bool {
	for _, l := range list {
		if s == l {
			return true
		} else if s < l {
			return false
		}
	}
	return false
}

func sorted(list []string) []string {
	sort.Strings(list)
	return list
}

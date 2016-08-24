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

// This file contains the module types for compiling C/C++ for Android, and converts the properties
// into the flags and filenames necessary to pass to the compiler.  The final creation of the rules
// is handled in builder.go

import (
	"fmt"
	"path/filepath"
	"strings"

	"github.com/google/blueprint"
	"github.com/google/blueprint/proptools"

	"android/soong"
	"android/soong/common"
	"android/soong/genrule"
)

func init() {
	soong.RegisterModuleType("cc_library_static", libraryStaticFactory)
	soong.RegisterModuleType("cc_library_shared", librarySharedFactory)
	soong.RegisterModuleType("cc_library", libraryFactory)
	soong.RegisterModuleType("cc_object", objectFactory)
	soong.RegisterModuleType("cc_binary", binaryFactory)
	soong.RegisterModuleType("cc_test", testFactory)
	soong.RegisterModuleType("cc_benchmark", benchmarkFactory)
	soong.RegisterModuleType("cc_defaults", defaultsFactory)

	soong.RegisterModuleType("toolchain_library", toolchainLibraryFactory)
	soong.RegisterModuleType("ndk_prebuilt_library", ndkPrebuiltLibraryFactory)
	soong.RegisterModuleType("ndk_prebuilt_object", ndkPrebuiltObjectFactory)
	soong.RegisterModuleType("ndk_prebuilt_static_stl", ndkPrebuiltStaticStlFactory)
	soong.RegisterModuleType("ndk_prebuilt_shared_stl", ndkPrebuiltSharedStlFactory)

	soong.RegisterModuleType("cc_library_host_static", libraryHostStaticFactory)
	soong.RegisterModuleType("cc_library_host_shared", libraryHostSharedFactory)
	soong.RegisterModuleType("cc_binary_host", binaryHostFactory)
	soong.RegisterModuleType("cc_test_host", testHostFactory)
	soong.RegisterModuleType("cc_benchmark_host", benchmarkHostFactory)

	// LinkageMutator must be registered after common.ArchMutator, but that is guaranteed by
	// the Go initialization order because this package depends on common, so common's init
	// functions will run first.
	common.RegisterBottomUpMutator("link", linkageMutator)
	common.RegisterBottomUpMutator("test_per_src", testPerSrcMutator)
	common.RegisterBottomUpMutator("deps", depsMutator)
}

var (
	HostPrebuiltTag = pctx.VariableConfigMethod("HostPrebuiltTag", common.Config.PrebuiltOS)

	LibcRoot = pctx.SourcePathVariable("LibcRoot", "bionic/libc")
	LibmRoot = pctx.SourcePathVariable("LibmRoot", "bionic/libm")
)

// Flags used by lots of devices.  Putting them in package static variables will save bytes in
// build.ninja so they aren't repeated for every file
var (
	commonGlobalCflags = []string{
		"-DANDROID",
		"-fmessage-length=0",
		"-W",
		"-Wall",
		"-Wno-unused",
		"-Winit-self",
		"-Wpointer-arith",
		"-fdebug-prefix-map=/proc/self/cwd=",

		// COMMON_RELEASE_CFLAGS
		"-DNDEBUG",
		"-UDEBUG",
	}

	deviceGlobalCflags = []string{
		"-fdiagnostics-color",

		// TARGET_ERROR_FLAGS
		"-Werror=return-type",
		"-Werror=non-virtual-dtor",
		"-Werror=address",
		"-Werror=sequence-point",
		"-Werror=date-time",
	}

	hostGlobalCflags = []string{}

	commonGlobalCppflags = []string{
		"-Wsign-promo",
	}

	noOverrideGlobalCflags = []string{
		"-Werror=int-to-pointer-cast",
		"-Werror=pointer-to-int-cast",
	}

	illegalFlags = []string{
		"-w",
	}
)

func init() {
	pctx.StaticVariable("commonGlobalCflags", strings.Join(commonGlobalCflags, " "))
	pctx.StaticVariable("deviceGlobalCflags", strings.Join(deviceGlobalCflags, " "))
	pctx.StaticVariable("hostGlobalCflags", strings.Join(hostGlobalCflags, " "))
	pctx.StaticVariable("noOverrideGlobalCflags", strings.Join(noOverrideGlobalCflags, " "))

	pctx.StaticVariable("commonGlobalCppflags", strings.Join(commonGlobalCppflags, " "))

	pctx.StaticVariable("commonClangGlobalCflags",
		strings.Join(append(clangFilterUnknownCflags(commonGlobalCflags), "${clangExtraCflags}"), " "))
	pctx.StaticVariable("deviceClangGlobalCflags",
		strings.Join(append(clangFilterUnknownCflags(deviceGlobalCflags), "${clangExtraTargetCflags}"), " "))
	pctx.StaticVariable("hostClangGlobalCflags",
		strings.Join(clangFilterUnknownCflags(hostGlobalCflags), " "))
	pctx.StaticVariable("noOverrideClangGlobalCflags",
		strings.Join(append(clangFilterUnknownCflags(noOverrideGlobalCflags), "${clangExtraNoOverrideCflags}"), " "))

	pctx.StaticVariable("commonClangGlobalCppflags",
		strings.Join(append(clangFilterUnknownCflags(commonGlobalCppflags), "${clangExtraCppflags}"), " "))

	// Everything in this list is a crime against abstraction and dependency tracking.
	// Do not add anything to this list.
	pctx.PrefixedPathsForOptionalSourceVariable("commonGlobalIncludes", "-isystem ",
		[]string{
			"system/core/include",
			"system/media/audio/include",
			"hardware/libhardware/include",
			"hardware/libhardware_legacy/include",
			"hardware/ril/include",
			"libnativehelper/include",
			"frameworks/native/include",
			"frameworks/native/opengl/include",
			"frameworks/av/include",
			"frameworks/base/include",
		})
	// This is used by non-NDK modules to get jni.h. export_include_dirs doesn't help
	// with this, since there is no associated library.
	pctx.PrefixedPathsForOptionalSourceVariable("commonNativehelperInclude", "-I",
		[]string{"libnativehelper/include/nativehelper"})

	pctx.SourcePathVariable("clangDefaultBase", "prebuilts/clang/host")
	pctx.VariableFunc("clangBase", func(config interface{}) (string, error) {
		if override := config.(common.Config).Getenv("LLVM_PREBUILTS_BASE"); override != "" {
			return override, nil
		}
		return "${clangDefaultBase}", nil
	})
	pctx.VariableFunc("clangVersion", func(config interface{}) (string, error) {
		if override := config.(common.Config).Getenv("LLVM_PREBUILTS_VERSION"); override != "" {
			return override, nil
		}
		return "clang-2690385", nil
	})
	pctx.StaticVariable("clangPath", "${clangBase}/${HostPrebuiltTag}/${clangVersion}/bin")
}

type Deps struct {
	SharedLibs, LateSharedLibs                  []string
	StaticLibs, LateStaticLibs, WholeStaticLibs []string

	ObjFiles common.Paths

	Cflags, ReexportedCflags []string

	CrtBegin, CrtEnd string
}

type PathDeps struct {
	SharedLibs, LateSharedLibs                  common.Paths
	StaticLibs, LateStaticLibs, WholeStaticLibs common.Paths

	ObjFiles               common.Paths
	WholeStaticLibObjFiles common.Paths

	Cflags, ReexportedCflags []string

	CrtBegin, CrtEnd common.OptionalPath
}

type Flags struct {
	GlobalFlags []string // Flags that apply to C, C++, and assembly source files
	AsFlags     []string // Flags that apply to assembly source files
	CFlags      []string // Flags that apply to C and C++ source files
	ConlyFlags  []string // Flags that apply to C source files
	CppFlags    []string // Flags that apply to C++ source files
	YaccFlags   []string // Flags that apply to Yacc source files
	LdFlags     []string // Flags that apply to linker command lines

	Nocrt     bool
	Toolchain Toolchain
	Clang     bool

	RequiredInstructionSet string
}

type BaseCompilerProperties struct {
	// list of source files used to compile the C/C++ module.  May be .c, .cpp, or .S files.
	Srcs []string `android:"arch_variant"`

	// list of source files that should not be used to build the C/C++ module.
	// This is most useful in the arch/multilib variants to remove non-common files
	Exclude_srcs []string `android:"arch_variant"`

	// list of module-specific flags that will be used for C and C++ compiles.
	Cflags []string `android:"arch_variant"`

	// list of module-specific flags that will be used for C++ compiles
	Cppflags []string `android:"arch_variant"`

	// list of module-specific flags that will be used for C compiles
	Conlyflags []string `android:"arch_variant"`

	// list of module-specific flags that will be used for .S compiles
	Asflags []string `android:"arch_variant"`

	// list of module-specific flags that will be used for C and C++ compiles when
	// compiling with clang
	Clang_cflags []string `android:"arch_variant"`

	// list of module-specific flags that will be used for .S compiles when
	// compiling with clang
	Clang_asflags []string `android:"arch_variant"`

	// list of module-specific flags that will be used for .y and .yy compiles
	Yaccflags []string

	// the instruction set architecture to use to compile the C/C++
	// module.
	Instruction_set string `android:"arch_variant"`

	// list of directories relative to the root of the source tree that will
	// be added to the include path using -I.
	// If possible, don't use this.  If adding paths from the current directory use
	// local_include_dirs, if adding paths from other modules use export_include_dirs in
	// that module.
	Include_dirs []string `android:"arch_variant"`

	// list of files relative to the root of the source tree that will be included
	// using -include.
	// If possible, don't use this.
	Include_files []string `android:"arch_variant"`

	// list of directories relative to the Blueprints file that will
	// be added to the include path using -I
	Local_include_dirs []string `android:"arch_variant"`

	// list of files relative to the Blueprints file that will be included
	// using -include.
	// If possible, don't use this.
	Local_include_files []string `android:"arch_variant"`

	// pass -frtti instead of -fno-rtti
	Rtti *bool

	Debug, Release struct {
		// list of module-specific flags that will be used for C and C++ compiles in debug or
		// release builds
		Cflags []string `android:"arch_variant"`
	} `android:"arch_variant"`
}

type BaseLinkerProperties struct {
	// list of modules whose object files should be linked into this module
	// in their entirety.  For static library modules, all of the .o files from the intermediate
	// directory of the dependency will be linked into this modules .a file.  For a shared library,
	// the dependency's .a file will be linked into this module using -Wl,--whole-archive.
	Whole_static_libs []string `android:"arch_variant"`

	// list of modules that should be statically linked into this module.
	Static_libs []string `android:"arch_variant"`

	// list of modules that should be dynamically linked into this module.
	Shared_libs []string `android:"arch_variant"`

	// list of module-specific flags that will be used for all link steps
	Ldflags []string `android:"arch_variant"`

	// don't insert default compiler flags into asflags, cflags,
	// cppflags, conlyflags, ldflags, or include_dirs
	No_default_compiler_flags *bool

	// list of system libraries that will be dynamically linked to
	// shared library and executable modules.  If unset, generally defaults to libc
	// and libm.  Set to [] to prevent linking against libc and libm.
	System_shared_libs []string

	// allow the module to contain undefined symbols.  By default,
	// modules cannot contain undefined symbols that are not satisified by their immediate
	// dependencies.  Set this flag to true to remove --no-undefined from the linker flags.
	// This flag should only be necessary for compiling low-level libraries like libc.
	Allow_undefined_symbols *bool

	// don't link in libgcc.a
	No_libgcc *bool

	// -l arguments to pass to linker for host-provided shared libraries
	Host_ldlibs []string `android:"arch_variant"`
}

type LibraryCompilerProperties struct {
	Static struct {
		Srcs         []string `android:"arch_variant"`
		Exclude_srcs []string `android:"arch_variant"`
		Cflags       []string `android:"arch_variant"`
	} `android:"arch_variant"`
	Shared struct {
		Srcs         []string `android:"arch_variant"`
		Exclude_srcs []string `android:"arch_variant"`
		Cflags       []string `android:"arch_variant"`
	} `android:"arch_variant"`
}

type LibraryLinkerProperties struct {
	Static struct {
		Whole_static_libs []string `android:"arch_variant"`
		Static_libs       []string `android:"arch_variant"`
		Shared_libs       []string `android:"arch_variant"`
	} `android:"arch_variant"`
	Shared struct {
		Whole_static_libs []string `android:"arch_variant"`
		Static_libs       []string `android:"arch_variant"`
		Shared_libs       []string `android:"arch_variant"`
	} `android:"arch_variant"`

	// local file name to pass to the linker as --version_script
	Version_script *string `android:"arch_variant"`
	// local file name to pass to the linker as -unexported_symbols_list
	Unexported_symbols_list *string `android:"arch_variant"`
	// local file name to pass to the linker as -force_symbols_not_weak_list
	Force_symbols_not_weak_list *string `android:"arch_variant"`
	// local file name to pass to the linker as -force_symbols_weak_list
	Force_symbols_weak_list *string `android:"arch_variant"`

	// list of directories relative to the Blueprints file that will
	// be added to the include path using -I for any module that links against this module
	Export_include_dirs []string `android:"arch_variant"`

	// don't link in crt_begin and crt_end.  This flag should only be necessary for
	// compiling crt or libc.
	Nocrt *bool `android:"arch_variant"`
}

type BinaryLinkerProperties struct {
	// compile executable with -static
	Static_executable *bool

	// set the name of the output
	Stem string `android:"arch_variant"`

	// append to the name of the output
	Suffix string `android:"arch_variant"`

	// if set, add an extra objcopy --prefix-symbols= step
	Prefix_symbols string
}

type TestLinkerProperties struct {
	// if set, build against the gtest library. Defaults to true.
	Gtest bool

	// Create a separate binary for each source file.  Useful when there is
	// global state that can not be torn down and reset between each test suite.
	Test_per_src *bool
}

// Properties used to compile all C or C++ modules
type BaseProperties struct {
	// compile module with clang instead of gcc
	Clang *bool `android:"arch_variant"`

	// Minimum sdk version supported when compiling against the ndk
	Sdk_version string

	// don't insert default compiler flags into asflags, cflags,
	// cppflags, conlyflags, ldflags, or include_dirs
	No_default_compiler_flags *bool
}

type InstallerProperties struct {
	// install to a subdirectory of the default install path for the module
	Relative_install_path string
}

type UnusedProperties struct {
	Native_coverage  *bool
	Required         []string
	Sanitize         []string `android:"arch_variant"`
	Sanitize_recover []string
	Strip            string
	Tags             []string
}

type ModuleContextIntf interface {
	module() *Module
	static() bool
	staticBinary() bool
	clang() bool
	toolchain() Toolchain
	noDefaultCompilerFlags() bool
	sdk() bool
	sdkVersion() string
}

type ModuleContext interface {
	common.AndroidModuleContext
	ModuleContextIntf
}

type BaseModuleContext interface {
	common.AndroidBaseContext
	ModuleContextIntf
}

type Customizer interface {
	CustomizeProperties(BaseModuleContext)
	Properties() []interface{}
}

type feature interface {
	begin(ctx BaseModuleContext)
	deps(ctx BaseModuleContext, deps Deps) Deps
	flags(ctx ModuleContext, flags Flags) Flags
	props() []interface{}
}

type compiler interface {
	feature
	compile(ctx ModuleContext, flags Flags) common.Paths
}

type linker interface {
	feature
	link(ctx ModuleContext, flags Flags, deps PathDeps, objFiles common.Paths) common.Path
}

type installer interface {
	props() []interface{}
	install(ctx ModuleContext, path common.Path)
	inData() bool
}

// Module contains the properties and members used by all C/C++ module types, and implements
// the blueprint.Module interface.  It delegates to compiler, linker, and installer interfaces
// to construct the output file.  Behavior can be customized with a Customizer interface
type Module struct {
	common.AndroidModuleBase
	common.DefaultableModule

	Properties BaseProperties
	unused     UnusedProperties

	// initialize before calling Init
	hod      common.HostOrDeviceSupported
	multilib common.Multilib

	// delegates, initialize before calling Init
	customizer Customizer
	features   []feature
	compiler   compiler
	linker     linker
	installer  installer

	deps       Deps
	outputFile common.OptionalPath

	cachedToolchain Toolchain
}

func (c *Module) Init() (blueprint.Module, []interface{}) {
	props := []interface{}{&c.Properties, &c.unused}
	if c.customizer != nil {
		props = append(props, c.customizer.Properties()...)
	}
	if c.compiler != nil {
		props = append(props, c.compiler.props()...)
	}
	if c.linker != nil {
		props = append(props, c.linker.props()...)
	}
	if c.installer != nil {
		props = append(props, c.installer.props()...)
	}
	for _, feature := range c.features {
		props = append(props, feature.props()...)
	}

	_, props = common.InitAndroidArchModule(c, c.hod, c.multilib, props...)

	return common.InitDefaultableModule(c, c, props...)
}

type baseModuleContext struct {
	common.AndroidBaseContext
	moduleContextImpl
}

type moduleContext struct {
	common.AndroidModuleContext
	moduleContextImpl
}

type moduleContextImpl struct {
	mod *Module
	ctx BaseModuleContext
}

func (ctx *moduleContextImpl) module() *Module {
	return ctx.mod
}

func (ctx *moduleContextImpl) clang() bool {
	return ctx.mod.clang(ctx.ctx)
}

func (ctx *moduleContextImpl) toolchain() Toolchain {
	return ctx.mod.toolchain(ctx.ctx)
}

func (ctx *moduleContextImpl) static() bool {
	if ctx.mod.linker == nil {
		panic(fmt.Errorf("static called on module %q with no linker", ctx.ctx.ModuleName()))
	}
	if linker, ok := ctx.mod.linker.(baseLinkerInterface); ok {
		return linker.static()
	} else {
		panic(fmt.Errorf("static called on module %q that doesn't use base linker", ctx.ctx.ModuleName()))
	}
}

func (ctx *moduleContextImpl) staticBinary() bool {
	if ctx.mod.linker == nil {
		panic(fmt.Errorf("staticBinary called on module %q with no linker", ctx.ctx.ModuleName()))
	}
	if linker, ok := ctx.mod.linker.(baseLinkerInterface); ok {
		return linker.staticBinary()
	} else {
		panic(fmt.Errorf("staticBinary called on module %q that doesn't use base linker", ctx.ctx.ModuleName()))
	}
}

func (ctx *moduleContextImpl) noDefaultCompilerFlags() bool {
	return Bool(ctx.mod.Properties.No_default_compiler_flags)
}

func (ctx *moduleContextImpl) sdk() bool {
	return ctx.mod.Properties.Sdk_version != ""
}

func (ctx *moduleContextImpl) sdkVersion() string {
	return ctx.mod.Properties.Sdk_version
}

func newBaseModule(hod common.HostOrDeviceSupported, multilib common.Multilib) *Module {
	return &Module{
		hod:      hod,
		multilib: multilib,
	}
}

func newModule(hod common.HostOrDeviceSupported, multilib common.Multilib) *Module {
	module := newBaseModule(hod, multilib)
	module.features = []feature{
		&stlFeature{},
	}
	return module
}

func (c *Module) GenerateAndroidBuildActions(actx common.AndroidModuleContext) {
	ctx := &moduleContext{
		AndroidModuleContext: actx,
		moduleContextImpl: moduleContextImpl{
			mod: c,
		},
	}
	ctx.ctx = ctx

	flags := Flags{
		Toolchain: c.toolchain(ctx),
		Clang:     c.clang(ctx),
	}

	if c.compiler != nil {
		flags = c.compiler.flags(ctx, flags)
	}
	if c.linker != nil {
		flags = c.linker.flags(ctx, flags)
	}
	for _, feature := range c.features {
		flags = feature.flags(ctx, flags)
	}
	if ctx.Failed() {
		return
	}

	flags.CFlags, _ = filterList(flags.CFlags, illegalFlags)
	flags.CppFlags, _ = filterList(flags.CppFlags, illegalFlags)
	flags.ConlyFlags, _ = filterList(flags.ConlyFlags, illegalFlags)

	// Optimization to reduce size of build.ninja
	// Replace the long list of flags for each file with a module-local variable
	ctx.Variable(pctx, "cflags", strings.Join(flags.CFlags, " "))
	ctx.Variable(pctx, "cppflags", strings.Join(flags.CppFlags, " "))
	ctx.Variable(pctx, "asflags", strings.Join(flags.AsFlags, " "))
	flags.CFlags = []string{"$cflags"}
	flags.CppFlags = []string{"$cppflags"}
	flags.AsFlags = []string{"$asflags"}

	deps := c.depsToPaths(actx, c.deps)
	if ctx.Failed() {
		return
	}

	flags.CFlags = append(flags.CFlags, deps.Cflags...)

	var objFiles common.Paths
	if c.compiler != nil {
		objFiles = c.compiler.compile(ctx, flags)
		if ctx.Failed() {
			return
		}
	}

	if c.linker != nil {
		outputFile := c.linker.link(ctx, flags, deps, objFiles)
		if ctx.Failed() {
			return
		}
		c.outputFile = common.OptionalPathForPath(outputFile)

		if c.installer != nil {
			c.installer.install(ctx, outputFile)
			if ctx.Failed() {
				return
			}
		}
	}
}

func (c *Module) toolchain(ctx BaseModuleContext) Toolchain {
	if c.cachedToolchain == nil {
		arch := ctx.Arch()
		hod := ctx.HostOrDevice()
		ht := ctx.HostType()
		factory := toolchainFactories[hod][ht][arch.ArchType]
		if factory == nil {
			ctx.ModuleErrorf("Toolchain not found for %s %s arch %q", hod.String(), ht.String(), arch.String())
			return nil
		}
		c.cachedToolchain = factory(arch)
	}
	return c.cachedToolchain
}

func (c *Module) begin(ctx BaseModuleContext) {
	if c.compiler != nil {
		c.compiler.begin(ctx)
	}
	if c.linker != nil {
		c.linker.begin(ctx)
	}
	for _, feature := range c.features {
		feature.begin(ctx)
	}
}

func (c *Module) depsMutator(actx common.AndroidBottomUpMutatorContext) {
	ctx := &baseModuleContext{
		AndroidBaseContext: actx,
		moduleContextImpl: moduleContextImpl{
			mod: c,
		},
	}
	ctx.ctx = ctx

	if c.customizer != nil {
		c.customizer.CustomizeProperties(ctx)
	}

	c.begin(ctx)

	c.deps = Deps{}

	if c.compiler != nil {
		c.deps = c.compiler.deps(ctx, c.deps)
	}
	if c.linker != nil {
		c.deps = c.linker.deps(ctx, c.deps)
	}
	for _, feature := range c.features {
		c.deps = feature.deps(ctx, c.deps)
	}

	c.deps.WholeStaticLibs = lastUniqueElements(c.deps.WholeStaticLibs)
	c.deps.StaticLibs = lastUniqueElements(c.deps.StaticLibs)
	c.deps.LateStaticLibs = lastUniqueElements(c.deps.LateStaticLibs)
	c.deps.SharedLibs = lastUniqueElements(c.deps.SharedLibs)
	c.deps.LateSharedLibs = lastUniqueElements(c.deps.LateSharedLibs)

	staticLibs := c.deps.WholeStaticLibs
	staticLibs = append(staticLibs, c.deps.StaticLibs...)
	staticLibs = append(staticLibs, c.deps.LateStaticLibs...)
	actx.AddVariationDependencies([]blueprint.Variation{{"link", "static"}}, staticLibs...)

	sharedLibs := c.deps.SharedLibs
	sharedLibs = append(sharedLibs, c.deps.LateSharedLibs...)
	actx.AddVariationDependencies([]blueprint.Variation{{"link", "shared"}}, sharedLibs...)

	actx.AddDependency(ctx.module(), c.deps.ObjFiles.Strings()...)
	if c.deps.CrtBegin != "" {
		actx.AddDependency(ctx.module(), c.deps.CrtBegin)
	}
	if c.deps.CrtEnd != "" {
		actx.AddDependency(ctx.module(), c.deps.CrtEnd)
	}
}

func depsMutator(ctx common.AndroidBottomUpMutatorContext) {
	if c, ok := ctx.Module().(*Module); ok {
		c.depsMutator(ctx)
	}
}

func (c *Module) clang(ctx BaseModuleContext) bool {
	clang := Bool(c.Properties.Clang)

	if c.Properties.Clang == nil {
		if ctx.Host() {
			clang = true
		}

		if ctx.Device() && ctx.AConfig().DeviceUsesClang() {
			clang = true
		}
	}

	if !c.toolchain(ctx).ClangSupported() {
		clang = false
	}

	return clang
}

func (c *Module) depsToPathsFromList(ctx common.AndroidModuleContext,
	names []string) (modules []common.AndroidModule,
	outputFiles common.Paths, exportedFlags []string) {

	for _, n := range names {
		found := false
		ctx.VisitDirectDeps(func(m blueprint.Module) {
			otherName := ctx.OtherModuleName(m)
			if otherName != n {
				return
			}

			if a, ok := m.(*Module); ok {
				if !a.Enabled() {
					ctx.ModuleErrorf("depends on disabled module %q", otherName)
					return
				}
				if a.HostOrDevice() != ctx.HostOrDevice() {
					ctx.ModuleErrorf("host/device mismatch between %q and %q", ctx.ModuleName(),
						otherName)
					return
				}

				if outputFile := a.outputFile; outputFile.Valid() {
					if found {
						ctx.ModuleErrorf("multiple modules satisified dependency on %q", otherName)
						return
					}
					outputFiles = append(outputFiles, outputFile.Path())
					modules = append(modules, a)
					if i, ok := a.linker.(exportedFlagsProducer); ok {
						exportedFlags = append(exportedFlags, i.exportedFlags()...)
					}
					found = true
				} else {
					ctx.ModuleErrorf("module %q missing output file", otherName)
					return
				}
			} else {
				ctx.ModuleErrorf("module %q not an android module", otherName)
				return
			}
		})
		if !found && !inList(n, ctx.GetMissingDependencies()) {
			ctx.ModuleErrorf("unsatisified dependency on %q", n)
		}
	}

	return modules, outputFiles, exportedFlags
}

// Convert dependency names to paths.  Takes a Deps containing names and returns a PathDeps
// containing paths
func (c *Module) depsToPaths(ctx common.AndroidModuleContext, deps Deps) PathDeps {
	var depPaths PathDeps
	var newCflags []string

	var wholeStaticLibModules []common.AndroidModule

	wholeStaticLibModules, depPaths.WholeStaticLibs, newCflags =
		c.depsToPathsFromList(ctx, deps.WholeStaticLibs)
	depPaths.Cflags = append(depPaths.Cflags, newCflags...)
	depPaths.ReexportedCflags = append(depPaths.ReexportedCflags, newCflags...)

	for _, am := range wholeStaticLibModules {
		if m, ok := am.(*Module); ok {
			if staticLib, ok := m.linker.(*libraryLinker); ok && staticLib.static() {
				if missingDeps := staticLib.getWholeStaticMissingDeps(); missingDeps != nil {
					postfix := " (required by " + ctx.OtherModuleName(m) + ")"
					for i := range missingDeps {
						missingDeps[i] += postfix
					}
					ctx.AddMissingDependencies(missingDeps)
				}
				depPaths.WholeStaticLibObjFiles =
					append(depPaths.WholeStaticLibObjFiles, staticLib.objFiles...)
			} else {
				ctx.ModuleErrorf("module %q not a static library", ctx.OtherModuleName(m))
			}
		} else {
			ctx.ModuleErrorf("module %q not an android module", ctx.OtherModuleName(m))
		}
	}

	_, depPaths.StaticLibs, newCflags = c.depsToPathsFromList(ctx, deps.StaticLibs)
	depPaths.Cflags = append(depPaths.Cflags, newCflags...)

	_, depPaths.LateStaticLibs, newCflags = c.depsToPathsFromList(ctx, deps.LateStaticLibs)
	depPaths.Cflags = append(depPaths.Cflags, newCflags...)

	_, depPaths.SharedLibs, newCflags = c.depsToPathsFromList(ctx, deps.SharedLibs)
	depPaths.Cflags = append(depPaths.Cflags, newCflags...)

	_, depPaths.LateSharedLibs, newCflags = c.depsToPathsFromList(ctx, deps.LateSharedLibs)
	depPaths.Cflags = append(depPaths.Cflags, newCflags...)

	ctx.VisitDirectDeps(func(bm blueprint.Module) {
		if m, ok := bm.(*Module); ok {
			otherName := ctx.OtherModuleName(m)
			if otherName == deps.CrtBegin {
				depPaths.CrtBegin = m.outputFile
			} else if otherName == deps.CrtEnd {
				depPaths.CrtEnd = m.outputFile
			} else {
				output := m.outputFile
				if output.Valid() {
					depPaths.ObjFiles = append(depPaths.ObjFiles, output.Path())
				} else {
					ctx.ModuleErrorf("module %s did not provide an output file", otherName)
				}
			}
		}
	})

	return depPaths
}

func (c *Module) InstallInData() bool {
	if c.installer == nil {
		return false
	}
	return c.installer.inData()
}

// Compiler

type baseCompiler struct {
	Properties BaseCompilerProperties
}

var _ compiler = (*baseCompiler)(nil)

func (compiler *baseCompiler) props() []interface{} {
	return []interface{}{&compiler.Properties}
}

func (compiler *baseCompiler) begin(ctx BaseModuleContext)                {}
func (compiler *baseCompiler) deps(ctx BaseModuleContext, deps Deps) Deps { return deps }

// Create a Flags struct that collects the compile flags from global values,
// per-target values, module type values, and per-module Blueprints properties
func (compiler *baseCompiler) flags(ctx ModuleContext, flags Flags) Flags {
	toolchain := ctx.toolchain()

	flags.CFlags = append(flags.CFlags, compiler.Properties.Cflags...)
	flags.CppFlags = append(flags.CppFlags, compiler.Properties.Cppflags...)
	flags.ConlyFlags = append(flags.ConlyFlags, compiler.Properties.Conlyflags...)
	flags.AsFlags = append(flags.AsFlags, compiler.Properties.Asflags...)
	flags.YaccFlags = append(flags.YaccFlags, compiler.Properties.Yaccflags...)

	// Include dir cflags
	rootIncludeDirs := common.PathsForSource(ctx, compiler.Properties.Include_dirs)
	localIncludeDirs := common.PathsForModuleSrc(ctx, compiler.Properties.Local_include_dirs)
	flags.GlobalFlags = append(flags.GlobalFlags,
		includeDirsToFlags(localIncludeDirs),
		includeDirsToFlags(rootIncludeDirs))

	rootIncludeFiles := common.PathsForSource(ctx, compiler.Properties.Include_files)
	localIncludeFiles := common.PathsForModuleSrc(ctx, compiler.Properties.Local_include_files)

	flags.GlobalFlags = append(flags.GlobalFlags,
		includeFilesToFlags(rootIncludeFiles),
		includeFilesToFlags(localIncludeFiles))

	if !ctx.noDefaultCompilerFlags() {
		if !ctx.sdk() || ctx.Host() {
			flags.GlobalFlags = append(flags.GlobalFlags,
				"${commonGlobalIncludes}",
				toolchain.IncludeFlags(),
				"${commonNativehelperInclude}")
		}

		flags.GlobalFlags = append(flags.GlobalFlags, []string{
			"-I" + common.PathForModuleSrc(ctx).String(),
			"-I" + common.PathForModuleOut(ctx).String(),
			"-I" + common.PathForModuleGen(ctx).String(),
		}...)
	}

	instructionSet := compiler.Properties.Instruction_set
	if flags.RequiredInstructionSet != "" {
		instructionSet = flags.RequiredInstructionSet
	}
	instructionSetFlags, err := toolchain.InstructionSetFlags(instructionSet)
	if flags.Clang {
		instructionSetFlags, err = toolchain.ClangInstructionSetFlags(instructionSet)
	}
	if err != nil {
		ctx.ModuleErrorf("%s", err)
	}

	// TODO: debug
	flags.CFlags = append(flags.CFlags, compiler.Properties.Release.Cflags...)

	if flags.Clang {
		flags.CFlags = clangFilterUnknownCflags(flags.CFlags)
		flags.CFlags = append(flags.CFlags, compiler.Properties.Clang_cflags...)
		flags.AsFlags = append(flags.AsFlags, compiler.Properties.Clang_asflags...)
		flags.CppFlags = clangFilterUnknownCflags(flags.CppFlags)
		flags.ConlyFlags = clangFilterUnknownCflags(flags.ConlyFlags)
		flags.LdFlags = clangFilterUnknownCflags(flags.LdFlags)

		target := "-target " + toolchain.ClangTriple()
		gccPrefix := "-B" + filepath.Join(toolchain.GccRoot(), toolchain.GccTriple(), "bin")

		flags.CFlags = append(flags.CFlags, target, gccPrefix)
		flags.AsFlags = append(flags.AsFlags, target, gccPrefix)
		flags.LdFlags = append(flags.LdFlags, target, gccPrefix)
	}

	if !ctx.noDefaultCompilerFlags() {
		flags.GlobalFlags = append(flags.GlobalFlags, instructionSetFlags)

		if flags.Clang {
			flags.AsFlags = append(flags.AsFlags, toolchain.ClangAsflags())
			flags.CppFlags = append(flags.CppFlags, "${commonClangGlobalCppflags}")
			flags.GlobalFlags = append(flags.GlobalFlags,
				toolchain.ClangCflags(),
				"${commonClangGlobalCflags}",
				fmt.Sprintf("${%sClangGlobalCflags}", ctx.HostOrDevice()))

			flags.ConlyFlags = append(flags.ConlyFlags, "${clangExtraConlyflags}")
		} else {
			flags.CppFlags = append(flags.CppFlags, "${commonGlobalCppflags}")
			flags.GlobalFlags = append(flags.GlobalFlags,
				toolchain.Cflags(),
				"${commonGlobalCflags}",
				fmt.Sprintf("${%sGlobalCflags}", ctx.HostOrDevice()))
		}

		if Bool(ctx.AConfig().ProductVariables.Brillo) {
			flags.GlobalFlags = append(flags.GlobalFlags, "-D__BRILLO__")
		}

		if ctx.Device() {
			if Bool(compiler.Properties.Rtti) {
				flags.CppFlags = append(flags.CppFlags, "-frtti")
			} else {
				flags.CppFlags = append(flags.CppFlags, "-fno-rtti")
			}
		}

		flags.AsFlags = append(flags.AsFlags, "-D__ASSEMBLY__")

		if flags.Clang {
			flags.CppFlags = append(flags.CppFlags, toolchain.ClangCppflags())
		} else {
			flags.CppFlags = append(flags.CppFlags, toolchain.Cppflags())
		}
	}

	if flags.Clang {
		flags.GlobalFlags = append(flags.GlobalFlags, toolchain.ToolchainClangCflags())
	} else {
		flags.GlobalFlags = append(flags.GlobalFlags, toolchain.ToolchainCflags())
	}
	flags.LdFlags = append(flags.LdFlags, toolchain.ToolchainLdflags())

	if !ctx.sdk() {
		if ctx.Host() && !flags.Clang {
			// The host GCC doesn't support C++14 (and is deprecated, so likely
			// never will). Build these modules with C++11.
			flags.CppFlags = append(flags.CppFlags, "-std=gnu++11")
		} else {
			flags.CppFlags = append(flags.CppFlags, "-std=gnu++14")
		}
	}

	// We can enforce some rules more strictly in the code we own. strict
	// indicates if this is code that we can be stricter with. If we have
	// rules that we want to apply to *our* code (but maybe can't for
	// vendor/device specific things), we could extend this to be a ternary
	// value.
	strict := true
	if strings.HasPrefix(common.PathForModuleSrc(ctx).String(), "external/") {
		strict = false
	}

	// Can be used to make some annotations stricter for code we can fix
	// (such as when we mark functions as deprecated).
	if strict {
		flags.CFlags = append(flags.CFlags, "-DANDROID_STRICT")
	}

	return flags
}

func (compiler *baseCompiler) compile(ctx ModuleContext, flags Flags) common.Paths {
	// Compile files listed in c.Properties.Srcs into objects
	objFiles := compiler.compileObjs(ctx, flags, "", compiler.Properties.Srcs, compiler.Properties.Exclude_srcs)
	if ctx.Failed() {
		return nil
	}

	var genSrcs common.Paths
	ctx.VisitDirectDeps(func(module blueprint.Module) {
		if gen, ok := module.(genrule.SourceFileGenerator); ok {
			genSrcs = append(genSrcs, gen.GeneratedSourceFiles()...)
		}
	})

	if len(genSrcs) != 0 {
		genObjs := TransformSourceToObj(ctx, "", genSrcs, flagsToBuilderFlags(flags), nil)
		objFiles = append(objFiles, genObjs...)
	}

	return objFiles
}

// Compile a list of source files into objects a specified subdirectory
func (compiler *baseCompiler) compileObjs(ctx common.AndroidModuleContext, flags Flags,
	subdir string, srcFiles, excludes []string) common.Paths {

	buildFlags := flagsToBuilderFlags(flags)

	inputFiles := ctx.ExpandSources(srcFiles, excludes)
	srcPaths, deps := genSources(ctx, inputFiles, buildFlags)

	return TransformSourceToObj(ctx, subdir, srcPaths, buildFlags, deps)
}

// baseLinker provides support for shared_libs, static_libs, and whole_static_libs properties
type baseLinker struct {
	Properties        BaseLinkerProperties
	dynamicProperties struct {
		VariantIsShared       bool `blueprint:"mutated"`
		VariantIsStatic       bool `blueprint:"mutated"`
		VariantIsStaticBinary bool `blueprint:"mutated"`
	}
}

func (linker *baseLinker) begin(ctx BaseModuleContext) {}

func (linker *baseLinker) props() []interface{} {
	return []interface{}{&linker.Properties, &linker.dynamicProperties}
}

func (linker *baseLinker) deps(ctx BaseModuleContext, deps Deps) Deps {
	deps.WholeStaticLibs = append(deps.WholeStaticLibs, linker.Properties.Whole_static_libs...)
	deps.StaticLibs = append(deps.StaticLibs, linker.Properties.Static_libs...)
	deps.SharedLibs = append(deps.SharedLibs, linker.Properties.Shared_libs...)

	if ctx.ModuleName() != "libcompiler_rt-extras" {
		deps.StaticLibs = append(deps.StaticLibs, "libcompiler_rt-extras")
	}

	if ctx.Device() {
		// libgcc and libatomic have to be last on the command line
		deps.LateStaticLibs = append(deps.LateStaticLibs, "libatomic")
		if !Bool(linker.Properties.No_libgcc) {
			deps.LateStaticLibs = append(deps.LateStaticLibs, "libgcc")
		}

		if !linker.static() {
			if linker.Properties.System_shared_libs != nil {
				deps.LateSharedLibs = append(deps.LateSharedLibs,
					linker.Properties.System_shared_libs...)
			} else if !ctx.sdk() {
				deps.LateSharedLibs = append(deps.LateSharedLibs, "libc", "libm")
			}
		}

		if ctx.sdk() {
			version := ctx.sdkVersion()
			deps.SharedLibs = append(deps.SharedLibs,
				"ndk_libc."+version,
				"ndk_libm."+version,
			)
		}
	}

	return deps
}

func (linker *baseLinker) flags(ctx ModuleContext, flags Flags) Flags {
	toolchain := ctx.toolchain()

	flags.LdFlags = append(flags.LdFlags, linker.Properties.Ldflags...)

	if !ctx.noDefaultCompilerFlags() {
		if ctx.Device() && !Bool(linker.Properties.Allow_undefined_symbols) {
			flags.LdFlags = append(flags.LdFlags, "-Wl,--no-undefined")
		}

		if flags.Clang {
			flags.LdFlags = append(flags.LdFlags, toolchain.ClangLdflags())
		} else {
			flags.LdFlags = append(flags.LdFlags, toolchain.Ldflags())
		}

		if ctx.Host() {
			flags.LdFlags = append(flags.LdFlags, linker.Properties.Host_ldlibs...)
		}
	}

	if !flags.Clang {
		flags.LdFlags = append(flags.LdFlags, toolchain.ToolchainLdflags())
	}

	return flags
}

func (linker *baseLinker) static() bool {
	return linker.dynamicProperties.VariantIsStatic
}

func (linker *baseLinker) staticBinary() bool {
	return linker.dynamicProperties.VariantIsStaticBinary
}

func (linker *baseLinker) setStatic(static bool) {
	linker.dynamicProperties.VariantIsStatic = static
}

type baseLinkerInterface interface {
	// Returns true if the build options for the module have selected a static or shared build
	buildStatic() bool
	buildShared() bool

	// Sets whether a specific variant is static or shared
	setStatic(bool)

	// Returns whether a specific variant is a static library or binary
	static() bool

	// Returns whether a module is a static binary
	staticBinary() bool
}

type exportedFlagsProducer interface {
	exportedFlags() []string
}

type baseInstaller struct {
	Properties InstallerProperties

	dir   string
	dir64 string
	data  bool

	path common.OutputPath
}

var _ installer = (*baseInstaller)(nil)

func (installer *baseInstaller) props() []interface{} {
	return []interface{}{&installer.Properties}
}

func (installer *baseInstaller) install(ctx ModuleContext, file common.Path) {
	subDir := installer.dir
	if ctx.toolchain().Is64Bit() && installer.dir64 != "" {
		subDir = installer.dir64
	}
	dir := common.PathForModuleInstall(ctx, subDir, installer.Properties.Relative_install_path)
	installer.path = ctx.InstallFile(dir, file)
}

func (installer *baseInstaller) inData() bool {
	return installer.data
}

//
// Combined static+shared libraries
//

type libraryCompiler struct {
	baseCompiler

	linker     *libraryLinker
	Properties LibraryCompilerProperties

	// For reusing static library objects for shared library
	reuseFrom     *libraryCompiler
	reuseObjFiles common.Paths
}

var _ compiler = (*libraryCompiler)(nil)

func (library *libraryCompiler) props() []interface{} {
	props := library.baseCompiler.props()
	return append(props, &library.Properties)
}

func (library *libraryCompiler) flags(ctx ModuleContext, flags Flags) Flags {
	flags = library.baseCompiler.flags(ctx, flags)

	// MinGW spits out warnings about -fPIC even for -fpie?!) being ignored because
	// all code is position independent, and then those warnings get promoted to
	// errors.
	if ctx.HostType() != common.Windows {
		flags.CFlags = append(flags.CFlags, "-fPIC")
	}

	if library.linker.static() {
		flags.CFlags = append(flags.CFlags, library.Properties.Static.Cflags...)
	} else {
		flags.CFlags = append(flags.CFlags, library.Properties.Shared.Cflags...)
	}

	return flags
}

func (library *libraryCompiler) compile(ctx ModuleContext, flags Flags) common.Paths {
	var objFiles common.Paths

	if library.reuseFrom != library && library.reuseFrom.Properties.Static.Cflags == nil &&
		library.Properties.Shared.Cflags == nil {
		objFiles = append(common.Paths(nil), library.reuseFrom.reuseObjFiles...)
	} else {
		objFiles = library.baseCompiler.compile(ctx, flags)
		library.reuseObjFiles = objFiles
	}

	if library.linker.static() {
		objFiles = append(objFiles, library.compileObjs(ctx, flags, common.DeviceStaticLibrary,
			library.Properties.Static.Srcs, library.Properties.Static.Exclude_srcs)...)
	} else {
		objFiles = append(objFiles, library.compileObjs(ctx, flags, common.DeviceSharedLibrary,
			library.Properties.Shared.Srcs, library.Properties.Shared.Exclude_srcs)...)
	}

	return objFiles
}

type libraryLinker struct {
	baseLinker

	Properties LibraryLinkerProperties

	dynamicProperties struct {
		BuildStatic bool `blueprint:"mutated"`
		BuildShared bool `blueprint:"mutated"`
	}

	exportFlags []string

	// If we're used as a whole_static_lib, our missing dependencies need
	// to be given
	wholeStaticMissingDeps []string

	// For whole_static_libs
	objFiles common.Paths
}

var _ linker = (*libraryLinker)(nil)
var _ exportedFlagsProducer = (*libraryLinker)(nil)

func (library *libraryLinker) props() []interface{} {
	props := library.baseLinker.props()
	return append(props, &library.Properties, &library.dynamicProperties)
}

func (library *libraryLinker) flags(ctx ModuleContext, flags Flags) Flags {
	flags = library.baseLinker.flags(ctx, flags)

	flags.Nocrt = Bool(library.Properties.Nocrt)

	if !library.static() {
		libName := ctx.ModuleName()
		// GCC for Android assumes that -shared means -Bsymbolic, use -Wl,-shared instead
		sharedFlag := "-Wl,-shared"
		if flags.Clang || ctx.Host() {
			sharedFlag = "-shared"
		}
		if ctx.Device() {
			flags.LdFlags = append(flags.LdFlags,
				"-nostdlib",
				"-Wl,--gc-sections",
			)
		}

		if ctx.Darwin() {
			flags.LdFlags = append(flags.LdFlags,
				"-dynamiclib",
				"-single_module",
				//"-read_only_relocs suppress",
				"-install_name @rpath/"+libName+flags.Toolchain.ShlibSuffix(),
			)
		} else {
			flags.LdFlags = append(flags.LdFlags,
				sharedFlag,
				"-Wl,-soname,"+libName+flags.Toolchain.ShlibSuffix(),
			)
		}
	}

	return flags
}

func (library *libraryLinker) deps(ctx BaseModuleContext, deps Deps) Deps {
	deps = library.baseLinker.deps(ctx, deps)
	if library.static() {
		deps.WholeStaticLibs = append(deps.WholeStaticLibs, library.Properties.Static.Whole_static_libs...)
		deps.StaticLibs = append(deps.StaticLibs, library.Properties.Static.Static_libs...)
		deps.SharedLibs = append(deps.SharedLibs, library.Properties.Static.Shared_libs...)
	} else {
		if ctx.Device() && !Bool(library.Properties.Nocrt) {
			if !ctx.sdk() {
				deps.CrtBegin = "crtbegin_so"
				deps.CrtEnd = "crtend_so"
			} else {
				deps.CrtBegin = "ndk_crtbegin_so." + ctx.sdkVersion()
				deps.CrtEnd = "ndk_crtend_so." + ctx.sdkVersion()
			}
		}
		deps.WholeStaticLibs = append(deps.WholeStaticLibs, library.Properties.Shared.Whole_static_libs...)
		deps.StaticLibs = append(deps.StaticLibs, library.Properties.Shared.Static_libs...)
		deps.SharedLibs = append(deps.SharedLibs, library.Properties.Shared.Shared_libs...)
	}

	return deps
}

func (library *libraryLinker) exportedFlags() []string {
	return library.exportFlags
}

func (library *libraryLinker) linkStatic(ctx ModuleContext,
	flags Flags, deps PathDeps, objFiles common.Paths) common.Path {

	objFiles = append(objFiles, deps.WholeStaticLibObjFiles...)
	library.objFiles = objFiles

	outputFile := common.PathForModuleOut(ctx, ctx.ModuleName()+staticLibraryExtension)

	if ctx.Darwin() {
		TransformDarwinObjToStaticLib(ctx, objFiles, flagsToBuilderFlags(flags), outputFile)
	} else {
		TransformObjToStaticLib(ctx, objFiles, flagsToBuilderFlags(flags), outputFile)
	}

	library.wholeStaticMissingDeps = ctx.GetMissingDependencies()

	ctx.CheckbuildFile(outputFile)

	return outputFile
}

func (library *libraryLinker) linkShared(ctx ModuleContext,
	flags Flags, deps PathDeps, objFiles common.Paths) common.Path {

	outputFile := common.PathForModuleOut(ctx, ctx.ModuleName()+flags.Toolchain.ShlibSuffix())

	var linkerDeps common.Paths

	versionScript := common.OptionalPathForModuleSrc(ctx, library.Properties.Version_script)
	unexportedSymbols := common.OptionalPathForModuleSrc(ctx, library.Properties.Unexported_symbols_list)
	forceNotWeakSymbols := common.OptionalPathForModuleSrc(ctx, library.Properties.Force_symbols_not_weak_list)
	forceWeakSymbols := common.OptionalPathForModuleSrc(ctx, library.Properties.Force_symbols_weak_list)
	if !ctx.Darwin() {
		if versionScript.Valid() {
			flags.LdFlags = append(flags.LdFlags, "-Wl,--version-script,"+versionScript.String())
			linkerDeps = append(linkerDeps, versionScript.Path())
		}
		if unexportedSymbols.Valid() {
			ctx.PropertyErrorf("unexported_symbols_list", "Only supported on Darwin")
		}
		if forceNotWeakSymbols.Valid() {
			ctx.PropertyErrorf("force_symbols_not_weak_list", "Only supported on Darwin")
		}
		if forceWeakSymbols.Valid() {
			ctx.PropertyErrorf("force_symbols_weak_list", "Only supported on Darwin")
		}
	} else {
		if versionScript.Valid() {
			ctx.PropertyErrorf("version_script", "Not supported on Darwin")
		}
		if unexportedSymbols.Valid() {
			flags.LdFlags = append(flags.LdFlags, "-Wl,-unexported_symbols_list,"+unexportedSymbols.String())
			linkerDeps = append(linkerDeps, unexportedSymbols.Path())
		}
		if forceNotWeakSymbols.Valid() {
			flags.LdFlags = append(flags.LdFlags, "-Wl,-force_symbols_not_weak_list,"+forceNotWeakSymbols.String())
			linkerDeps = append(linkerDeps, forceNotWeakSymbols.Path())
		}
		if forceWeakSymbols.Valid() {
			flags.LdFlags = append(flags.LdFlags, "-Wl,-force_symbols_weak_list,"+forceWeakSymbols.String())
			linkerDeps = append(linkerDeps, forceWeakSymbols.Path())
		}
	}

	sharedLibs := deps.SharedLibs
	sharedLibs = append(sharedLibs, deps.LateSharedLibs...)

	TransformObjToDynamicBinary(ctx, objFiles, sharedLibs,
		deps.StaticLibs, deps.LateStaticLibs, deps.WholeStaticLibs,
		linkerDeps, deps.CrtBegin, deps.CrtEnd, false, flagsToBuilderFlags(flags), outputFile)

	return outputFile
}

func (library *libraryLinker) link(ctx ModuleContext,
	flags Flags, deps PathDeps, objFiles common.Paths) common.Path {

	var out common.Path
	if library.static() {
		out = library.linkStatic(ctx, flags, deps, objFiles)
	} else {
		out = library.linkShared(ctx, flags, deps, objFiles)
	}

	includeDirs := common.PathsForModuleSrc(ctx, library.Properties.Export_include_dirs)
	library.exportFlags = []string{includeDirsToFlags(includeDirs)}
	library.exportFlags = append(library.exportFlags, deps.ReexportedCflags...)

	return out
}

func (library *libraryLinker) buildStatic() bool {
	return library.dynamicProperties.BuildStatic
}

func (library *libraryLinker) buildShared() bool {
	return library.dynamicProperties.BuildShared
}

func (library *libraryLinker) getWholeStaticMissingDeps() []string {
	return library.wholeStaticMissingDeps
}

type libraryInstaller struct {
	baseInstaller

	linker *libraryLinker
}

func (library *libraryInstaller) install(ctx ModuleContext, file common.Path) {
	if !library.linker.static() {
		library.baseInstaller.install(ctx, file)
	}
}

func NewLibrary(hod common.HostOrDeviceSupported, shared, static bool) *Module {
	module := newModule(hod, common.MultilibBoth)

	linker := &libraryLinker{}
	linker.dynamicProperties.BuildShared = shared
	linker.dynamicProperties.BuildStatic = static
	module.linker = linker

	module.compiler = &libraryCompiler{
		linker: linker,
	}
	module.installer = &libraryInstaller{
		baseInstaller: baseInstaller{
			dir:   "lib",
			dir64: "lib64",
		},
		linker: linker,
	}

	return module
}

func libraryFactory() (blueprint.Module, []interface{}) {
	module := NewLibrary(common.HostAndDeviceSupported, true, true)
	return module.Init()
}

//
// Objects (for crt*.o)
//

type objectLinker struct {
}

func objectFactory() (blueprint.Module, []interface{}) {
	module := newBaseModule(common.DeviceSupported, common.MultilibBoth)
	module.compiler = &baseCompiler{}
	module.linker = &objectLinker{}
	return module.Init()
}

func (*objectLinker) props() []interface{} {
	return nil
}

func (*objectLinker) begin(ctx BaseModuleContext) {}

func (*objectLinker) deps(ctx BaseModuleContext, deps Deps) Deps {
	// object files can't have any dynamic dependencies
	return deps
}

func (*objectLinker) flags(ctx ModuleContext, flags Flags) Flags {
	return flags
}

func (object *objectLinker) link(ctx ModuleContext,
	flags Flags, deps PathDeps, objFiles common.Paths) common.Path {

	objFiles = append(objFiles, deps.ObjFiles...)

	var outputFile common.Path
	if len(objFiles) == 1 {
		outputFile = objFiles[0]
	} else {
		output := common.PathForModuleOut(ctx, ctx.ModuleName()+objectExtension)
		TransformObjsToObj(ctx, objFiles, flagsToBuilderFlags(flags), output)
		outputFile = output
	}

	ctx.CheckbuildFile(outputFile)
	return outputFile
}

//
// Executables
//

type binaryLinker struct {
	baseLinker

	Properties BinaryLinkerProperties

	hostToolPath common.OptionalPath
}

var _ linker = (*binaryLinker)(nil)

func (binary *binaryLinker) props() []interface{} {
	return append(binary.baseLinker.props(), &binary.Properties)
}

func (binary *binaryLinker) buildStatic() bool {
	return Bool(binary.Properties.Static_executable)
}

func (binary *binaryLinker) buildShared() bool {
	return !Bool(binary.Properties.Static_executable)
}

func (binary *binaryLinker) getStem(ctx BaseModuleContext) string {
	stem := ctx.ModuleName()
	if binary.Properties.Stem != "" {
		stem = binary.Properties.Stem
	}

	return stem + binary.Properties.Suffix
}

func (binary *binaryLinker) deps(ctx BaseModuleContext, deps Deps) Deps {
	deps = binary.baseLinker.deps(ctx, deps)
	if ctx.Device() {
		if !ctx.sdk() {
			if Bool(binary.Properties.Static_executable) {
				deps.CrtBegin = "crtbegin_static"
			} else {
				deps.CrtBegin = "crtbegin_dynamic"
			}
			deps.CrtEnd = "crtend_android"
		} else {
			if Bool(binary.Properties.Static_executable) {
				deps.CrtBegin = "ndk_crtbegin_static." + ctx.sdkVersion()
			} else {
				deps.CrtBegin = "ndk_crtbegin_dynamic." + ctx.sdkVersion()
			}
			deps.CrtEnd = "ndk_crtend_android." + ctx.sdkVersion()
		}

		if Bool(binary.Properties.Static_executable) {
			if inList("libc++_static", deps.StaticLibs) {
				deps.StaticLibs = append(deps.StaticLibs, "libm", "libc", "libdl")
			}
			// static libraries libcompiler_rt, libc and libc_nomalloc need to be linked with
			// --start-group/--end-group along with libgcc.  If they are in deps.StaticLibs,
			// move them to the beginning of deps.LateStaticLibs
			var groupLibs []string
			deps.StaticLibs, groupLibs = filterList(deps.StaticLibs,
				[]string{"libc", "libc_nomalloc", "libcompiler_rt"})
			deps.LateStaticLibs = append(groupLibs, deps.LateStaticLibs...)
		}
	}

	if !Bool(binary.Properties.Static_executable) && inList("libc", deps.StaticLibs) {
		ctx.ModuleErrorf("statically linking libc to dynamic executable, please remove libc\n" +
			"from static libs or set static_executable: true")
	}
	return deps
}

func NewBinary(hod common.HostOrDeviceSupported) *Module {
	module := newModule(hod, common.MultilibFirst)
	module.compiler = &baseCompiler{}
	module.linker = &binaryLinker{}
	module.installer = &baseInstaller{
		dir: "bin",
	}
	return module
}

func binaryFactory() (blueprint.Module, []interface{}) {
	module := NewBinary(common.HostAndDeviceSupported)
	return module.Init()
}

func (binary *binaryLinker) ModifyProperties(ctx ModuleContext) {
	if ctx.Darwin() {
		binary.Properties.Static_executable = proptools.BoolPtr(false)
	}
	if Bool(binary.Properties.Static_executable) {
		binary.dynamicProperties.VariantIsStaticBinary = true
	}
}

func (binary *binaryLinker) flags(ctx ModuleContext, flags Flags) Flags {
	flags = binary.baseLinker.flags(ctx, flags)

	if ctx.Host() {
		flags.LdFlags = append(flags.LdFlags, "-pie")
		if ctx.HostType() == common.Windows {
			flags.LdFlags = append(flags.LdFlags, "-Wl,-e_mainCRTStartup")
		}
	}

	// MinGW spits out warnings about -fPIC even for -fpie?!) being ignored because
	// all code is position independent, and then those warnings get promoted to
	// errors.
	if ctx.HostType() != common.Windows {
		flags.CFlags = append(flags.CFlags, "-fpie")
	}

	if ctx.Device() {
		if Bool(binary.Properties.Static_executable) {
			// Clang driver needs -static to create static executable.
			// However, bionic/linker uses -shared to overwrite.
			// Linker for x86 targets does not allow coexistance of -static and -shared,
			// so we add -static only if -shared is not used.
			if !inList("-shared", flags.LdFlags) {
				flags.LdFlags = append(flags.LdFlags, "-static")
			}

			flags.LdFlags = append(flags.LdFlags,
				"-nostdlib",
				"-Bstatic",
				"-Wl,--gc-sections",
			)

		} else {
			linker := "/system/bin/linker"
			if flags.Toolchain.Is64Bit() {
				linker += "64"
			}

			flags.LdFlags = append(flags.LdFlags,
				"-pie",
				"-nostdlib",
				"-Bdynamic",
				fmt.Sprintf("-Wl,-dynamic-linker,%s", linker),
				"-Wl,--gc-sections",
				"-Wl,-z,nocopyreloc",
			)
		}
	} else if ctx.Darwin() {
		flags.LdFlags = append(flags.LdFlags, "-Wl,-headerpad_max_install_names")
	}

	return flags
}

func (binary *binaryLinker) link(ctx ModuleContext,
	flags Flags, deps PathDeps, objFiles common.Paths) common.Path {

	outputFile := common.PathForModuleOut(ctx, binary.getStem(ctx)+flags.Toolchain.ExecutableSuffix())
	if ctx.HostOrDevice().Host() {
		binary.hostToolPath = common.OptionalPathForPath(outputFile)
	}
	ret := outputFile

	if binary.Properties.Prefix_symbols != "" {
		afterPrefixSymbols := outputFile
		outputFile = common.PathForModuleOut(ctx, binary.getStem(ctx)+".intermediate")
		TransformBinaryPrefixSymbols(ctx, binary.Properties.Prefix_symbols, outputFile,
			flagsToBuilderFlags(flags), afterPrefixSymbols)
	}

	var linkerDeps common.Paths

	sharedLibs := deps.SharedLibs
	sharedLibs = append(sharedLibs, deps.LateSharedLibs...)

	TransformObjToDynamicBinary(ctx, objFiles, sharedLibs, deps.StaticLibs,
		deps.LateStaticLibs, deps.WholeStaticLibs, linkerDeps, deps.CrtBegin, deps.CrtEnd, true,
		flagsToBuilderFlags(flags), outputFile)

	return ret
}

func (binary *binaryLinker) HostToolPath() common.OptionalPath {
	return binary.hostToolPath
}

func testPerSrcMutator(mctx common.AndroidBottomUpMutatorContext) {
	if m, ok := mctx.Module().(*Module); ok {
		if test, ok := m.linker.(*testLinker); ok {
			if Bool(test.Properties.Test_per_src) {
				testNames := make([]string, len(m.compiler.(*baseCompiler).Properties.Srcs))
				for i, src := range m.compiler.(*baseCompiler).Properties.Srcs {
					testNames[i] = strings.TrimSuffix(filepath.Base(src), filepath.Ext(src))
				}
				tests := mctx.CreateLocalVariations(testNames...)
				for i, src := range m.compiler.(*baseCompiler).Properties.Srcs {
					tests[i].(*Module).compiler.(*baseCompiler).Properties.Srcs = []string{src}
					tests[i].(*Module).linker.(*testLinker).binaryLinker.Properties.Stem = testNames[i]
				}
			}
		}
	}
}

type testLinker struct {
	binaryLinker
	Properties TestLinkerProperties
}

func (test *testLinker) props() []interface{} {
	return append(test.binaryLinker.props(), &test.Properties)
}

func (test *testLinker) flags(ctx ModuleContext, flags Flags) Flags {
	flags = test.binaryLinker.flags(ctx, flags)

	if !test.Properties.Gtest {
		return flags
	}

	flags.CFlags = append(flags.CFlags, "-DGTEST_HAS_STD_STRING")
	if ctx.Host() {
		flags.CFlags = append(flags.CFlags, "-O0", "-g")

		if ctx.HostType() == common.Windows {
			flags.CFlags = append(flags.CFlags, "-DGTEST_OS_WINDOWS")
		} else {
			flags.CFlags = append(flags.CFlags, "-DGTEST_OS_LINUX")
			flags.LdFlags = append(flags.LdFlags, "-lpthread")
		}
	} else {
		flags.CFlags = append(flags.CFlags, "-DGTEST_OS_LINUX_ANDROID")
	}

	// TODO(danalbert): Make gtest export its dependencies.
	flags.CFlags = append(flags.CFlags,
		"-I"+common.PathForSource(ctx, "external/gtest/include").String())

	return flags
}

func (test *testLinker) deps(ctx BaseModuleContext, deps Deps) Deps {
	if test.Properties.Gtest {
		deps.StaticLibs = append(deps.StaticLibs, "libgtest_main", "libgtest")
	}
	deps = test.binaryLinker.deps(ctx, deps)
	return deps
}

type testInstaller struct {
	baseInstaller
}

func (installer *testInstaller) install(ctx ModuleContext, file common.Path) {
	installer.dir = filepath.Join(installer.dir, ctx.ModuleName())
	installer.dir64 = filepath.Join(installer.dir64, ctx.ModuleName())
	installer.baseInstaller.install(ctx, file)
}

func NewTest(hod common.HostOrDeviceSupported) *Module {
	module := newModule(hod, common.MultilibBoth)
	module.compiler = &baseCompiler{}
	linker := &testLinker{}
	linker.Properties.Gtest = true
	module.linker = linker
	module.installer = &testInstaller{
		baseInstaller: baseInstaller{
			dir:   "nativetest",
			dir64: "nativetest64",
			data:  true,
		},
	}
	return module
}

func testFactory() (blueprint.Module, []interface{}) {
	module := NewTest(common.HostAndDeviceSupported)
	return module.Init()
}

type benchmarkLinker struct {
	binaryLinker
}

func (benchmark *benchmarkLinker) deps(ctx BaseModuleContext, deps Deps) Deps {
	deps = benchmark.binaryLinker.deps(ctx, deps)
	deps.StaticLibs = append(deps.StaticLibs, "libbenchmark", "libbase")
	return deps
}

func NewBenchmark(hod common.HostOrDeviceSupported) *Module {
	module := newModule(hod, common.MultilibFirst)
	module.compiler = &baseCompiler{}
	module.linker = &benchmarkLinker{}
	module.installer = &baseInstaller{
		dir:   "nativetest",
		dir64: "nativetest64",
		data:  true,
	}
	return module
}

func benchmarkFactory() (blueprint.Module, []interface{}) {
	module := NewBenchmark(common.HostAndDeviceSupported)
	return module.Init()
}

//
// Static library
//

func libraryStaticFactory() (blueprint.Module, []interface{}) {
	module := NewLibrary(common.HostAndDeviceSupported, false, true)
	return module.Init()
}

//
// Shared libraries
//

func librarySharedFactory() (blueprint.Module, []interface{}) {
	module := NewLibrary(common.HostAndDeviceSupported, true, false)
	return module.Init()
}

//
// Host static library
//

func libraryHostStaticFactory() (blueprint.Module, []interface{}) {
	module := NewLibrary(common.HostSupported, false, true)
	return module.Init()
}

//
// Host Shared libraries
//

func libraryHostSharedFactory() (blueprint.Module, []interface{}) {
	module := NewLibrary(common.HostSupported, true, false)
	return module.Init()
}

//
// Host Binaries
//

func binaryHostFactory() (blueprint.Module, []interface{}) {
	module := NewBinary(common.HostSupported)
	return module.Init()
}

//
// Host Tests
//

func testHostFactory() (blueprint.Module, []interface{}) {
	module := NewTest(common.HostSupported)
	return module.Init()
}

//
// Host Benchmarks
//

func benchmarkHostFactory() (blueprint.Module, []interface{}) {
	module := NewBenchmark(common.HostSupported)
	return module.Init()
}

//
// Defaults
//
type Defaults struct {
	common.AndroidModuleBase
	common.DefaultsModule
}

func (*Defaults) GenerateAndroidBuildActions(ctx common.AndroidModuleContext) {
}

func defaultsFactory() (blueprint.Module, []interface{}) {
	module := &Defaults{}

	propertyStructs := []interface{}{
		&BaseProperties{},
		&BaseCompilerProperties{},
		&BaseLinkerProperties{},
		&LibraryCompilerProperties{},
		&LibraryLinkerProperties{},
		&BinaryLinkerProperties{},
		&TestLinkerProperties{},
		&UnusedProperties{},
		&StlProperties{},
	}

	_, propertyStructs = common.InitAndroidArchModule(module, common.HostAndDeviceDefault,
		common.MultilibDefault, propertyStructs...)

	return common.InitDefaultsModule(module, module, propertyStructs...)
}

//
// Device libraries shipped with gcc
//

type toolchainLibraryLinker struct {
	baseLinker
}

var _ baseLinkerInterface = (*toolchainLibraryLinker)(nil)

func (*toolchainLibraryLinker) deps(ctx BaseModuleContext, deps Deps) Deps {
	// toolchain libraries can't have any dependencies
	return deps
}

func (*toolchainLibraryLinker) buildStatic() bool {
	return true
}

func (*toolchainLibraryLinker) buildShared() bool {
	return false
}

func toolchainLibraryFactory() (blueprint.Module, []interface{}) {
	module := newBaseModule(common.DeviceSupported, common.MultilibBoth)
	module.compiler = &baseCompiler{}
	module.linker = &toolchainLibraryLinker{}
	module.Properties.Clang = proptools.BoolPtr(false)
	return module.Init()
}

func (library *toolchainLibraryLinker) link(ctx ModuleContext,
	flags Flags, deps PathDeps, objFiles common.Paths) common.Path {

	libName := ctx.ModuleName() + staticLibraryExtension
	outputFile := common.PathForModuleOut(ctx, libName)

	if flags.Clang {
		ctx.ModuleErrorf("toolchain_library must use GCC, not Clang")
	}

	CopyGccLib(ctx, libName, flagsToBuilderFlags(flags), outputFile)

	ctx.CheckbuildFile(outputFile)

	return outputFile
}

// NDK prebuilt libraries.
//
// These differ from regular prebuilts in that they aren't stripped and usually aren't installed
// either (with the exception of the shared STLs, which are installed to the app's directory rather
// than to the system image).

func getNdkLibDir(ctx common.AndroidModuleContext, toolchain Toolchain, version string) common.SourcePath {
	return common.PathForSource(ctx, fmt.Sprintf("prebuilts/ndk/current/platforms/android-%s/arch-%s/usr/lib",
		version, toolchain.Name()))
}

func ndkPrebuiltModuleToPath(ctx common.AndroidModuleContext, toolchain Toolchain,
	ext string, version string) common.Path {

	// NDK prebuilts are named like: ndk_NAME.EXT.SDK_VERSION.
	// We want to translate to just NAME.EXT
	name := strings.Split(strings.TrimPrefix(ctx.ModuleName(), "ndk_"), ".")[0]
	dir := getNdkLibDir(ctx, toolchain, version)
	return dir.Join(ctx, name+ext)
}

type ndkPrebuiltObjectLinker struct {
	objectLinker
}

func (*ndkPrebuiltObjectLinker) deps(ctx BaseModuleContext, deps Deps) Deps {
	// NDK objects can't have any dependencies
	return deps
}

func ndkPrebuiltObjectFactory() (blueprint.Module, []interface{}) {
	module := newBaseModule(common.DeviceSupported, common.MultilibBoth)
	module.linker = &ndkPrebuiltObjectLinker{}
	return module.Init()
}

func (c *ndkPrebuiltObjectLinker) link(ctx ModuleContext, flags Flags,
	deps PathDeps, objFiles common.Paths) common.Path {
	// A null build step, but it sets up the output path.
	if !strings.HasPrefix(ctx.ModuleName(), "ndk_crt") {
		ctx.ModuleErrorf("NDK prebuilts must have an ndk_crt prefixed name")
	}

	return ndkPrebuiltModuleToPath(ctx, flags.Toolchain, objectExtension, ctx.sdkVersion())
}

type ndkPrebuiltLibraryLinker struct {
	libraryLinker
	Properties struct {
		Export_include_dirs []string `android:"arch_variant"`
	}
}

var _ baseLinkerInterface = (*ndkPrebuiltLibraryLinker)(nil)
var _ exportedFlagsProducer = (*libraryLinker)(nil)

func (ndk *ndkPrebuiltLibraryLinker) props() []interface{} {
	return []interface{}{&ndk.Properties}
}

func (*ndkPrebuiltLibraryLinker) deps(ctx BaseModuleContext, deps Deps) Deps {
	// NDK libraries can't have any dependencies
	return deps
}

func ndkPrebuiltLibraryFactory() (blueprint.Module, []interface{}) {
	module := newBaseModule(common.DeviceSupported, common.MultilibBoth)
	linker := &ndkPrebuiltLibraryLinker{}
	linker.dynamicProperties.BuildShared = true
	module.linker = linker
	return module.Init()
}

func (ndk *ndkPrebuiltLibraryLinker) link(ctx ModuleContext, flags Flags,
	deps PathDeps, objFiles common.Paths) common.Path {
	// A null build step, but it sets up the output path.
	if !strings.HasPrefix(ctx.ModuleName(), "ndk_lib") {
		ctx.ModuleErrorf("NDK prebuilts must have an ndk_lib prefixed name")
	}

	includeDirs := common.PathsForModuleSrc(ctx, ndk.Properties.Export_include_dirs)
	ndk.exportFlags = []string{common.JoinWithPrefix(includeDirs.Strings(), "-isystem ")}

	return ndkPrebuiltModuleToPath(ctx, flags.Toolchain, flags.Toolchain.ShlibSuffix(),
		ctx.sdkVersion())
}

// The NDK STLs are slightly different from the prebuilt system libraries:
//     * Are not specific to each platform version.
//     * The libraries are not in a predictable location for each STL.

type ndkPrebuiltStlLinker struct {
	ndkPrebuiltLibraryLinker
}

func ndkPrebuiltSharedStlFactory() (blueprint.Module, []interface{}) {
	module := newBaseModule(common.DeviceSupported, common.MultilibBoth)
	linker := &ndkPrebuiltStlLinker{}
	linker.dynamicProperties.BuildShared = true
	module.linker = linker
	return module.Init()
}

func ndkPrebuiltStaticStlFactory() (blueprint.Module, []interface{}) {
	module := newBaseModule(common.DeviceSupported, common.MultilibBoth)
	linker := &ndkPrebuiltStlLinker{}
	linker.dynamicProperties.BuildStatic = true
	module.linker = linker
	return module.Init()
}

func getNdkStlLibDir(ctx common.AndroidModuleContext, toolchain Toolchain, stl string) common.SourcePath {
	gccVersion := toolchain.GccVersion()
	var libDir string
	switch stl {
	case "libstlport":
		libDir = "cxx-stl/stlport/libs"
	case "libc++":
		libDir = "cxx-stl/llvm-libc++/libs"
	case "libgnustl":
		libDir = fmt.Sprintf("cxx-stl/gnu-libstdc++/%s/libs", gccVersion)
	}

	if libDir != "" {
		ndkSrcRoot := "prebuilts/ndk/current/sources"
		return common.PathForSource(ctx, ndkSrcRoot).Join(ctx, libDir, ctx.Arch().Abi[0])
	}

	ctx.ModuleErrorf("Unknown NDK STL: %s", stl)
	return common.PathForSource(ctx, "")
}

func (ndk *ndkPrebuiltStlLinker) link(ctx ModuleContext, flags Flags,
	deps PathDeps, objFiles common.Paths) common.Path {
	// A null build step, but it sets up the output path.
	if !strings.HasPrefix(ctx.ModuleName(), "ndk_lib") {
		ctx.ModuleErrorf("NDK prebuilts must have an ndk_lib prefixed name")
	}

	includeDirs := common.PathsForModuleSrc(ctx, ndk.Properties.Export_include_dirs)
	ndk.exportFlags = []string{includeDirsToFlags(includeDirs)}

	libName := strings.TrimPrefix(ctx.ModuleName(), "ndk_")
	libExt := flags.Toolchain.ShlibSuffix()
	if ndk.dynamicProperties.BuildStatic {
		libExt = staticLibraryExtension
	}

	stlName := strings.TrimSuffix(libName, "_shared")
	stlName = strings.TrimSuffix(stlName, "_static")
	libDir := getNdkStlLibDir(ctx, flags.Toolchain, stlName)
	return libDir.Join(ctx, libName+libExt)
}

func linkageMutator(mctx common.AndroidBottomUpMutatorContext) {
	if m, ok := mctx.Module().(*Module); ok {
		if m.linker != nil {
			if linker, ok := m.linker.(baseLinkerInterface); ok {
				var modules []blueprint.Module
				if linker.buildStatic() && linker.buildShared() {
					modules = mctx.CreateLocalVariations("static", "shared")
					modules[0].(*Module).linker.(baseLinkerInterface).setStatic(true)
					modules[0].(*Module).installer = nil
					modules[1].(*Module).linker.(baseLinkerInterface).setStatic(false)
				} else if linker.buildStatic() {
					modules = mctx.CreateLocalVariations("static")
					modules[0].(*Module).linker.(baseLinkerInterface).setStatic(true)
					modules[0].(*Module).installer = nil
				} else if linker.buildShared() {
					modules = mctx.CreateLocalVariations("shared")
					modules[0].(*Module).linker.(baseLinkerInterface).setStatic(false)
				} else {
					panic(fmt.Errorf("library %q not static or shared", mctx.ModuleName()))
				}

				if _, ok := m.compiler.(*libraryCompiler); ok {
					reuseFrom := modules[0].(*Module).compiler.(*libraryCompiler)
					for _, m := range modules {
						m.(*Module).compiler.(*libraryCompiler).reuseFrom = reuseFrom
					}
				}
			}
		}
	}
}

// lastUniqueElements returns all unique elements of a slice, keeping the last copy of each
// modifies the slice contents in place, and returns a subslice of the original slice
func lastUniqueElements(list []string) []string {
	totalSkip := 0
	for i := len(list) - 1; i >= totalSkip; i-- {
		skip := 0
		for j := i - 1; j >= totalSkip; j-- {
			if list[i] == list[j] {
				skip++
			} else {
				list[j+skip] = list[j]
			}
		}
		totalSkip += skip
	}
	return list[totalSkip:]
}

var Bool = proptools.Bool

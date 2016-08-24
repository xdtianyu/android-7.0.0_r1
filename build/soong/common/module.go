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
	"fmt"
	"path/filepath"
	"strings"

	"android/soong"
	"android/soong/glob"

	"github.com/google/blueprint"
)

var (
	DeviceSharedLibrary = "shared_library"
	DeviceStaticLibrary = "static_library"
	DeviceExecutable    = "executable"
	HostSharedLibrary   = "host_shared_library"
	HostStaticLibrary   = "host_static_library"
	HostExecutable      = "host_executable"
)

type ModuleBuildParams struct {
	Rule      blueprint.Rule
	Output    WritablePath
	Outputs   WritablePaths
	Input     Path
	Inputs    Paths
	Implicit  Path
	Implicits Paths
	OrderOnly Paths
	Default   bool
	Args      map[string]string
}

type androidBaseContext interface {
	Arch() Arch
	HostOrDevice() HostOrDevice
	HostType() HostType
	Host() bool
	Device() bool
	Darwin() bool
	Debug() bool
	AConfig() Config
	Proprietary() bool
	InstallInData() bool
}

type AndroidBaseContext interface {
	blueprint.BaseModuleContext
	androidBaseContext
}

type AndroidModuleContext interface {
	blueprint.ModuleContext
	androidBaseContext

	// Similar to Build, but takes Paths instead of []string,
	// and performs more verification.
	ModuleBuild(pctx blueprint.PackageContext, params ModuleBuildParams)

	ExpandSources(srcFiles, excludes []string) Paths
	Glob(outDir, globPattern string, excludes []string) Paths

	InstallFile(installPath OutputPath, srcPath Path, deps ...Path) OutputPath
	InstallFileName(installPath OutputPath, name string, srcPath Path, deps ...Path) OutputPath
	CheckbuildFile(srcPath Path)

	AddMissingDependencies(deps []string)
}

type AndroidModule interface {
	blueprint.Module

	GenerateAndroidBuildActions(AndroidModuleContext)

	base() *AndroidModuleBase
	Enabled() bool
	HostOrDevice() HostOrDevice
	InstallInData() bool
}

type commonProperties struct {
	Name string
	Deps []string
	Tags []string

	// emit build rules for this module
	Enabled *bool `android:"arch_variant"`

	// control whether this module compiles for 32-bit, 64-bit, or both.  Possible values
	// are "32" (compile for 32-bit only), "64" (compile for 64-bit only), "both" (compile for both
	// architectures), or "first" (compile for 64-bit on a 64-bit platform, and 32-bit on a 32-bit
	// platform
	Compile_multilib string

	// whether this is a proprietary vendor module, and should be installed into /vendor
	Proprietary bool

	// Set by HostOrDeviceMutator
	CompileHostOrDevice HostOrDevice `blueprint:"mutated"`

	// Set by HostTypeMutator
	CompileHostType HostType `blueprint:"mutated"`

	// Set by ArchMutator
	CompileArch Arch `blueprint:"mutated"`

	// Set by InitAndroidModule
	HostOrDeviceSupported HostOrDeviceSupported `blueprint:"mutated"`
}

type hostAndDeviceProperties struct {
	Host_supported   bool
	Device_supported bool
}

type Multilib string

const (
	MultilibBoth    Multilib = "both"
	MultilibFirst   Multilib = "first"
	MultilibCommon  Multilib = "common"
	MultilibDefault Multilib = ""
)

func InitAndroidModule(m AndroidModule,
	propertyStructs ...interface{}) (blueprint.Module, []interface{}) {

	base := m.base()
	base.module = m

	propertyStructs = append(propertyStructs, &base.commonProperties, &base.variableProperties)

	return m, propertyStructs
}

func InitAndroidArchModule(m AndroidModule, hod HostOrDeviceSupported, defaultMultilib Multilib,
	propertyStructs ...interface{}) (blueprint.Module, []interface{}) {

	_, propertyStructs = InitAndroidModule(m, propertyStructs...)

	base := m.base()
	base.commonProperties.HostOrDeviceSupported = hod
	base.commonProperties.Compile_multilib = string(defaultMultilib)

	switch hod {
	case HostAndDeviceSupported:
		// Default to module to device supported, host not supported, can override in module
		// properties
		base.hostAndDeviceProperties.Device_supported = true
		fallthrough
	case HostAndDeviceDefault:
		propertyStructs = append(propertyStructs, &base.hostAndDeviceProperties)
	}

	return InitArchModule(m, propertyStructs...)
}

// A AndroidModuleBase object contains the properties that are common to all Android
// modules.  It should be included as an anonymous field in every module
// struct definition.  InitAndroidModule should then be called from the module's
// factory function, and the return values from InitAndroidModule should be
// returned from the factory function.
//
// The AndroidModuleBase type is responsible for implementing the
// GenerateBuildActions method to support the blueprint.Module interface. This
// method will then call the module's GenerateAndroidBuildActions method once
// for each build variant that is to be built. GenerateAndroidBuildActions is
// passed a AndroidModuleContext rather than the usual blueprint.ModuleContext.
// AndroidModuleContext exposes extra functionality specific to the Android build
// system including details about the particular build variant that is to be
// generated.
//
// For example:
//
//     import (
//         "android/soong/common"
//         "github.com/google/blueprint"
//     )
//
//     type myModule struct {
//         common.AndroidModuleBase
//         properties struct {
//             MyProperty string
//         }
//     }
//
//     func NewMyModule() (blueprint.Module, []interface{}) {
//         m := &myModule{}
//         return common.InitAndroidModule(m, &m.properties)
//     }
//
//     func (m *myModule) GenerateAndroidBuildActions(ctx common.AndroidModuleContext) {
//         // Get the CPU architecture for the current build variant.
//         variantArch := ctx.Arch()
//
//         // ...
//     }
type AndroidModuleBase struct {
	// Putting the curiously recurring thing pointing to the thing that contains
	// the thing pattern to good use.
	module AndroidModule

	commonProperties        commonProperties
	variableProperties      variableProperties
	hostAndDeviceProperties hostAndDeviceProperties
	generalProperties       []interface{}
	archProperties          []*archProperties

	noAddressSanitizer bool
	installFiles       Paths
	checkbuildFiles    Paths

	// Used by buildTargetSingleton to create checkbuild and per-directory build targets
	// Only set on the final variant of each module
	installTarget    string
	checkbuildTarget string
	blueprintDir     string
}

func (a *AndroidModuleBase) base() *AndroidModuleBase {
	return a
}

func (a *AndroidModuleBase) SetHostOrDevice(hod HostOrDevice) {
	a.commonProperties.CompileHostOrDevice = hod
}

func (a *AndroidModuleBase) SetHostType(ht HostType) {
	a.commonProperties.CompileHostType = ht
}

func (a *AndroidModuleBase) SetArch(arch Arch) {
	a.commonProperties.CompileArch = arch
}

func (a *AndroidModuleBase) HostOrDevice() HostOrDevice {
	return a.commonProperties.CompileHostOrDevice
}

func (a *AndroidModuleBase) HostType() HostType {
	return a.commonProperties.CompileHostType
}

func (a *AndroidModuleBase) Host() bool {
	return a.HostOrDevice().Host()
}

func (a *AndroidModuleBase) Arch() Arch {
	return a.commonProperties.CompileArch
}

func (a *AndroidModuleBase) HostSupported() bool {
	return a.commonProperties.HostOrDeviceSupported == HostSupported ||
		a.commonProperties.HostOrDeviceSupported == HostAndDeviceSupported &&
			a.hostAndDeviceProperties.Host_supported
}

func (a *AndroidModuleBase) DeviceSupported() bool {
	return a.commonProperties.HostOrDeviceSupported == DeviceSupported ||
		a.commonProperties.HostOrDeviceSupported == HostAndDeviceSupported &&
			a.hostAndDeviceProperties.Device_supported
}

func (a *AndroidModuleBase) Enabled() bool {
	if a.commonProperties.Enabled == nil {
		if a.HostSupported() && a.HostOrDevice().Host() && a.HostType() == Windows {
			return false
		} else {
			return true
		}
	}
	return *a.commonProperties.Enabled
}

func (a *AndroidModuleBase) computeInstallDeps(
	ctx blueprint.ModuleContext) Paths {

	result := Paths{}
	ctx.VisitDepsDepthFirstIf(isFileInstaller,
		func(m blueprint.Module) {
			fileInstaller := m.(fileInstaller)
			files := fileInstaller.filesToInstall()
			result = append(result, files...)
		})

	return result
}

func (a *AndroidModuleBase) filesToInstall() Paths {
	return a.installFiles
}

func (p *AndroidModuleBase) NoAddressSanitizer() bool {
	return p.noAddressSanitizer
}

func (p *AndroidModuleBase) InstallInData() bool {
	return false
}

func (a *AndroidModuleBase) generateModuleTarget(ctx blueprint.ModuleContext) {
	if a != ctx.FinalModule().(AndroidModule).base() {
		return
	}

	allInstalledFiles := Paths{}
	allCheckbuildFiles := Paths{}
	ctx.VisitAllModuleVariants(func(module blueprint.Module) {
		a := module.(AndroidModule).base()
		allInstalledFiles = append(allInstalledFiles, a.installFiles...)
		allCheckbuildFiles = append(allCheckbuildFiles, a.checkbuildFiles...)
	})

	deps := []string{}

	if len(allInstalledFiles) > 0 {
		name := ctx.ModuleName() + "-install"
		ctx.Build(pctx, blueprint.BuildParams{
			Rule:      blueprint.Phony,
			Outputs:   []string{name},
			Implicits: allInstalledFiles.Strings(),
			Optional:  ctx.Config().(Config).EmbeddedInMake(),
		})
		deps = append(deps, name)
		a.installTarget = name
	}

	if len(allCheckbuildFiles) > 0 {
		name := ctx.ModuleName() + "-checkbuild"
		ctx.Build(pctx, blueprint.BuildParams{
			Rule:      blueprint.Phony,
			Outputs:   []string{name},
			Implicits: allCheckbuildFiles.Strings(),
			Optional:  true,
		})
		deps = append(deps, name)
		a.checkbuildTarget = name
	}

	if len(deps) > 0 {
		suffix := ""
		if ctx.Config().(Config).EmbeddedInMake() {
			suffix = "-soong"
		}

		ctx.Build(pctx, blueprint.BuildParams{
			Rule:      blueprint.Phony,
			Outputs:   []string{ctx.ModuleName() + suffix},
			Implicits: deps,
			Optional:  true,
		})

		a.blueprintDir = ctx.ModuleDir()
	}
}

func (a *AndroidModuleBase) androidBaseContextFactory(ctx blueprint.BaseModuleContext) androidBaseContextImpl {
	return androidBaseContextImpl{
		arch:          a.commonProperties.CompileArch,
		hod:           a.commonProperties.CompileHostOrDevice,
		ht:            a.commonProperties.CompileHostType,
		proprietary:   a.commonProperties.Proprietary,
		config:        ctx.Config().(Config),
		installInData: a.module.InstallInData(),
	}
}

func (a *AndroidModuleBase) GenerateBuildActions(ctx blueprint.ModuleContext) {
	androidCtx := &androidModuleContext{
		ModuleContext:          ctx,
		androidBaseContextImpl: a.androidBaseContextFactory(ctx),
		installDeps:            a.computeInstallDeps(ctx),
		installFiles:           a.installFiles,
		missingDeps:            ctx.GetMissingDependencies(),
	}

	if !a.Enabled() {
		return
	}

	a.module.GenerateAndroidBuildActions(androidCtx)
	if ctx.Failed() {
		return
	}

	a.installFiles = append(a.installFiles, androidCtx.installFiles...)
	a.checkbuildFiles = append(a.checkbuildFiles, androidCtx.checkbuildFiles...)

	a.generateModuleTarget(ctx)
	if ctx.Failed() {
		return
	}
}

type androidBaseContextImpl struct {
	arch          Arch
	hod           HostOrDevice
	ht            HostType
	debug         bool
	config        Config
	proprietary   bool
	installInData bool
}

type androidModuleContext struct {
	blueprint.ModuleContext
	androidBaseContextImpl
	installDeps     Paths
	installFiles    Paths
	checkbuildFiles Paths
	missingDeps     []string
}

func (a *androidModuleContext) ninjaError(outputs []string, err error) {
	a.ModuleContext.Build(pctx, blueprint.BuildParams{
		Rule:     ErrorRule,
		Outputs:  outputs,
		Optional: true,
		Args: map[string]string{
			"error": err.Error(),
		},
	})
	return
}

func (a *androidModuleContext) Build(pctx blueprint.PackageContext, params blueprint.BuildParams) {
	if a.missingDeps != nil {
		a.ninjaError(params.Outputs, fmt.Errorf("module %s missing dependencies: %s\n",
			a.ModuleName(), strings.Join(a.missingDeps, ", ")))
		return
	}

	params.Optional = true
	a.ModuleContext.Build(pctx, params)
}

func (a *androidModuleContext) ModuleBuild(pctx blueprint.PackageContext, params ModuleBuildParams) {
	bparams := blueprint.BuildParams{
		Rule:      params.Rule,
		Outputs:   params.Outputs.Strings(),
		Inputs:    params.Inputs.Strings(),
		Implicits: params.Implicits.Strings(),
		OrderOnly: params.OrderOnly.Strings(),
		Args:      params.Args,
		Optional:  !params.Default,
	}

	if params.Output != nil {
		bparams.Outputs = append(bparams.Outputs, params.Output.String())
	}
	if params.Input != nil {
		bparams.Inputs = append(bparams.Inputs, params.Input.String())
	}
	if params.Implicit != nil {
		bparams.Implicits = append(bparams.Implicits, params.Implicit.String())
	}

	if a.missingDeps != nil {
		a.ninjaError(bparams.Outputs, fmt.Errorf("module %s missing dependencies: %s\n",
			a.ModuleName(), strings.Join(a.missingDeps, ", ")))
		return
	}

	a.ModuleContext.Build(pctx, bparams)
}

func (a *androidModuleContext) GetMissingDependencies() []string {
	return a.missingDeps
}

func (a *androidModuleContext) AddMissingDependencies(deps []string) {
	if deps != nil {
		a.missingDeps = append(a.missingDeps, deps...)
	}
}

func (a *androidBaseContextImpl) Arch() Arch {
	return a.arch
}

func (a *androidBaseContextImpl) HostOrDevice() HostOrDevice {
	return a.hod
}

func (a *androidBaseContextImpl) HostType() HostType {
	return a.ht
}

func (a *androidBaseContextImpl) Host() bool {
	return a.hod.Host()
}

func (a *androidBaseContextImpl) Device() bool {
	return a.hod.Device()
}

func (a *androidBaseContextImpl) Darwin() bool {
	return a.hod.Host() && a.ht == Darwin
}

func (a *androidBaseContextImpl) Debug() bool {
	return a.debug
}

func (a *androidBaseContextImpl) AConfig() Config {
	return a.config
}

func (a *androidBaseContextImpl) Proprietary() bool {
	return a.proprietary
}

func (a *androidBaseContextImpl) InstallInData() bool {
	return a.installInData
}

func (a *androidModuleContext) InstallFileName(installPath OutputPath, name string, srcPath Path,
	deps ...Path) OutputPath {

	fullInstallPath := installPath.Join(a, name)

	if a.Host() || !a.AConfig().SkipDeviceInstall() {
		deps = append(deps, a.installDeps...)

		a.ModuleBuild(pctx, ModuleBuildParams{
			Rule:      Cp,
			Output:    fullInstallPath,
			Input:     srcPath,
			OrderOnly: Paths(deps),
			Default:   !a.AConfig().EmbeddedInMake(),
		})

		a.installFiles = append(a.installFiles, fullInstallPath)
	}
	a.checkbuildFiles = append(a.checkbuildFiles, srcPath)
	return fullInstallPath
}

func (a *androidModuleContext) InstallFile(installPath OutputPath, srcPath Path, deps ...Path) OutputPath {
	return a.InstallFileName(installPath, filepath.Base(srcPath.String()), srcPath, deps...)
}

func (a *androidModuleContext) CheckbuildFile(srcPath Path) {
	a.checkbuildFiles = append(a.checkbuildFiles, srcPath)
}

type fileInstaller interface {
	filesToInstall() Paths
}

func isFileInstaller(m blueprint.Module) bool {
	_, ok := m.(fileInstaller)
	return ok
}

func isAndroidModule(m blueprint.Module) bool {
	_, ok := m.(AndroidModule)
	return ok
}

func findStringInSlice(str string, slice []string) int {
	for i, s := range slice {
		if s == str {
			return i
		}
	}
	return -1
}

func (ctx *androidModuleContext) ExpandSources(srcFiles, excludes []string) Paths {
	prefix := PathForModuleSrc(ctx).String()
	for i, e := range excludes {
		j := findStringInSlice(e, srcFiles)
		if j != -1 {
			srcFiles = append(srcFiles[:j], srcFiles[j+1:]...)
		}

		excludes[i] = filepath.Join(prefix, e)
	}

	globbedSrcFiles := make(Paths, 0, len(srcFiles))
	for _, s := range srcFiles {
		if glob.IsGlob(s) {
			globbedSrcFiles = append(globbedSrcFiles, ctx.Glob("src_glob", filepath.Join(prefix, s), excludes)...)
		} else {
			globbedSrcFiles = append(globbedSrcFiles, PathForModuleSrc(ctx, s))
		}
	}

	return globbedSrcFiles
}

func (ctx *androidModuleContext) Glob(outDir, globPattern string, excludes []string) Paths {
	ret, err := Glob(ctx, PathForModuleOut(ctx, outDir).String(), globPattern, excludes)
	if err != nil {
		ctx.ModuleErrorf("glob: %s", err.Error())
	}
	return pathsForModuleSrcFromFullPath(ctx, ret)
}

func init() {
	soong.RegisterSingletonType("buildtarget", BuildTargetSingleton)
}

func BuildTargetSingleton() blueprint.Singleton {
	return &buildTargetSingleton{}
}

type buildTargetSingleton struct{}

func (c *buildTargetSingleton) GenerateBuildActions(ctx blueprint.SingletonContext) {
	checkbuildDeps := []string{}

	dirModules := make(map[string][]string)

	ctx.VisitAllModules(func(module blueprint.Module) {
		if a, ok := module.(AndroidModule); ok {
			blueprintDir := a.base().blueprintDir
			installTarget := a.base().installTarget
			checkbuildTarget := a.base().checkbuildTarget

			if checkbuildTarget != "" {
				checkbuildDeps = append(checkbuildDeps, checkbuildTarget)
				dirModules[blueprintDir] = append(dirModules[blueprintDir], checkbuildTarget)
			}

			if installTarget != "" {
				dirModules[blueprintDir] = append(dirModules[blueprintDir], installTarget)
			}
		}
	})

	suffix := ""
	if ctx.Config().(Config).EmbeddedInMake() {
		suffix = "-soong"
	}

	// Create a top-level checkbuild target that depends on all modules
	ctx.Build(pctx, blueprint.BuildParams{
		Rule:      blueprint.Phony,
		Outputs:   []string{"checkbuild" + suffix},
		Implicits: checkbuildDeps,
		Optional:  true,
	})

	// Create a mm/<directory> target that depends on all modules in a directory
	dirs := sortedKeys(dirModules)
	for _, dir := range dirs {
		ctx.Build(pctx, blueprint.BuildParams{
			Rule:      blueprint.Phony,
			Outputs:   []string{filepath.Join("mm", dir)},
			Implicits: dirModules[dir],
			// HACK: checkbuild should be an optional build, but force it
			// enabled for now in standalone builds
			Optional: ctx.Config().(Config).EmbeddedInMake(),
		})
	}
}

type AndroidModulesByName struct {
	slice []AndroidModule
	ctx   interface {
		ModuleName(blueprint.Module) string
		ModuleSubDir(blueprint.Module) string
	}
}

func (s AndroidModulesByName) Len() int { return len(s.slice) }
func (s AndroidModulesByName) Less(i, j int) bool {
	mi, mj := s.slice[i], s.slice[j]
	ni, nj := s.ctx.ModuleName(mi), s.ctx.ModuleName(mj)

	if ni != nj {
		return ni < nj
	} else {
		return s.ctx.ModuleSubDir(mi) < s.ctx.ModuleSubDir(mj)
	}
}
func (s AndroidModulesByName) Swap(i, j int) { s.slice[i], s.slice[j] = s.slice[j], s.slice[i] }

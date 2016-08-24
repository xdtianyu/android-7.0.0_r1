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
	"reflect"
	"runtime"
	"strings"

	"github.com/google/blueprint"
	"github.com/google/blueprint/proptools"
)

func init() {
	RegisterBottomUpMutator("defaults_deps", defaultsDepsMutator)
	RegisterTopDownMutator("defaults", defaultsMutator)

	RegisterBottomUpMutator("host_or_device", HostOrDeviceMutator)
	RegisterBottomUpMutator("host_type", HostTypeMutator)
	RegisterBottomUpMutator("arch", ArchMutator)
}

var (
	Arm    = newArch("arm", "lib32")
	Arm64  = newArch("arm64", "lib64")
	Mips   = newArch("mips", "lib32")
	Mips64 = newArch("mips64", "lib64")
	X86    = newArch("x86", "lib32")
	X86_64 = newArch("x86_64", "lib64")

	Common = ArchType{
		Name: "common",
	}
)

var archTypeMap = map[string]ArchType{
	"arm":    Arm,
	"arm64":  Arm64,
	"mips":   Mips,
	"mips64": Mips64,
	"x86":    X86,
	"x86_64": X86_64,
}

/*
Example blueprints file containing all variant property groups, with comment listing what type
of variants get properties in that group:

module {
    arch: {
        arm: {
            // Host or device variants with arm architecture
        },
        arm64: {
            // Host or device variants with arm64 architecture
        },
        mips: {
            // Host or device variants with mips architecture
        },
        mips64: {
            // Host or device variants with mips64 architecture
        },
        x86: {
            // Host or device variants with x86 architecture
        },
        x86_64: {
            // Host or device variants with x86_64 architecture
        },
    },
    multilib: {
        lib32: {
            // Host or device variants for 32-bit architectures
        },
        lib64: {
            // Host or device variants for 64-bit architectures
        },
    },
    target: {
        android: {
            // Device variants
        },
        host: {
            // Host variants
        },
        linux: {
            // Linux host variants
        },
        darwin: {
            // Darwin host variants
        },
        windows: {
            // Windows host variants
        },
        not_windows: {
            // Non-windows host variants
        },
    },
}
*/

type Embed interface{}

type archProperties struct {
	// Properties to vary by target architecture
	Arch struct {
		// Properties for module variants being built to run on arm (host or device)
		Arm struct {
			Embed `blueprint:"filter(android:\"arch_variant\")"`

			// Arm arch variants
			Armv5te      interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Armv7_a      interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Armv7_a_neon interface{} `blueprint:"filter(android:\"arch_variant\")"`

			// Arm cpu variants
			Cortex_a7      interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Cortex_a8      interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Cortex_a9      interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Cortex_a15     interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Cortex_a53     interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Cortex_a53_a57 interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Krait          interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Denver         interface{} `blueprint:"filter(android:\"arch_variant\")"`
		}

		// Properties for module variants being built to run on arm64 (host or device)
		Arm64 struct {
			Embed `blueprint:"filter(android:\"arch_variant\")"`

			// Arm64 arch variants
			Armv8_a interface{} `blueprint:"filter(android:\"arch_variant\")"`

			// Arm64 cpu variants
			Cortex_a53 interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Denver64   interface{} `blueprint:"filter(android:\"arch_variant\")"`
		}

		// Properties for module variants being built to run on mips (host or device)
		Mips struct {
			Embed `blueprint:"filter(android:\"arch_variant\")"`

			// Mips arch variants
			Mips32_fp          interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Mips32r2_fp        interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Mips32r2_fp_xburst interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Mips32r2dsp_fp     interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Mips32r2dspr2_fp   interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Mips32r6           interface{} `blueprint:"filter(android:\"arch_variant\")"`

			// Mips arch features
			Rev6 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		}

		// Properties for module variants being built to run on mips64 (host or device)
		Mips64 struct {
			Embed `blueprint:"filter(android:\"arch_variant\")"`

			// Mips64 arch variants
			Mips64r2 interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Mips64r6 interface{} `blueprint:"filter(android:\"arch_variant\")"`

			// Mips64 arch features
			Rev6 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		}

		// Properties for module variants being built to run on x86 (host or device)
		X86 struct {
			Embed `blueprint:"filter(android:\"arch_variant\")"`

			// X86 arch variants
			Atom        interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Haswell     interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Ivybridge   interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Sandybridge interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Silvermont  interface{} `blueprint:"filter(android:\"arch_variant\")"`

			// X86 arch features
			Ssse3  interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Sse4   interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Sse4_1 interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Sse4_2 interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Aes_ni interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Avx    interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Popcnt interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Movbe  interface{} `blueprint:"filter(android:\"arch_variant\")"`
		}

		// Properties for module variants being built to run on x86_64 (host or device)
		X86_64 struct {
			Embed `blueprint:"filter(android:\"arch_variant\")"`

			// X86 arch variants
			Haswell     interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Ivybridge   interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Sandybridge interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Silvermont  interface{} `blueprint:"filter(android:\"arch_variant\")"`

			// X86 arch features
			Ssse3  interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Sse4   interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Sse4_1 interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Sse4_2 interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Aes_ni interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Avx    interface{} `blueprint:"filter(android:\"arch_variant\")"`
			Popcnt interface{} `blueprint:"filter(android:\"arch_variant\")"`
		}
	}

	// Properties to vary by 32-bit or 64-bit
	Multilib struct {
		// Properties for module variants being built to run on 32-bit devices
		Lib32 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on 64-bit devices
		Lib64 interface{} `blueprint:"filter(android:\"arch_variant\")"`
	}
	// Properties to vary by build target (host or device, os, os+archictecture)
	Target struct {
		// Properties for module variants being built to run on the host
		Host interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on the device
		Android interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on arm devices
		Android_arm interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on arm64 devices
		Android_arm64 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on mips devices
		Android_mips interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on mips64 devices
		Android_mips64 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on x86 devices
		Android_x86 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on x86_64 devices
		Android_x86_64 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on devices that support 64-bit
		Android64 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on devices that do not support 64-bit
		Android32 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on linux hosts
		Linux interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on linux x86 hosts
		Linux_x86 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on linux x86_64 hosts
		Linux_x86_64 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on darwin hosts
		Darwin interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on darwin x86 hosts
		Darwin_x86 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on darwin x86_64 hosts
		Darwin_x86_64 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on windows hosts
		Windows interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on windows x86 hosts
		Windows_x86 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on windows x86_64 hosts
		Windows_x86_64 interface{} `blueprint:"filter(android:\"arch_variant\")"`
		// Properties for module variants being built to run on linux or darwin hosts
		Not_windows interface{} `blueprint:"filter(android:\"arch_variant\")"`
	}
}

var archFeatureMap = map[ArchType]map[string][]string{}

func RegisterArchFeatures(arch ArchType, variant string, features ...string) {
	archField := proptools.FieldNameForProperty(arch.Name)
	variantField := proptools.FieldNameForProperty(variant)
	archStruct := reflect.ValueOf(archProperties{}.Arch).FieldByName(archField)
	if variant != "" {
		if !archStruct.FieldByName(variantField).IsValid() {
			panic(fmt.Errorf("Invalid variant %q for arch %q", variant, arch))
		}
	}
	for _, feature := range features {
		field := proptools.FieldNameForProperty(feature)
		if !archStruct.FieldByName(field).IsValid() {
			panic(fmt.Errorf("Invalid feature %q for arch %q variant %q", feature, arch, variant))
		}
	}
	if archFeatureMap[arch] == nil {
		archFeatureMap[arch] = make(map[string][]string)
	}
	archFeatureMap[arch][variant] = features
}

// An Arch indicates a single CPU architecture.
type Arch struct {
	ArchType     ArchType
	ArchVariant  string
	CpuVariant   string
	Abi          []string
	ArchFeatures []string
}

func (a Arch) String() string {
	s := a.ArchType.String()
	if a.ArchVariant != "" {
		s += "_" + a.ArchVariant
	}
	if a.CpuVariant != "" {
		s += "_" + a.CpuVariant
	}
	return s
}

type ArchType struct {
	Name     string
	Multilib string
}

func newArch(name, multilib string) ArchType {
	return ArchType{
		Name:     name,
		Multilib: multilib,
	}
}

func (a ArchType) String() string {
	return a.Name
}

type HostOrDeviceSupported int

const (
	_ HostOrDeviceSupported = iota
	HostSupported
	DeviceSupported
	HostAndDeviceSupported
	HostAndDeviceDefault
)

type HostOrDevice int

const (
	_ HostOrDevice = iota
	Host
	Device
)

func (hod HostOrDevice) String() string {
	switch hod {
	case Device:
		return "device"
	case Host:
		return "host"
	default:
		panic(fmt.Sprintf("unexpected HostOrDevice value %d", hod))
	}
}

func (hod HostOrDevice) Property() string {
	switch hod {
	case Device:
		return "android"
	case Host:
		return "host"
	default:
		panic(fmt.Sprintf("unexpected HostOrDevice value %d", hod))
	}
}

func (hod HostOrDevice) Host() bool {
	if hod == 0 {
		panic("HostOrDevice unset")
	}
	return hod == Host
}

func (hod HostOrDevice) Device() bool {
	if hod == 0 {
		panic("HostOrDevice unset")
	}
	return hod == Device
}

var hostOrDeviceName = map[HostOrDevice]string{
	Device: "device",
	Host:   "host",
}

type HostType int

const (
	NoHostType HostType = iota
	Linux
	Darwin
	Windows
)

func CurrentHostType() HostType {
	switch runtime.GOOS {
	case "linux":
		return Linux
	case "darwin":
		return Darwin
	default:
		panic(fmt.Sprintf("unsupported OS: %s", runtime.GOOS))
	}
}

func (ht HostType) String() string {
	switch ht {
	case Linux:
		return "linux"
	case Darwin:
		return "darwin"
	case Windows:
		return "windows"
	default:
		panic(fmt.Sprintf("unexpected HostType value %d", ht))
	}
}

func (ht HostType) Field() string {
	switch ht {
	case Linux:
		return "Linux"
	case Darwin:
		return "Darwin"
	case Windows:
		return "Windows"
	default:
		panic(fmt.Sprintf("unexpected HostType value %d", ht))
	}
}

var (
	commonArch = Arch{
		ArchType: Common,
	}
)

func HostOrDeviceMutator(mctx AndroidBottomUpMutatorContext) {
	var module AndroidModule
	var ok bool
	if module, ok = mctx.Module().(AndroidModule); !ok {
		return
	}

	hods := []HostOrDevice{}

	if module.base().HostSupported() {
		hods = append(hods, Host)
	}

	if module.base().DeviceSupported() {
		hods = append(hods, Device)
	}

	if len(hods) == 0 {
		return
	}

	hodNames := []string{}
	for _, hod := range hods {
		hodNames = append(hodNames, hod.String())
	}

	modules := mctx.CreateVariations(hodNames...)
	for i, m := range modules {
		m.(AndroidModule).base().SetHostOrDevice(hods[i])
	}
}

func HostTypeMutator(mctx AndroidBottomUpMutatorContext) {
	var module AndroidModule
	var ok bool
	if module, ok = mctx.Module().(AndroidModule); !ok {
		return
	}

	if !module.base().HostSupported() || !module.base().HostOrDevice().Host() {
		return
	}

	buildTypes, err := decodeHostTypesProductVariables(mctx.Config().(Config).ProductVariables)
	if err != nil {
		mctx.ModuleErrorf("%s", err.Error())
		return
	}

	typeNames := []string{}
	for _, ht := range buildTypes {
		typeNames = append(typeNames, ht.String())
	}

	modules := mctx.CreateVariations(typeNames...)
	for i, m := range modules {
		m.(AndroidModule).base().SetHostType(buildTypes[i])
	}
}

func ArchMutator(mctx AndroidBottomUpMutatorContext) {
	var module AndroidModule
	var ok bool
	if module, ok = mctx.Module().(AndroidModule); !ok {
		return
	}

	moduleArches := []Arch{}
	multilib := module.base().commonProperties.Compile_multilib

	if module.base().HostSupported() && module.base().HostOrDevice().Host() {
		hostModuleArches, err := decodeMultilib(multilib, mctx.Config().(Config).HostArches[module.base().HostType()])
		if err != nil {
			mctx.ModuleErrorf("%s", err.Error())
		}

		moduleArches = append(moduleArches, hostModuleArches...)
	}

	if module.base().DeviceSupported() && module.base().HostOrDevice().Device() {
		deviceModuleArches, err := decodeMultilib(multilib, mctx.Config().(Config).DeviceArches)
		if err != nil {
			mctx.ModuleErrorf("%s", err.Error())
		}

		moduleArches = append(moduleArches, deviceModuleArches...)
	}

	if len(moduleArches) == 0 {
		return
	}

	archNames := []string{}
	for _, arch := range moduleArches {
		archNames = append(archNames, arch.String())
	}

	modules := mctx.CreateVariations(archNames...)

	for i, m := range modules {
		m.(AndroidModule).base().SetArch(moduleArches[i])
		m.(AndroidModule).base().setArchProperties(mctx)
	}
}

func InitArchModule(m AndroidModule,
	propertyStructs ...interface{}) (blueprint.Module, []interface{}) {

	base := m.base()

	base.generalProperties = append(base.generalProperties,
		propertyStructs...)

	for _, properties := range base.generalProperties {
		propertiesValue := reflect.ValueOf(properties)
		if propertiesValue.Kind() != reflect.Ptr {
			panic(fmt.Errorf("properties must be a pointer to a struct, got %T",
				propertiesValue.Interface()))
		}

		propertiesValue = propertiesValue.Elem()
		if propertiesValue.Kind() != reflect.Struct {
			panic(fmt.Errorf("properties must be a pointer to a struct, got %T",
				propertiesValue.Interface()))
		}

		archProperties := &archProperties{}
		forEachInterface(reflect.ValueOf(archProperties), func(v reflect.Value) {
			newValue := proptools.CloneEmptyProperties(propertiesValue)
			v.Set(newValue)
		})

		base.archProperties = append(base.archProperties, archProperties)
	}

	var allProperties []interface{}
	allProperties = append(allProperties, base.generalProperties...)
	for _, asp := range base.archProperties {
		allProperties = append(allProperties, asp)
	}

	return m, allProperties
}

var variantReplacer = strings.NewReplacer("-", "_", ".", "_")

func (a *AndroidModuleBase) appendProperties(ctx AndroidBottomUpMutatorContext,
	dst, src interface{}, field, srcPrefix string) interface{} {

	srcField := reflect.ValueOf(src).FieldByName(field)
	if !srcField.IsValid() {
		ctx.ModuleErrorf("field %q does not exist", srcPrefix)
		return nil
	}

	ret := srcField

	if srcField.Kind() == reflect.Struct {
		srcField = srcField.FieldByName("Embed")
	}

	src = srcField.Elem().Interface()

	filter := func(property string,
		dstField, srcField reflect.StructField,
		dstValue, srcValue interface{}) (bool, error) {

		srcProperty := srcPrefix + "." + property

		if !proptools.HasTag(dstField, "android", "arch_variant") {
			if ctx.ContainsProperty(srcProperty) {
				return false, fmt.Errorf("can't be specific to a build variant")
			} else {
				return false, nil
			}
		}

		return true, nil
	}

	err := proptools.AppendProperties(dst, src, filter)
	if err != nil {
		if propertyErr, ok := err.(*proptools.ExtendPropertyError); ok {
			ctx.PropertyErrorf(propertyErr.Property, "%s", propertyErr.Err.Error())
		} else {
			panic(err)
		}
	}

	return ret.Interface()
}

// Rewrite the module's properties structs to contain arch-specific values.
func (a *AndroidModuleBase) setArchProperties(ctx AndroidBottomUpMutatorContext) {
	arch := a.commonProperties.CompileArch
	hod := a.commonProperties.CompileHostOrDevice
	ht := a.commonProperties.CompileHostType

	if arch.ArchType == Common {
		return
	}

	for i := range a.generalProperties {
		genProps := a.generalProperties[i]
		archProps := a.archProperties[i]
		// Handle arch-specific properties in the form:
		// arch: {
		//     arm64: {
		//         key: value,
		//     },
		// },
		t := arch.ArchType

		field := proptools.FieldNameForProperty(t.Name)
		prefix := "arch." + t.Name
		archStruct := a.appendProperties(ctx, genProps, archProps.Arch, field, prefix)

		// Handle arch-variant-specific properties in the form:
		// arch: {
		//     variant: {
		//         key: value,
		//     },
		// },
		v := variantReplacer.Replace(arch.ArchVariant)
		if v != "" {
			field := proptools.FieldNameForProperty(v)
			prefix := "arch." + t.Name + "." + v
			a.appendProperties(ctx, genProps, archStruct, field, prefix)
		}

		// Handle cpu-variant-specific properties in the form:
		// arch: {
		//     variant: {
		//         key: value,
		//     },
		// },
		c := variantReplacer.Replace(arch.CpuVariant)
		if c != "" {
			field := proptools.FieldNameForProperty(c)
			prefix := "arch." + t.Name + "." + c
			a.appendProperties(ctx, genProps, archStruct, field, prefix)
		}

		// Handle arch-feature-specific properties in the form:
		// arch: {
		//     feature: {
		//         key: value,
		//     },
		// },
		for _, feature := range arch.ArchFeatures {
			field := proptools.FieldNameForProperty(feature)
			prefix := "arch." + t.Name + "." + feature
			a.appendProperties(ctx, genProps, archStruct, field, prefix)
		}

		// Handle multilib-specific properties in the form:
		// multilib: {
		//     lib32: {
		//         key: value,
		//     },
		// },
		field = proptools.FieldNameForProperty(t.Multilib)
		prefix = "multilib." + t.Multilib
		a.appendProperties(ctx, genProps, archProps.Multilib, field, prefix)

		// Handle host-or-device-specific properties in the form:
		// target: {
		//     host: {
		//         key: value,
		//     },
		// },
		hodProperty := hod.Property()
		field = proptools.FieldNameForProperty(hodProperty)
		prefix = "target." + hodProperty
		a.appendProperties(ctx, genProps, archProps.Target, field, prefix)

		// Handle host target properties in the form:
		// target: {
		//     linux: {
		//         key: value,
		//     },
		//     not_windows: {
		//         key: value,
		//     },
		//     linux_x86: {
		//         key: value,
		//     },
		//     linux_arm: {
		//         key: value,
		//     },
		// },
		if hod.Host() {
			field := ht.Field()
			prefix := "target." + ht.String()
			a.appendProperties(ctx, genProps, archProps.Target, field, prefix)

			t := arch.ArchType
			field = ht.Field() + "_" + t.Name
			prefix = "target." + ht.String() + "_" + t.Name
			a.appendProperties(ctx, genProps, archProps.Target, field, prefix)

			if ht != Windows {
				field := "Not_windows"
				prefix := "target.not_windows"
				a.appendProperties(ctx, genProps, archProps.Target, field, prefix)
			}
		}

		// Handle 64-bit device properties in the form:
		// target {
		//     android64 {
		//         key: value,
		//     },
		//     android32 {
		//         key: value,
		//     },
		// },
		// WARNING: this is probably not what you want to use in your blueprints file, it selects
		// options for all targets on a device that supports 64-bit binaries, not just the targets
		// that are being compiled for 64-bit.  Its expected use case is binaries like linker and
		// debuggerd that need to know when they are a 32-bit process running on a 64-bit device
		if hod.Device() {
			if true /* && target_is_64_bit */ {
				field := "Android64"
				prefix := "target.android64"
				a.appendProperties(ctx, genProps, archProps.Target, field, prefix)
			} else {
				field := "Android32"
				prefix := "target.android32"
				a.appendProperties(ctx, genProps, archProps.Target, field, prefix)
			}
		}

		// Handle device architecture properties in the form:
		// target {
		//     android_arm {
		//         key: value,
		//     },
		//     android_x86 {
		//         key: value,
		//     },
		// },
		if hod.Device() {
			t := arch.ArchType
			field := "Android_" + t.Name
			prefix := "target.android_" + t.Name
			a.appendProperties(ctx, genProps, archProps.Target, field, prefix)
		}

		if ctx.Failed() {
			return
		}
	}
}

func forEachInterface(v reflect.Value, f func(reflect.Value)) {
	switch v.Kind() {
	case reflect.Interface:
		f(v)
	case reflect.Struct:
		for i := 0; i < v.NumField(); i++ {
			forEachInterface(v.Field(i), f)
		}
	case reflect.Ptr:
		forEachInterface(v.Elem(), f)
	default:
		panic(fmt.Errorf("Unsupported kind %s", v.Kind()))
	}
}

// Get a list of HostTypes from the product variables
func decodeHostTypesProductVariables(variables productVariables) ([]HostType, error) {
	ret := []HostType{CurrentHostType()}

	if variables.CrossHost != nil && *variables.CrossHost != "" {
		switch *variables.CrossHost {
		case "windows":
			ret = append(ret, Windows)
		default:
			return nil, fmt.Errorf("Unsupported secondary host: %s", *variables.CrossHost)
		}
	}

	return ret, nil
}

// Convert the arch product variables into a list of host and device Arch structs
func decodeArchProductVariables(variables productVariables) (map[HostType][]Arch, []Arch, error) {
	if variables.HostArch == nil {
		return nil, nil, fmt.Errorf("No host primary architecture set")
	}

	hostArch, err := decodeArch(*variables.HostArch, nil, nil, nil)
	if err != nil {
		return nil, nil, err
	}

	hostArches := []Arch{hostArch}

	if variables.HostSecondaryArch != nil && *variables.HostSecondaryArch != "" {
		hostSecondaryArch, err := decodeArch(*variables.HostSecondaryArch, nil, nil, nil)
		if err != nil {
			return nil, nil, err
		}
		hostArches = append(hostArches, hostSecondaryArch)
	}

	hostTypeArches := map[HostType][]Arch{
		CurrentHostType(): hostArches,
	}

	if variables.CrossHost != nil && *variables.CrossHost != "" {
		if variables.CrossHostArch == nil || *variables.CrossHostArch == "" {
			return nil, nil, fmt.Errorf("No cross-host primary architecture set")
		}

		crossHostArch, err := decodeArch(*variables.CrossHostArch, nil, nil, nil)
		if err != nil {
			return nil, nil, err
		}

		crossHostArches := []Arch{crossHostArch}

		if variables.CrossHostSecondaryArch != nil && *variables.CrossHostSecondaryArch != "" {
			crossHostSecondaryArch, err := decodeArch(*variables.CrossHostSecondaryArch, nil, nil, nil)
			if err != nil {
				return nil, nil, err
			}
			crossHostArches = append(crossHostArches, crossHostSecondaryArch)
		}

		switch *variables.CrossHost {
		case "windows":
			hostTypeArches[Windows] = crossHostArches
		default:
			return nil, nil, fmt.Errorf("Unsupported cross-host: %s", *variables.CrossHost)
		}
	}

	if variables.DeviceArch == nil {
		return nil, nil, fmt.Errorf("No device primary architecture set")
	}

	deviceArch, err := decodeArch(*variables.DeviceArch, variables.DeviceArchVariant,
		variables.DeviceCpuVariant, variables.DeviceAbi)
	if err != nil {
		return nil, nil, err
	}

	deviceArches := []Arch{deviceArch}

	if variables.DeviceSecondaryArch != nil && *variables.DeviceSecondaryArch != "" {
		deviceSecondaryArch, err := decodeArch(*variables.DeviceSecondaryArch,
			variables.DeviceSecondaryArchVariant, variables.DeviceSecondaryCpuVariant,
			variables.DeviceSecondaryAbi)
		if err != nil {
			return nil, nil, err
		}
		deviceArches = append(deviceArches, deviceSecondaryArch)
	}

	return hostTypeArches, deviceArches, nil
}

func decodeMegaDevice() ([]Arch, error) {
	archSettings := []struct {
		arch        string
		archVariant string
		cpuVariant  string
		abi         []string
	}{
		{"arm", "armv7-a-neon", "cortex-a7", []string{"armeabi-v7a"}},
		{"arm", "armv7-a-neon", "cortex-a8", []string{"armeabi-v7a"}},
		// gtest_all_test.cc fails to build:
		//   error in backend: Unsupported library call operation!
		//{"arm", "armv7-a-neon", "cortex-a9", []string{"armeabi-v7a"}},
		{"arm", "armv7-a-neon", "cortex-a15", []string{"armeabi-v7a"}},
		{"arm", "armv7-a-neon", "cortex-a53", []string{"armeabi-v7a"}},
		{"arm", "armv7-a-neon", "cortex-a53.a57", []string{"armeabi-v7a"}},
		{"arm", "armv7-a-neon", "denver", []string{"armeabi-v7a"}},
		{"arm", "armv7-a-neon", "krait", []string{"armeabi-v7a"}},
		{"arm64", "", "cortex-a53", []string{"arm64-v8a"}},
		{"arm64", "", "denver64", []string{"arm64-v8a"}},
		{"mips", "mips32-fp", "", []string{"mips"}},
		{"mips", "mips32r2-fp", "", []string{"mips"}},
		{"mips", "mips32r2-fp-xburst", "", []string{"mips"}},
		{"mips", "mips32r6", "", []string{"mips32r6"}},
		// mips32r2dsp[r2]-fp fails in the assembler for divdf3.c in compiler-rt:
		// (same errors in make and soong)
		//   Error: invalid operands `mtlo $ac0,$11'
		//   Error: invalid operands `mthi $ac0,$12'
		//{"mips", "mips32r2dsp-fp", "", []string{"mips"}},
		//{"mips", "mips32r2dspr2-fp", "", []string{"mips"}},
		// mips64r2 is mismatching 64r2 and 64r6 libraries during linking to libgcc
		//{"mips64", "mips64r2", "", []string{"mips64"}},
		{"mips64", "mips64r6", "", []string{"mips64"}},
		{"x86", "", "", []string{"x86"}},
		{"x86", "atom", "", []string{"x86"}},
		{"x86", "haswell", "", []string{"x86"}},
		{"x86", "ivybridge", "", []string{"x86"}},
		{"x86", "sandybridge", "", []string{"x86"}},
		{"x86", "silvermont", "", []string{"x86"}},
		{"x86_64", "", "", []string{"x86_64"}},
		{"x86_64", "haswell", "", []string{"x86_64"}},
		{"x86_64", "ivybridge", "", []string{"x86_64"}},
		{"x86_64", "sandybridge", "", []string{"x86_64"}},
		{"x86_64", "silvermont", "", []string{"x86_64"}},
	}

	var ret []Arch

	for _, config := range archSettings {
		arch, err := decodeArch(config.arch, &config.archVariant,
			&config.cpuVariant, &config.abi)
		if err != nil {
			return nil, err
		}
		ret = append(ret, arch)
	}

	return ret, nil
}

// Convert a set of strings from product variables into a single Arch struct
func decodeArch(arch string, archVariant, cpuVariant *string, abi *[]string) (Arch, error) {
	stringPtr := func(p *string) string {
		if p != nil {
			return *p
		}
		return ""
	}

	slicePtr := func(p *[]string) []string {
		if p != nil {
			return *p
		}
		return nil
	}

	archType, ok := archTypeMap[arch]
	if !ok {
		return Arch{}, fmt.Errorf("unknown arch %q", arch)
	}

	a := Arch{
		ArchType:    archType,
		ArchVariant: stringPtr(archVariant),
		CpuVariant:  stringPtr(cpuVariant),
		Abi:         slicePtr(abi),
	}

	if a.ArchVariant == a.ArchType.Name || a.ArchVariant == "generic" {
		a.ArchVariant = ""
	}

	if a.CpuVariant == a.ArchType.Name || a.CpuVariant == "generic" {
		a.CpuVariant = ""
	}

	for i := 0; i < len(a.Abi); i++ {
		if a.Abi[i] == "" {
			a.Abi = append(a.Abi[:i], a.Abi[i+1:]...)
			i--
		}
	}

	if featureMap, ok := archFeatureMap[archType]; ok {
		a.ArchFeatures = featureMap[stringPtr(archVariant)]
	}

	return a, nil
}

// Use the module multilib setting to select one or more arches from an arch list
func decodeMultilib(multilib string, arches []Arch) ([]Arch, error) {
	buildArches := []Arch{}
	switch multilib {
	case "common":
		buildArches = append(buildArches, commonArch)
	case "both":
		buildArches = append(buildArches, arches...)
	case "first":
		buildArches = append(buildArches, arches[0])
	case "32":
		for _, a := range arches {
			if a.ArchType.Multilib == "lib32" {
				buildArches = append(buildArches, a)
			}
		}
	case "64":
		for _, a := range arches {
			if a.ArchType.Multilib == "lib64" {
				buildArches = append(buildArches, a)
			}
		}
	default:
		return nil, fmt.Errorf(`compile_multilib must be "both", "first", "32", or "64", found %q`,
			multilib)
		//buildArches = append(buildArches, arches[0])
	}

	return buildArches, nil
}

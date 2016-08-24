package main

import (
	mkparser "android/soong/androidmk/parser"
	"fmt"
	"strings"

	bpparser "github.com/google/blueprint/parser"
)

const (
	clear_vars = "__android_mk_clear_vars"
)

var standardProperties = map[string]struct {
	string
	bpparser.ValueType
}{
	// String properties
	"LOCAL_MODULE":               {"name", bpparser.String},
	"LOCAL_MODULE_CLASS":         {"class", bpparser.String},
	"LOCAL_CXX_STL":              {"stl", bpparser.String},
	"LOCAL_STRIP_MODULE":         {"strip", bpparser.String},
	"LOCAL_MULTILIB":             {"compile_multilib", bpparser.String},
	"LOCAL_ARM_MODE_HACK":        {"instruction_set", bpparser.String},
	"LOCAL_SDK_VERSION":          {"sdk_version", bpparser.String},
	"LOCAL_NDK_STL_VARIANT":      {"stl", bpparser.String},
	"LOCAL_JAR_MANIFEST":         {"manifest", bpparser.String},
	"LOCAL_JARJAR_RULES":         {"jarjar_rules", bpparser.String},
	"LOCAL_CERTIFICATE":          {"certificate", bpparser.String},
	"LOCAL_PACKAGE_NAME":         {"name", bpparser.String},
	"LOCAL_MODULE_RELATIVE_PATH": {"relative_install_path", bpparser.String},

	// List properties
	"LOCAL_SRC_FILES":               {"srcs", bpparser.List},
	"LOCAL_SHARED_LIBRARIES":        {"shared_libs", bpparser.List},
	"LOCAL_STATIC_LIBRARIES":        {"static_libs", bpparser.List},
	"LOCAL_WHOLE_STATIC_LIBRARIES":  {"whole_static_libs", bpparser.List},
	"LOCAL_SYSTEM_SHARED_LIBRARIES": {"system_shared_libs", bpparser.List},
	"LOCAL_ASFLAGS":                 {"asflags", bpparser.List},
	"LOCAL_CLANG_ASFLAGS":           {"clang_asflags", bpparser.List},
	"LOCAL_CFLAGS":                  {"cflags", bpparser.List},
	"LOCAL_CONLYFLAGS":              {"conlyflags", bpparser.List},
	"LOCAL_CPPFLAGS":                {"cppflags", bpparser.List},
	"LOCAL_LDFLAGS":                 {"ldflags", bpparser.List},
	"LOCAL_REQUIRED_MODULES":        {"required", bpparser.List},
	"LOCAL_MODULE_TAGS":             {"tags", bpparser.List},
	"LOCAL_LDLIBS":                  {"host_ldlibs", bpparser.List},
	"LOCAL_CLANG_CFLAGS":            {"clang_cflags", bpparser.List},
	"LOCAL_YACCFLAGS":               {"yaccflags", bpparser.List},
	"LOCAL_SANITIZE":                {"sanitize", bpparser.List},
	"LOCAL_SANITIZE_RECOVER":        {"sanitize_recover", bpparser.List},

	"LOCAL_JAVA_RESOURCE_DIRS":    {"java_resource_dirs", bpparser.List},
	"LOCAL_JAVACFLAGS":            {"javacflags", bpparser.List},
	"LOCAL_DX_FLAGS":              {"dxflags", bpparser.List},
	"LOCAL_JAVA_LIBRARIES":        {"java_libs", bpparser.List},
	"LOCAL_STATIC_JAVA_LIBRARIES": {"java_static_libs", bpparser.List},
	"LOCAL_AIDL_INCLUDES":         {"aidl_includes", bpparser.List},
	"LOCAL_AAPT_FLAGS":            {"aaptflags", bpparser.List},
	"LOCAL_PACKAGE_SPLITS":        {"package_splits", bpparser.List},

	// Bool properties
	"LOCAL_IS_HOST_MODULE":          {"host", bpparser.Bool},
	"LOCAL_CLANG":                   {"clang", bpparser.Bool},
	"LOCAL_FORCE_STATIC_EXECUTABLE": {"static", bpparser.Bool},
	"LOCAL_NATIVE_COVERAGE":         {"native_coverage", bpparser.Bool},
	"LOCAL_NO_CRT":                  {"nocrt", bpparser.Bool},
	"LOCAL_ALLOW_UNDEFINED_SYMBOLS": {"allow_undefined_symbols", bpparser.Bool},
	"LOCAL_RTTI_FLAG":               {"rtti", bpparser.Bool},

	"LOCAL_NO_STANDARD_LIBRARIES": {"no_standard_libraries", bpparser.Bool},

	"LOCAL_EXPORT_PACKAGE_RESOURCES": {"export_package_resources", bpparser.Bool},
}

var rewriteProperties = map[string]struct {
	f func(file *bpFile, prefix string, value *mkparser.MakeString, append bool) error
}{
	"LOCAL_C_INCLUDES":            {localIncludeDirs},
	"LOCAL_EXPORT_C_INCLUDE_DIRS": {exportIncludeDirs},
	"LOCAL_MODULE_STEM":           {stem},
	"LOCAL_MODULE_HOST_OS":        {hostOs},
}

func localAbsPath(value bpparser.Value) (*bpparser.Value, error) {
	if value.Type != bpparser.String {
		return nil, fmt.Errorf("isLocalAbsPath expected a string, got %d", value.Type)
	}

	if value.Expression == nil {
		if value.Variable == "LOCAL_PATH" {
			return &bpparser.Value{
				Type:        bpparser.String,
				StringValue: ".",
			}, nil
		}
		return nil, nil
	}

	if value.Expression.Operator != '+' {
		return nil, nil
	}

	firstOperand := value.Expression.Args[0]
	secondOperand := value.Expression.Args[1]
	if firstOperand.Type != bpparser.String {
		return nil, nil
	}

	if firstOperand.Expression != nil {
		return nil, nil
	}

	if firstOperand.Variable != "LOCAL_PATH" {
		return nil, nil
	}

	if secondOperand.Expression == nil && secondOperand.Variable == "" {
		if strings.HasPrefix(secondOperand.StringValue, "/") {
			secondOperand.StringValue = secondOperand.StringValue[1:]
		}
	}
	return &secondOperand, nil
}

func emptyList(value *bpparser.Value) bool {
	return value.Type == bpparser.List && value.Expression == nil && value.Variable == "" &&
		len(value.ListValue) == 0
}

func splitLocalGlobal(file *bpFile, val *bpparser.Value) (local, global *bpparser.Value, err error) {
	local = &bpparser.Value{
		Type: bpparser.List,
	}
	global = &bpparser.Value{
		Type: bpparser.List,
	}

	if val.Expression != nil {
		localA, globalA, err := splitLocalGlobal(file, &val.Expression.Args[0])
		if err != nil {
			return nil, nil, err
		}

		localB, globalB, err := splitLocalGlobal(file, &val.Expression.Args[1])
		if err != nil {
			return nil, nil, err
		}

		if emptyList(localA) {
			local = localB
		} else if emptyList(localB) {
			local = localA
		} else {
			localExpression := *val.Expression
			local.Expression = &localExpression
			local.Expression.Args = [2]bpparser.Value{*localA, *localB}
		}

		if emptyList(globalA) {
			global = globalB
		} else if emptyList(globalB) {
			global = globalA
		} else {
			globalExpression := *val.Expression
			global.Expression = &globalExpression
			global.Expression.Args = [2]bpparser.Value{*globalA, *globalB}
		}
	} else if val.Variable != "" {
		if val.Variable == "LOCAL_PATH" {
			local.ListValue = append(local.ListValue, bpparser.Value{
				Type:        bpparser.String,
				StringValue: ".",
			})
		} else {
			global.Variable = val.Variable
		}
	} else {
		for _, v := range val.ListValue {
			localPath, err := localAbsPath(v)
			if err != nil {
				return nil, nil, err
			}
			if localPath != nil {
				local.ListValue = append(local.ListValue, *localPath)
			} else {
				global.ListValue = append(global.ListValue, v)
			}
		}
	}

	return local, global, nil
}

func localIncludeDirs(file *bpFile, prefix string, value *mkparser.MakeString, appendVariable bool) error {
	val, err := makeVariableToBlueprint(file, value, bpparser.List)
	if err != nil {
		return err
	}

	local, global, err := splitLocalGlobal(file, val)
	if err != nil {
		return err
	}

	if len(global.ListValue) > 0 || global.Expression != nil || global.Variable != "" {
		err = setVariable(file, appendVariable, prefix, "include_dirs", global, true)
		if err != nil {
			return err
		}
	}

	if len(local.ListValue) > 0 || local.Expression != nil || local.Variable != "" {
		err = setVariable(file, appendVariable, prefix, "local_include_dirs", local, true)
		if err != nil {
			return err
		}
	}

	return nil
}

func exportIncludeDirs(file *bpFile, prefix string, value *mkparser.MakeString, appendVariable bool) error {
	val, err := makeVariableToBlueprint(file, value, bpparser.List)
	if err != nil {
		return err
	}

	local, global, err := splitLocalGlobal(file, val)
	if err != nil {
		return err
	}

	if len(local.ListValue) > 0 || local.Expression != nil || local.Variable != "" {
		err = setVariable(file, appendVariable, prefix, "export_include_dirs", local, true)
		if err != nil {
			return err
		}
		appendVariable = true
	}

	// Add any paths that could not be converted to local relative paths to export_include_dirs
	// anyways, they will cause an error if they don't exist and can be fixed manually.
	if len(global.ListValue) > 0 || global.Expression != nil || global.Variable != "" {
		err = setVariable(file, appendVariable, prefix, "export_include_dirs", global, true)
		if err != nil {
			return err
		}
	}

	return nil
}

func stem(file *bpFile, prefix string, value *mkparser.MakeString, appendVariable bool) error {
	val, err := makeVariableToBlueprint(file, value, bpparser.String)
	if err != nil {
		return err
	}
	varName := "stem"

	if val.Expression != nil && val.Expression.Operator == '+' &&
		val.Expression.Args[0].Variable == "LOCAL_MODULE" {
		varName = "suffix"
		val = &val.Expression.Args[1]
	}

	return setVariable(file, appendVariable, prefix, varName, val, true)
}

func hostOs(file *bpFile, prefix string, value *mkparser.MakeString, appendVariable bool) error {
	val, err := makeVariableToBlueprint(file, value, bpparser.List)
	if err != nil {
		return err
	}

	inList := func(s string) bool {
		for _, v := range val.ListValue {
			if v.StringValue == s {
				return true
			}
		}
		return false
	}

	falseValue := &bpparser.Value{
		Type:      bpparser.Bool,
		BoolValue: false,
	}

	trueValue := &bpparser.Value{
		Type:      bpparser.Bool,
		BoolValue: true,
	}

	if inList("windows") {
		err = setVariable(file, appendVariable, "target.windows", "enabled", trueValue, true)
	}

	if !inList("linux") && err == nil {
		err = setVariable(file, appendVariable, "target.linux", "enabled", falseValue, true)
	}

	if !inList("darwin") && err == nil {
		err = setVariable(file, appendVariable, "target.darwin", "enabled", falseValue, true)
	}

	return err
}

var deleteProperties = map[string]struct{}{
	"LOCAL_CPP_EXTENSION": struct{}{},
}

var propertyPrefixes = map[string]string{
	"arm":    "arch.arm",
	"arm64":  "arm.arm64",
	"mips":   "arch.mips",
	"mips64": "arch.mips64",
	"x86":    "arch.x86",
	"x86_64": "arch.x86_64",
	"32":     "multilib.lib32",
	"64":     "multilib.lib64",
}

var conditionalTranslations = map[string]map[bool]string{
	"($(HOST_OS),darwin)": {
		true:  "target.darwin",
		false: "target.not_darwin"},
	"($(HOST_OS), darwin)": {
		true:  "target.darwin",
		false: "target.not_darwin"},
	"($(HOST_OS),windows)": {
		true:  "target.windows",
		false: "target.not_windows"},
	"($(HOST_OS), windows)": {
		true:  "target.windows",
		false: "target.not_windows"},
	"($(HOST_OS),linux)": {
		true:  "target.linux",
		false: "target.not_linux"},
	"($(HOST_OS), linux)": {
		true:  "target.linux",
		false: "target.not_linux"},
	"($(BUILD_OS),darwin)": {
		true:  "target.darwin",
		false: "target.not_darwin"},
	"($(BUILD_OS), darwin)": {
		true:  "target.darwin",
		false: "target.not_darwin"},
	"($(BUILD_OS),linux)": {
		true:  "target.linux",
		false: "target.not_linux"},
	"($(BUILD_OS), linux)": {
		true:  "target.linux",
		false: "target.not_linux"},
	"USE_MINGW": {
		true:  "target.windows",
		false: "target.not_windows"},
	"(,$(TARGET_BUILD_APPS))": {
		false: "product_variables.unbundled_build",
	},
}

func mydir(args []string) string {
	return "."
}

func allJavaFilesUnder(args []string) string {
	dir := ""
	if len(args) > 0 {
		dir = strings.TrimSpace(args[0])
	}

	return fmt.Sprintf("%s/**/*.java", dir)
}

func allSubdirJavaFiles(args []string) string {
	return "**/*.java"
}

var moduleTypes = map[string]string{
	"BUILD_SHARED_LIBRARY":        "cc_library_shared",
	"BUILD_STATIC_LIBRARY":        "cc_library_static",
	"BUILD_HOST_SHARED_LIBRARY":   "cc_library_host_shared",
	"BUILD_HOST_STATIC_LIBRARY":   "cc_library_host_static",
	"BUILD_EXECUTABLE":            "cc_binary",
	"BUILD_HOST_EXECUTABLE":       "cc_binary_host",
	"BUILD_NATIVE_TEST":           "cc_test",
	"BUILD_HOST_NATIVE_TEST":      "cc_test_host",
	"BUILD_NATIVE_BENCHMARK":      "cc_benchmark",
	"BUILD_HOST_NATIVE_BENCHMARK": "cc_benchmark_host",

	"BUILD_JAVA_LIBRARY":             "java_library",
	"BUILD_STATIC_JAVA_LIBRARY":      "java_library_static",
	"BUILD_HOST_JAVA_LIBRARY":        "java_library_host",
	"BUILD_HOST_DALVIK_JAVA_LIBRARY": "java_library_host_dalvik",
	"BUILD_PACKAGE":                  "android_app",

	"BUILD_PREBUILT": "prebuilt",
}

var soongModuleTypes = map[string]bool{}

func androidScope() mkparser.Scope {
	globalScope := mkparser.NewScope(nil)
	globalScope.Set("CLEAR_VARS", clear_vars)
	globalScope.SetFunc("my-dir", mydir)
	globalScope.SetFunc("all-java-files-under", allJavaFilesUnder)
	globalScope.SetFunc("all-subdir-java-files", allSubdirJavaFiles)

	for k, v := range moduleTypes {
		globalScope.Set(k, v)
		soongModuleTypes[v] = true
	}

	return globalScope
}

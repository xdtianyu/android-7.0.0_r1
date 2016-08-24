# Code codeverage with Jack

Jack supports code coverage with `JaCoCo` (http://eclemma.org/jacoco). During the compilation,
it will instrument code using JaCoCo API. Therefore, it requires a dependency to a jacoco-agent.jar
that can be imported in the DEX output. Jack supports Jacoco v0.7.5 (see directory external/jacoco
in the Android source tree).

Besides, Jack will also produce a coverage metadata file (or description file) in a JSON format.
It will be used to generate the report so we can match coverage execution file with source code.

## Enabling code coverage with Jack

### Using the Android build system

You can enable code coverage by setting `EMMA_INSTRUMENT_STATIC=true` in your make command. The build
system will compile it with Jack by enabling code coverage and importing the `jacoco-agent.jar`
defined in external/jacoco. It will produce the metadata file in the 'intermediates' directory of
the app.

For instance, to instrument the Settings app:

    EMMA_INSTRUMENT_STATIC=true mmma -j32 packages/apps/Settings

The medatafile is located in `$ANDROID_BUILD_TOP/out/target/common/obj/APPS/Settings_intermediates/coverage.em`

Once the application is instrumented, you can install it and execute it to produce code coverage
execution file.

You can define class name filters to select which classes will be instrumented (all classes are
instrumented by default) by defining the following build variables:
* `LOCAL_JACK_COVERAGE_INCLUDE_FILTER`: a comma-separated list of class names to include
* `LOCAL_JACK_COVERAGE_EXCLUDE_FILTER`: a comma-separated list of class names to exclude
These filters will be passed on the Jack command-line (see below) only when code coverage is
enabled.

### Using Jack command-line

To enable code coverage with Jack command-line, pass the following property flags:

>  -D jack.coverage=true  
>  -D jack.coverage.metadata.file=<coverage_metadata_file_path>  
>  -D jack.coverage.jacoco.package=<jacoco_internal_package_name>

where
* `<coverage_metadata_file_path>` is the path of the file that will contain coverage information
                                to generate the report
* `<jacoco_internal_package_name>` is the name of the internal package name containing the class
                                 'Offline' in the associated jacoco-agent.jar file. This package
                                 name is different for each release of JaCoCo.
                                 (Note: this may be removed in the future)

Jack also supports include and exclude filtering based on class name:

> -D jack.coverage.jacoco.include=<includes>  
> -D jack.coverage.jacoco.exclude=<excludes>

where
* `<includes>` is a comma-separated list of fully-qualified class names to include
* `<excludes>` is a comma-separated list of fully-qualified class names to exclude

Wildcards characters '?' and '*' are accepted to replace respectively one character or multiple
characters.

## Collecting code coverage

To produce coverage execution data, instrumented classes must be executed and coverage data be
dumped in a coverage execution file. For Android instrumentation tests, the frameworks can do
so automatically:

    adb shell am instrument -w -e coverage true <package_name>/<instrumentation_class_name>

For the case of the Settings app tests:

    adb shell am instrument -w -e coverage true com.android.settings.tests/android.test.InstrumentationTestRunner

Once the tests are finished, the location of the coverage execution file should be printed in the
console.

## Generating the report

A coverage report can be generated using the `jack-jacoco-reporter.jar` file. This is a command-line
tool taking at least three inputs:

* the coverage metadata file produced by Jack
* the coverage execution file produced during execution
* an existing directory where the report is generated

It is also recommended to indicate the directories containing the source code of classes being
analyzed to link coverage information with the source code.

The command then looks like:

    java -jar jack-jacoco-reporter.jar --metadata-file <metadata_file> --coverage-file <execution_file> --report-dir <report_directory> --source-dir <source_dir_1> ... --source-dir <source_dir_N>

You can find the full command-line usage with

    java -jar jack-jacoco-reporter.jar --help


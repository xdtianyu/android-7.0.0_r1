# Contributing

DroidDriver issues are [tracked on GitHub](https://github.com/appium/droiddriver/issues)

The [`master` branch](https://github.com/appium/droiddriver/tree/master) on GitHub tracks [the AOSP master branch](https://android.googlesource.com/platform/external/droiddriver). [All releases](releasing_to_jcenter.md) are made from the master branch.

Code changes should be [submitted to AOSP](contributing_aosp.md) and then they'll be synced to GitHub once they've passed code reivew on Gerrit.

#### Requirements

Gradle 2.2.1 or better is required to be installed on the system. In Android Studio, you'll need to provide the gradle location.

On Mac OSX with homebrew, `brew install gradle` will install gradle. To locate the path, use `brew info gradle` The homebrew path follows this format: `/usr/local/Cellar/gradle/2.2.1/libexec`

If you installed gradle using the zip (`gradle-2.2.1-bin.zip`), then the path will be the `gradle-2.2.1` folder.

#### Import into Android Studio

- Clone from git
- Launch Android Studio and select `Open an existing Android Studio project`
- Navigate to `droiddriver/build.gradle` and press Choose
- Select `Use local gradle distribution` and enter the Gradle path
- Android Studio will now import the project successfully

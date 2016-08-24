Android Clang/LLVM
==================

Platform Projects
-----------------

The LLVM toolchain is primarily composed of the following projects:

* **external/clang**
* **external/compiler-rt**
* **external/llvm**

Each of these projects has three important branches:

* *aosp/master*

  This is the branch that will be present in most platform trees. As such,
  platform components that use clang or LLVM will build against the headers and
  libraries in this branch.

  This branch does not contain a full history. Updates to this are done via a
  squashed merge from the *dev* branch. Aside from updates, patches usually
  shouldn't be submitted to this branch. Any that are will need to be
  cherry-picked to *dev*.

* *aosp/dev*

  The primary purpose of this branch is allowing us to decouple the platform
  compilers from RenderScript and other platform components that depend on LLVM
  libraries. This means we can update the platform compilers even if
  RenderScript will need substantial modification for API changes.

  Updates are performed in this branch, and the platform compilers are built
  from this branch.

* *aosp/upstream-master*

  This branch is an automatically updated direct mirror of the upstream master
  branch. This is a read only branch that is the merge source for updates to
  *dev*.


Development Flow
----------------

Rebases take place in the **aosp/llvm** tree. This tree is a manifest that uses
the appropriate *dev* branches of each LLVM project and contains only the
projects needed to build the platform compilers. Updates are done by merging
from the *aosp/upstream-master* branch.

Conflicts are resolved manually and then a patch is produced to adapt
Android.mk files (as well as to add/delete any updated source files).
Prebuilts are then generated using these projects and placed into the proper
**prebuilts/clang/host** subprojects.

The prebuilt projects contain multiple versions to make it easy to check in a
new compiler that may not be ready to be enabled by default. Each new toolchain
will add the following paths:

    prebuilts/clang/host/linux-x86/clang-$BUILD_NUMBER
    prebuilts/clang/host/darwin-x86/clang-$BUILD_NUMBER
    prebuilts/clang/host/windows-x86/clang-$BUILD_NUMBER

In order to prepare for the actual rebase (including updating dependent
projects), we will copy each **external/** *aosp/dev* project to its
corresponding **external/** *aosp/master* project as a squashed single CL.
This makes rollbacks simpler, and it also reduces churn on the Android build
servers.
This also has the side effect of not spamming non-Android @google.com
committers to upstream LLVM projects, since their commits will be batched up
into a single copy commit on each tracked **external/** project.

Prebuilts for llvm-rs-cc and bcc\_compat also need to be generated for
**prebuilts/sdk**.
This is done by running **frameworks/rs/update\_rs\_prebuilts.sh** on both Mac
and Linux. This is done from the normal AOSP platform tree rather than the LLVM
tree.
After this completes, the **prebuilts/sdk** project will have a prepared
branch/CL that can be uploaded for review/commit.


Fixing Bugs
-----------

If we find a host-side bug that needs to be fixed, it may trigger an update of
the host prebuilts (i.e. rebase).
Device-side fixes can be pushed directly to **external/** *aosp/master* and then
copied to **external/** *aosp/dev* to speed up the process (assuming that it
doesn’t affect the host builds).


Looking at Upstream
-------------------

The upstream repositories are automatically mirrored to the
*aosp/upstream-master* branch. Update with `git fetch aosp upstream-master`.


Guides for Updating Toolchains
------------------------------

* [Updating platform toolchains](ToolchainPrebuilts.md)
* [Updating RenderScript prebuilts](RenderScriptPrebuilts.md)

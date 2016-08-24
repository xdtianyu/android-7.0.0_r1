Updating Platform Toolchains
============================

Updating Toolchain Source
-------------------------

The following process is done in the Android LLVM tree. To fetch the sources:

    repo init -u https://android.googlesource.com/platform/manifest -b llvm

    # Googlers, use
    repo init -u \
        persistent-https://android.git.corp.google.com/platform/manifest -b llvm

Loop over llvm, clang, compiler-rt (in this order):

1. We are working from a separate untracked/merged branch called *aosp/dev*.

        git branch -D working_dev
        repo start working_dev .

2. **OPTIONAL FIXUPS**.
   These aren't really necessary if you remember to always keep *aosp/dev* and
   *aosp/master* synchronized otherwise, but very often someone will forget to
   merge back a change.

   1. Grab the squashed commit that went into *aosp/master* and mark it
      committed to *aosp/dev* too.

      **Note**: If there were changes to *aosp/master* before the squashed
      commit, grab those changes (using step 2), before applying this step,
      and finally repeat step 2 for changes after the squashed commit.

          git branch -D clean_master
          git checkout -b clean_master <SHA_FOR_SQUASH>
          git checkout working_dev
          git merge -s ours clean_master
          git push aosp refs/heads/working_dev:refs/heads/dev
          git branch -D clean_master

   2. Grab all outstanding changes that went into *aosp/master* and put them
      into *aosp/dev* too.

          git branch -D clean_master
          git checkout -b clean_master aosp/master
          git checkout working_dev
          git merge clean_master
          git push aosp refs/heads/working_dev:refs/heads/dev
          git branch -D clean_master

3. Merge the upstream branch.
   Use `git log aosp/upsteam-master` to browse upstream commits and find a SHA.

       git merge <upstream_sha>

4. Fix conflicts.

5. Update build rules and commit that patch on top.

6. Test everything before pushing.

7. Submit your work to *aosp/dev*.

       git push aosp refs/heads/working_dev:refs/heads/dev

8. Squash your work for *aosp/master*.

       repo start update_38 .
       git merge --squash working_dev
       git commit -a
       repo upload .

9. Test everything before submitting the patch from the previous step.

10. Grab the squashed commit and replay it in *aosp/dev*.

        repo sync .
        git remote update
        git branch -D clean_master
        git checkout -b clean_master aosp/master
        git checkout working_dev

    Use `-s ours` to ensure that we skip the squashed set of changes.
    If/when we forget this, we have to do it later.

        git merge -s ours clean_master
        git push aosp refs/heads/working_dev:refs/heads/dev
        git branch -D clean_master

11. Clean up after our working branch.

        git checkout --detach
        git branch -D working_dev

This works better because we can keep full history in *aosp/dev*, while
maintaining easy reverts/commits through *aosp/master*.


Generating New Prebuilts
------------------------

1. Run the toolchain build script. This will perform a two stage build and
   create a tarball of the final toolchain.

        python external/clang/build.py

2. The just built toolchain can be tested in an existing AOSP tree by invoking
   make with:

        make \
            LLVM_PREBUILTS_VERSION=clang-dev \
            LLVM_PREBUILTS_BASE=/path/to/llvm/out/install

   This will use the just built toolchain rather than the one in **prebuilts/**.
   If you used something other than the default for `--build-name`, use
   `clang-$BUILD_NAME` instead of `clang-dev`.

3. Once the updates have been verified, upload to gerrit, review, submit. The
   build server will pick up the changes and build them. The LLVM build page is
   http://go/clang-build. Sorry, Googlers only (for now) :(

4. To update the platform compiler, download the selected package from the build
   server and extract them to the appropriate prebuilts directory. The new
   directory will be named "clang-BUILD\_NUMBER".

5. Update `LLVM\_PREBUILTS\_VERSION` in `build/core/clang/config.mk` to match
   the new prebuilt directory. We typically keep around two versions of the
   toolchain in prebuilts so we can easily switch between them in the build
   system rather than needing to revert prebuilts. This also allows developers
   that need new toolchain features to take advantage of them locally while
   validation for the new compiler is still in progress.

6. Rebuild/test everything one more time to ensure correctness.

   Make sure you check *goog/master* as well as *aosp/master*.

   There may be necessary fixups here, to handle .ll reading or other projects
   where new warnings/errors are firing.

        m -j48 checkbuild

6. Upload the changes produced in **prebuilts/clang/host**.
   This may entail more than a simple `git commit -a`, so look at `git status`
   before finally uploading/committing.

       repo start updated_toolchain .
       git add clang-BUILD_NUMBER
       git commit
       repo upload --cbr .

7. Submit CLs.


Testing Checklist
-----------------

1. Do a checkbuild.
2. Go to **external/llvm** and run `./android_test.sh` (no known failures as of
   2015-10-08).
3. Ensure successful build for all architectures: 32- and 64- bit ARM, x86 and
   Mips.
4. Run ART host tests.
   This was broken by a rebase once, and worth testing after every rebase.

       croot && cd art && mma -j40 test-art-host

5. Run ART device tests.

       croot && cd art && mma -j4 test-art-device


Checklist for CLs
-----------------

The following projects will almost always have CLs as a part of the rebase.
Depending on the changes in LLVM, there might be updates to other projects as
well.

* External projects

  * **external/clang**
  * **external/compiler-rt**
  * **external/llvm**

* Prebuilts

  * **prebuilts/clang/host/darwin-x86/**
  * **prebuilts/clang/host/linux-x86/**
  * **prebuilts/clang/host/windows-x86/**

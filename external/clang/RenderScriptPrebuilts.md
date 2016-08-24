Updating RenderScript
=====================

Updating LLVM Libraries
-----------------------

Loop over llvm, clang, compiler-rt (in this order):

1. Do a squashed merge of *aosp/dev* to *aosp/master*.

        repo start update .
        git fetch aosp dev
        git merge --squash aosp/dev
        git commit -a
        repo upload .

2. Test everything before submitting the patch from the previous step.

3. Grab the squashed commit and replay it in *aosp/dev*.

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

4. Clean up after our working branch.

        git checkout --detach
        git branch -D working_dev

This works better because we can keep full history in *aosp/dev*, while
maintaining easy reverts/commits through *aosp/master*.


Generating New Prebuilts
------------------------

1. Iteratively attempt to build the platform and fix any API differences in
   frameworks/compile/slang, and/or frameworks/compile/libbcc. This may entail
   updating the various snapshots of Bitcode Readers/Writers.
2. Update RenderScript prebuilts.

        cd $ANDROID_BUILD_TOP/frameworks/rs
        ./update_rs_prebuilts.sh

3. The prebuilts get copied to **prebuilts/sdk**, so we must upload the
relevant bits from there.

        cd $ANDROID_BUILD_TOP/prebuilts/sdk
        git commit -a
        repo upload .

4. Submit CLs.


Testing Checklist
-----------------

1. Go to **external/llvm** and run `./android_test.sh` (no known failures
as of 2015-10-08).
2. Ensure successful build for all architectures: 32- and 64- bit ARM, x86 and
Mips.
3. Run 32- and 64- bit RenderScript CTS at least for ARM and AArch64.
4. Test RenderScript apps: RsTest, ImageProcessing, and finally
RSTest\_Compatlib in compatibility mode.
5. Test old APKs with rebased tools: grab the above apps from a different tree
(i.e. without the rebase), push them to a device with the rebased tools, and
test.
This ensures that the rebased BitcodeReader can read the output of old
BitcodeWriters.
6. Test new APKs on an old device: test freshly built APKs for
RSTest\_V{11,14,16}, and ImageProcessing\_2 on an old device (say Manta) and
ensure they pass.
This ensures that the rebase did not break the 2.9 and 3.2 BitcodeWriters.


Checklist for CLs
-----------------

The following projects will almost always have CLs as a part of the rebase.
Depending on the changes in LLVM, there might be updates to other projects as
well.

* External projects

  * **external/clang**
  * **external/compiler-rt**
  * **external/llvm**
  * **frameworks/compile/mclinker**

* RenderScript projects

  * **frameworks/compile/libbcc**
  * **frameworks/compile/slang**
  * **frameworks/rs**

* Prebuilts
  * **prebuilts/sdk**

* CTS tests

  * **cts/tests/tests/renderscript**
  * **cts/tests/tests/renderscriptlegacy**
  * **cts/tests/tests/rscpp**

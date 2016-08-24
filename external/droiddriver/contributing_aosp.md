## Working on AOSP

#### Downloading source

Follow instructions at https://source.android.com/source/downloading.html except those noted below. You need to set up authentication to be able to submit changes.
DroidDriver is an "unbundled" project. If you specify the repo manifest branch "droiddriver-dev" (see below), you'll get only the relevant projects instead of the whole AOSP tree.

Create a dir for AOSP, e.g. ~/android/aosp. It should be separate from your work on the internal repo to avoid confusion.
Then get a local client of the repo:

```bash
$ mkdir droiddriver-dev
$ cd droiddriver-dev
$ repo init -u https://android.googlesource.com/a/platform/manifest -b droiddriver-dev
$ repo sync
```

The code should be downloaded to the current dir. You may see some lines in the output like:
curl: (22) The requested URL returned error: 401 Unauthorized
These messages seem non-fatal and you should see these dirs after it is done:
build/  external/  frameworks/  Makefile  prebuilts/

#### Submitting Patches

[Submitting patches to Android](https://source.android.com/source/submit-patches.html)

- `cd external/droiddriver/`
- `repo start somebranchname .`
-  make changes and commit them
- `repo upload`

After submitting a branch to gerrit for review, each commit will show up as an individual patch set on gerrit. First the code needs to be code reviewed (+2), then verified & submitted by an approver. Reviewers without approval rights are limited to adding a code review +1.

If commits are uploaded together (on the same branch) then they are considered dependent upon eachother. To submit an individual commit without requiring other commits to be merged first, that commit must be cherry picked to a new branch. This can be done either locally or via the gerrit UI.

#### Working with gerrit

Reviewers must be added to each changeset for them to approve the code. Reviews can be specified on command line in this format:

`$ repo upload --re="<joe@example.com>,<john@example.com>" .`

The '<>' above are literal. The emails must have been registered with Gerrit. You can also use the "--cc" flag.

When commenting on the code, posts will show up as drafts. Drafts are not visible to other users. To post the drafts, press the reply button (keyboard shortcut 'a') and then click Post.

#### Updating patches on Gerrit

- `repo sync`
- Use the `get fetch` command from the gerrit changeset under the Download menu
- Make new commit then squash into previous commit to retain the gerrit change id
- `repo upload`

The [`repo prune`](https://source.android.com/source/using-repo.html) command can be used to delete already merged branches.

#### Building

This sets up environment and some bash functions, particularly "tapas"
(the counterpart of "lunch" for unbundled projects) and "m".

```bash
$ . build/envsetup.sh
$ tapas droiddriver ManualDD
$ m
```

ManualDD is an APK you can use to manually test DroidDriver.

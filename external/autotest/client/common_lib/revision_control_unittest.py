#! /usr/bin/python
import logging, mox, os, shutil, tempfile, unittest, utils

# This makes autotest_lib imports available.
import common
from autotest_lib.client.common_lib import revision_control


class GitRepoManager(object):
    """
    A wrapper for GitRepo.
    """
    commit_hash = None
    commit_msg = None
    repodir = None
    git_repo_manager = None


    def __init__(self, master_repo=None):
        """
        Setup self.git_repo_manager.

        If a master_repo is present clone it.
        Otherwise create a directory in /tmp and init it.

        @param master_repo: GitRepo representing master.
        """
        if master_repo is None:
            self.repodir = tempfile.mktemp(suffix='master')
            self._create_git_repo(self.repodir)
            self.git_repo_manager = revision_control.GitRepo(
                                        self.repodir,
                                        self.repodir,
                                        abs_work_tree=self.repodir)
            self._setup_git_environment()
            # Create an initial commit. We really care about the common case
            # where there exists a commit in the upstream repo.
            self._edit('initial_commit_file', 'is_non_empty')
            self.add()
            self.commit('initial_commit')
        else:
            self.repodir = tempfile.mktemp(suffix='dependent')
            self.git_repo_manager = revision_control.GitRepo(
                                      self.repodir,
                                      master_repo.repodir,
                                      abs_work_tree=self.repodir)
            self.git_repo_manager.clone()
            self._setup_git_environment()


    def _setup_git_environment(self):
        """
        Mock out basic git environment to keep tests deterministic.
        """
        # Set user and email for the test git checkout.
        self.git_repo_manager.gitcmd('config user.name Unittests')
        self.git_repo_manager.gitcmd('config user.email utests@chromium.org')


    def _edit(self, filename='foo', msg='bar'):
        """
        Write msg into a file in the repodir.

        @param filename: Name of the file in current repo.
                If none exists one will be created.
        @param msg: A message to write into the file.
        """
        local_file_name = os.path.join(self.git_repo_manager.repodir,
                                       filename)
        with open(local_file_name, 'w') as f:
            f.write(msg)


    def _create_git_repo(self, repodir):
        """
        Init a new git repository.

        @param repodir: directory for repo.
        """
        logging.info('initializing git repo in: %s', repodir)
        gitcmd = 'git init %s' % repodir
        rv = utils.run(gitcmd)
        if rv.exit_status != 0:
            logging.error(rv.stderr)
            raise revision_control.revision_control.GitError(gitcmd + 'failed')


    def add(self):
        """
        Add all unadded files in repodir to repo.
        """
        rv = self.git_repo_manager.gitcmd('add .')
        if rv.exit_status != 0:
            logging.error(rv.stderr)
            raise revision_control.GitError('Unable to add files to repo', rv)


    def commit(self, msg='default'):
        """
        Commit changes to repo with the supplied commit msg.
        Also updates commit_hash with the hash for this commit.

        @param msg: A message that goes with the commit.
        """
        self.git_repo_manager.commit(msg)
        self.commit_hash = self.git_repo_manager.get_latest_commit_hash()


    def get_master_tot(self):
        """
        Get everything from masters TOT squashing local changes.
        If the dependent repo is empty pull from master.
        """
        self.git_repo_manager.reinit_repo_at('master')
        self.commit_hash = self.git_repo_manager.get_latest_commit_hash()


class RevisionControlUnittest(mox.MoxTestBase):
    """
    A unittest to exercise build_externals.py's usage
    of revision_control.py's Git wrappers.
    """
    master_repo=None
    dependent_repo=None

    def setUp(self):
        """
        Create a master repo and clone it into a dependent repo.
        """
        super(RevisionControlUnittest, self).setUp()
        self.master_repo = GitRepoManager()
        self.dependent_repo = GitRepoManager(self.master_repo)


    def tearDown(self):
        """
        Delete temporary directories.
        """
        shutil.rmtree(self.master_repo.repodir)
        shutil.rmtree(self.dependent_repo.repodir)
        super(RevisionControlUnittest, self).tearDown()


    def testCommit(self):
        """
        Test add, commit, pull, clone.
        """
        self.master_repo._edit()
        self.master_repo.add()
        self.master_repo.commit()
        self.dependent_repo.get_master_tot()
        self.assertEquals(self.dependent_repo.commit_hash,
            self.master_repo.commit_hash,
            msg=(("hashes don't match after clone, master and dependent repo"
                  "out of sync: %r != %r") %
                  (self.dependent_repo.commit_hash,
                   self.master_repo.commit_hash)))

        self.master_repo._edit(msg='foobar')
        self.master_repo.commit()
        self.dependent_repo.get_master_tot()
        self.assertEquals(self.dependent_repo.commit_hash,
            self.master_repo.commit_hash,
            msg=(("hashes don't match after pull, master and dependent repo"
                  "out of sync: %r != %r") %
                  (self.dependent_repo.commit_hash,
                   self.master_repo.commit_hash)))


    def testGitUrlClone(self):
        """
        Test that git clone raises a ValueError if giturl is unset.
        """
        self.dependent_repo.git_repo_manager._giturl = None
        self.assertRaises(ValueError,
                          self.dependent_repo.git_repo_manager.clone)


    def testGitUrlPull(self):
        """
        Test that git pull raises a ValueError if giturl is unset.
        """
        self.dependent_repo.git_repo_manager._giturl = None
        self.assertRaises(ValueError,
                          self.dependent_repo.git_repo_manager.pull)


    def testGitUrlFetch(self):
        """
        Test that git fetch raises a ValueError if giturl is unset.
        """
        self.dependent_repo.git_repo_manager._giturl = None
        self.assertRaises(ValueError,
                          self.dependent_repo.git_repo_manager.fetch_remote)


if __name__ == '__main__':
  unittest.main()

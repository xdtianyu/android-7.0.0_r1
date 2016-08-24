#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib
import logging
import os
import sys
import urllib2

import common
try:
    # Ensure the chromite site-package is installed.
    from chromite.lib import *
except ImportError:
    import subprocess
    build_externals_path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
            'utils', 'build_externals.py')
    subprocess.check_call([build_externals_path, 'chromiterepo'])
    # Restart the script so python now finds the autotest site-packages.
    sys.exit(os.execv(__file__, sys.argv))
from autotest_lib.server.hosts import moblab_host
from autotest_lib.site_utils import brillo_common


_DEFAULT_STAGE_PATH_TEMPLATE = 'aue2e/%(use)s'
_DEVSERVER_STAGE_URL_TEMPLATE = ('http://%(moblab)s:%(port)s/stage?'
                                 'local_path=%(stage_dir)s&'
                                 'files=%(stage_files)s')
_DEVSERVER_PAYLOAD_URI_TEMPLATE = ('http://%(moblab)s:%(port)s/static/'
                                   '%(stage_path)s')
_STAGED_PAYLOAD_FILENAME = 'update.gz'
_SPEC_GEN_LABEL = 'gen'
_TEST_JOB_NAME = 'brillo_update_test'
_TEST_NAME = 'autoupdate_EndToEndTest'
_DEFAULT_DEVSERVER_PORT = '8080'

# Snippet of code that runs on the Moblab and returns the type of a payload
# file. Result is either 'delta' or 'full', acordingly.
_GET_PAYLOAD_TYPE = """
import update_payload
p = update_payload(open('%(payload_file)s'))
p.Init()
print 'delta' if p.IsDelta() else 'full'
"""


class PayloadStagingError(brillo_common.BrilloTestError):
    """A failure that occurred while staging an update payload."""


class PayloadGenerationError(brillo_common.BrilloTestError):
    """A failure that occurred while generating an update payload."""


def setup_parser(parser):
    """Add parser options.

    @param parser: argparse.ArgumentParser of the script.
    """
    parser.add_argument('-t', '--target_payload', metavar='SPEC', required=True,
                        help='Stage a target payload. This can either be a '
                             'path to a local payload file, or take the form '
                             '"%s:DST_IMAGE[:SRC_IMAGE]", in which case a '
                             'new payload will get generated from SRC_IMAGE '
                             '(if given) and DST_IMAGE and staged on the '
                             'server. This is a mandatory input.' %
                             _SPEC_GEN_LABEL)
    parser.add_argument('-s', '--source_payload', metavar='SPEC',
                        help='Stage a source payload. This is an optional '
                             'input. See --target_payload for possible values '
                             'for SPEC.')

    brillo_common.setup_test_action_parser(parser)


def get_stage_rel_path(stage_file):
    """Returns the relative stage path for remote file.

    The relative stage path consists of the last three path components: the
    file name and the two directory levels that contain it.

    @param stage_file: Path to the file that is being staged.

    @return A stage relative path.
    """
    components = []
    for i in range(3):
        stage_file, component = os.path.split(stage_file)
        components.insert(0, component)
    return os.path.join(*components)


def stage_remote_payload(moblab, devserver_port, tmp_stage_file):
    """Stages a remote payload on the Moblab's devserver.

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param devserver_port: Externally accessible port to the Moblab devserver.
    @param tmp_stage_file: Path to the remote payload file to stage.

    @return URI to use for downloading the staged payload.

    @raise PayloadStagingError: If we failed to stage the payload.
    """
    # Remove the artifact if previously staged.
    stage_rel_path = get_stage_rel_path(tmp_stage_file)
    target_stage_file = os.path.join(moblab_host.MOBLAB_IMAGE_STORAGE,
                                     stage_rel_path)
    moblab.run('rm -f %s && chown moblab:moblab %s' %
               (target_stage_file, tmp_stage_file))
    tmp_stage_dir, stage_file = os.path.split(tmp_stage_file)
    devserver_host = moblab.web_address.split(':')[0]
    try:
        stage_url = _DEVSERVER_STAGE_URL_TEMPLATE % {
                'moblab': devserver_host,
                'port': devserver_port,
                'stage_dir': tmp_stage_dir,
                'stage_files': stage_file}
        res = urllib2.urlopen(stage_url).read()
    except (urllib2.HTTPError, httplib.HTTPException, urllib2.URLError) as e:
        raise PayloadStagingError('Unable to stage payload on moblab: %s' % e)
    else:
        if res != 'Success':
            raise PayloadStagingError('Staging failed: %s' % res)

    logging.debug('Payload is staged on Moblab as %s', stage_rel_path)
    return _DEVSERVER_PAYLOAD_URI_TEMPLATE % {
            'moblab': devserver_host,
            'port': _DEFAULT_DEVSERVER_PORT,
            'stage_path': os.path.dirname(stage_rel_path)}


def stage_local_payload(moblab, devserver_port, tmp_stage_dir, payload):
    """Stages a local payload on the MobLab's devserver.

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param devserver_port: Externally accessible port to the Moblab devserver.
    @param tmp_stage_dir: Path of temporary staging directory on the Moblab.
    @param payload: Path to the local payload file to stage.

    @return Tuple consisting a payload download URI and the payload type
            ('delta' or 'full').

    @raise PayloadStagingError: If we failed to stage the payload.
    """
    if not os.path.isfile(payload):
        raise PayloadStagingError('Payload file %s does not exist.' % payload)

    # Copy the payload file over to the temporary stage directory.
    tmp_stage_file = os.path.join(tmp_stage_dir, _STAGED_PAYLOAD_FILENAME)
    moblab.send_file(payload, tmp_stage_file)

    # Find the payload type.
    get_payload_type = _GET_PAYLOAD_TYPE % {'payload_file': tmp_stage_file}
    payload_type = moblab.run('python', stdin=get_payload_type).stdout.strip()

    # Stage the copied payload.
    payload_uri = stage_remote_payload(moblab, devserver_port, tmp_stage_file)

    return payload_uri, payload_type


def generate_payload(moblab, devserver_port, tmp_stage_dir, payload_spec):
    """Generates and stages a payload from local image(s).

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param devserver_port: Externally accessible port to the Moblab devserver.
    @param tmp_stage_dir: Path of temporary staging directory on the Moblab.
    @param payload_spec: A string of the form "DST_IMAGE[:SRC_IMAGE]", where
                         DST_IMAGE is a target image and SRC_IMAGE an optional
                         source image.

    @return Tuple consisting a payload download URI and the payload type
            ('delta' or 'full').

    @raise PayloadGenerationError: If we failed to generate the payload.
    @raise PayloadStagingError: If we failed to stage the payload.
    """
    parts = payload_spec.split(':', 1)
    dst_image = parts[0]
    src_image = parts[1] if len(parts) == 2 else None

    if not os.path.isfile(dst_image):
        raise PayloadGenerationError('Target image file %s does not exist.' %
                                     dst_image)
    if src_image and not os.path.isfile(src_image):
        raise PayloadGenerationError('Source image file %s does not exist.' %
                                     src_image)

    tmp_images_dir = moblab.make_tmp_dir()
    try:
        # Copy the images to a temporary location.
        remote_dst_image = os.path.join(tmp_images_dir,
                                        os.path.basename(dst_image))
        moblab.send_file(dst_image, remote_dst_image)
        remote_src_image = None
        if src_image:
            remote_src_image = os.path.join(tmp_images_dir,
                                            os.path.basename(src_image))
            moblab.send_file(src_image, remote_src_image)

        # Generate the payload into a temporary staging directory.
        tmp_stage_file = os.path.join(tmp_stage_dir, _STAGED_PAYLOAD_FILENAME)
        gen_cmd = ['brillo_update_payload', 'generate',
                   '--payload', tmp_stage_file,
                   '--target_image', remote_dst_image]
        if remote_src_image:
            payload_type = 'delta'
            gen_cmd += ['--source_image', remote_src_image]
        else:
            payload_type = 'full'

        moblab.run(' '.join(gen_cmd), stdout_tee=None, stderr_tee=None)
    finally:
        moblab.run('rm -rf %s' % tmp_images_dir)

    # Stage the generated payload.
    payload_uri = stage_remote_payload(moblab, devserver_port, tmp_stage_file)

    return payload_uri, payload_type


def stage_payload(moblab, devserver_port, tmp_dir, use, payload_spec):
    """Stages the payload based on a given specification.

    @param moblab: MoblabHost representing the MobLab being used for testing.
    @param devserver_port: Externally accessible port to the Moblab devserver.
    @param tmp_dir: Path of temporary static subdirectory.
    @param use: String defining the use for the payload, either 'source' or
                'target'.
    @param payload_spec: Either a string of the form
                         "PAYLOAD:DST_IMAGE[:SRC_IMAGE]" describing how to
                         generate a new payload from a target and (optionally)
                         source image; or path to a local payload file.

    @return Tuple consisting a payload download URI and the payload type
            ('delta' or 'full').

    @raise PayloadGenerationError: If we failed to generate the payload.
    @raise PayloadStagingError: If we failed to stage the payload.
    """
    tmp_stage_dir = os.path.join(
            tmp_dir, _DEFAULT_STAGE_PATH_TEMPLATE % {'use': use})
    moblab.run('mkdir -p %s && chown -R moblab:moblab %s' %
               (tmp_stage_dir, tmp_stage_dir))

    spec_gen_prefix = _SPEC_GEN_LABEL + ':'
    if payload_spec.startswith(spec_gen_prefix):
        return generate_payload(moblab, devserver_port, tmp_stage_dir,
                                payload_spec[len(spec_gen_prefix):])
    else:
        return stage_local_payload(moblab, devserver_port, tmp_stage_dir,
                                   payload_spec)


def main(args):
    """The main function."""
    args = brillo_common.parse_args(
            'Set up Moblab for running Brillo AU end-to-end test, then launch '
            'the test (unless otherwise requested).',
            setup_parser=setup_parser)

    moblab, devserver_port = brillo_common.get_moblab_and_devserver_port(
            args.moblab_host)
    tmp_dir = moblab.make_tmp_dir(base=moblab_host.MOBLAB_IMAGE_STORAGE)
    moblab.run('chown -R moblab:moblab %s' % tmp_dir)
    test_args = {'name': _TEST_JOB_NAME}
    try:
        if args.source_payload:
            payload_uri, _ = stage_payload(moblab, devserver_port, tmp_dir,
                                           'source', args.source_payload)
            test_args['source_payload_uri'] = payload_uri
            logging.info('Source payload was staged')

        payload_uri, payload_type = stage_payload(
                moblab, devserver_port, tmp_dir, 'target', args.target_payload)
        test_args['target_payload_uri'] = payload_uri
        test_args['update_type'] = payload_type
        logging.info('Target payload was staged')
    finally:
        moblab.run('rm -rf %s' % tmp_dir)

    brillo_common.do_test_action(args, moblab, _TEST_NAME, test_args)


if __name__ == '__main__':
    try:
        main(sys.argv)
        sys.exit(0)
    except brillo_common.BrilloTestError as e:
        logging.error('Error: %s', e)

    sys.exit(1)

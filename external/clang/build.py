#!/usr/bin/env python
#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
from __future__ import print_function

import argparse
import glob
import multiprocessing
import os
import shutil
import subprocess
import sys

import version


THIS_DIR = os.path.realpath(os.path.dirname(__file__))
ORIG_ENV = dict(os.environ)


def android_path(*args):
    return os.path.realpath(os.path.join(THIS_DIR, '../..', *args))


def build_path(*args):
    # Our multistage build directories will be placed under OUT_DIR if it is in
    # the environment. By default they will be placed under
    # $ANDROID_BUILD_TOP/out.
    top_out = ORIG_ENV.get('OUT_DIR', android_path('out'))
    if not os.path.isabs(top_out):
        top_out = os.path.realpath(top_out)
    return os.path.join(top_out, *args)


def short_version():
    return '.'.join([version.major, version.minor])


def long_version():
    return '.'.join([version.major, version.minor, version.patch])


def install_file(src, dst):
    print('Copying ' + src)
    shutil.copy2(src, dst)


def install_directory(src, dst):
    print('Copying ' + src)
    shutil.copytree(src, dst)


def build(out_dir, prebuilts_path=None, prebuilts_version=None,
          build_all_llvm_tools=None):
    products = (
        'aosp_arm',
        'aosp_arm64',
        'aosp_mips',
        'aosp_mips64',
        'aosp_x86',
        'aosp_x86_64',
    )
    for product in products:
        build_product(out_dir, product, prebuilts_path, prebuilts_version,
                      build_all_llvm_tools)


def build_product(out_dir, product, prebuilts_path, prebuilts_version,
                  build_all_llvm_tools):
    env = dict(ORIG_ENV)
    env['DISABLE_LLVM_DEVICE_BUILDS'] = 'true'
    env['DISABLE_RELOCATION_PACKER'] = 'true'
    env['FORCE_BUILD_LLVM_COMPONENTS'] = 'true'
    env['FORCE_BUILD_SANITIZER_SHARED_OBJECTS'] = 'true'
    env['OUT_DIR'] = out_dir
    env['SKIP_LLVM_TESTS'] = 'true'
    env['SOONG_ALLOW_MISSING_DEPENDENCIES'] = 'true'
    env['TARGET_BUILD_VARIANT'] = 'userdebug'
    env['TARGET_PRODUCT'] = product

    overrides = []
    if prebuilts_path is not None:
        overrides.append('LLVM_PREBUILTS_BASE={}'.format(prebuilts_path))
    if prebuilts_version is not None:
        overrides.append('LLVM_PREBUILTS_VERSION={}'.format(prebuilts_version))

    jobs_arg = '-j{}'.format(multiprocessing.cpu_count())
    targets = ['clang-toolchain']
    if build_all_llvm_tools:
        targets += ['llvm-tools']
    subprocess.check_call(
        ['make', jobs_arg] + overrides + targets, cwd=android_path(), env=env)


def package_toolchain(build_dir, build_name, host, dist_dir):
    package_name = 'clang-' + build_name
    install_host_dir = build_path('install', host)
    install_dir = os.path.join(install_host_dir, package_name)

    # Remove any previously installed toolchain so it doesn't pollute the
    # build.
    if os.path.exists(install_host_dir):
        shutil.rmtree(install_host_dir)

    install_toolchain(build_dir, install_dir, host)

    version_file_path = os.path.join(install_dir, 'AndroidVersion.txt')
    with open(version_file_path, 'w') as version_file:
        version_file.write('{}.{}.{}\n'.format(
            version.major, version.minor, version.patch))

    tarball_name = package_name + '-' + host
    package_path = os.path.join(dist_dir, tarball_name) + '.tar.bz2'
    print('Packaging ' + package_path)
    args = [
        'tar', '-cjC', install_host_dir, '-f', package_path, package_name
    ]
    subprocess.check_call(args)


def install_toolchain(build_dir, install_dir, host):
    install_built_host_files(build_dir, install_dir, host)
    install_sanitizer_scripts(install_dir)
    install_scan_scripts(install_dir)
    install_analyzer_scripts(install_dir)
    install_headers(build_dir, install_dir, host)
    install_profile_rt(build_dir, install_dir, host)
    install_sanitizers(build_dir, install_dir, host)
    install_license_files(install_dir)
    install_repo_prop(install_dir)


def install_built_host_files(build_dir, install_dir, host):
    is_windows = host.startswith('windows')
    is_darwin = host.startswith('darwin-x86')
    bin_ext = '.exe' if is_windows else ''

    if is_windows:
        lib_ext = '.dll'
    elif is_darwin:
        lib_ext = '.dylib'
    else:
        lib_ext = '.so'

    built_files = [
        'bin/clang' + bin_ext,
        'bin/clang++' + bin_ext,
    ]
    if host != 'windows-x86':
        built_files.extend([
            'bin/FileCheck' + bin_ext,
            'bin/llvm-as' + bin_ext,
            'bin/llvm-dis' + bin_ext,
            'bin/llvm-link' + bin_ext,
            'lib64/libc++' + lib_ext,
            'lib64/libLLVM' + lib_ext,
            'lib64/LLVMgold' + lib_ext,
        ])

    for built_file in built_files:
        dirname = os.path.dirname(built_file)
        install_path = os.path.join(install_dir, dirname)
        if not os.path.exists(install_path):
            os.makedirs(install_path)

        built_path = os.path.join(build_dir, 'host', host, built_file)
        install_file(built_path, install_path)

        file_name = os.path.basename(built_file)

        # Only strip bin files (not libs) on darwin.
        if not is_darwin or built_file.startswith('bin/'):
            subprocess.check_call(
                ['strip', os.path.join(install_path, file_name)])


def install_sanitizer_scripts(install_dir):
    script_path = android_path(
        'external/compiler-rt/lib/asan/scripts/asan_device_setup')
    shutil.copy2(script_path, os.path.join(install_dir, 'bin'))


def install_analyzer_scripts(install_dir):
    """Create and install bash scripts for invoking Clang for analysis."""
    analyzer_text = (
        '#!/bin/bash\n'
        'if [ "$1" != "-cc1" ]; then\n'
        '    `dirname $0`/../clang{clang_suffix} -target {target} "$@"\n'
        'else\n'
        '    # target/triple already spelled out.\n'
        '    `dirname $0`/../clang{clang_suffix} "$@"\n'
        'fi\n'
    )

    arch_target_pairs = (
        ('arm64-v8a', 'aarch64-none-linux-android'),
        ('armeabi', 'armv5te-none-linux-androideabi'),
        ('armeabi-v7a', 'armv7-none-linux-androideabi'),
        ('armeabi-v7a-hard', 'armv7-none-linux-androideabi'),
        ('mips', 'mipsel-none-linux-android'),
        ('mips64', 'mips64el-none-linux-android'),
        ('x86', 'i686-none-linux-android'),
        ('x86_64', 'x86_64-none-linux-android'),
    )

    for arch, target in arch_target_pairs:
        arch_path = os.path.join(install_dir, 'bin', arch)
        os.makedirs(arch_path)

        analyzer_file_path = os.path.join(arch_path, 'analyzer')
        print('Creating ' + analyzer_file_path)
        with open(analyzer_file_path, 'w') as analyzer_file:
            analyzer_file.write(
                analyzer_text.format(clang_suffix='', target=target))

        analyzerpp_file_path = os.path.join(arch_path, 'analyzer++')
        print('Creating ' + analyzerpp_file_path)
        with open(analyzerpp_file_path, 'w') as analyzerpp_file:
            analyzerpp_file.write(
                analyzer_text.format(clang_suffix='++', target=target))


def install_scan_scripts(install_dir):
    tools_install_dir = os.path.join(install_dir, 'tools')
    os.makedirs(tools_install_dir)
    tools = ('scan-build', 'scan-view')
    tools_dir = android_path('external/clang/tools')
    for tool in tools:
        tool_path = os.path.join(tools_dir, tool)
        install_path = os.path.join(install_dir, 'tools', tool)
        install_directory(tool_path, install_path)


def install_headers(build_dir, install_dir, host):
    def should_copy(path):
        if os.path.basename(path) in ('Makefile', 'CMakeLists.txt'):
            return False
        _, ext = os.path.splitext(path)
        if ext == '.mk':
            return False
        return True

    headers_src = android_path('external/clang/lib/Headers')
    headers_dst = os.path.join(
        install_dir, 'lib64/clang', short_version(), 'include')
    os.makedirs(headers_dst)
    for header in os.listdir(headers_src):
        if not should_copy(header):
            continue
        install_file(os.path.join(headers_src, header), headers_dst)

    install_file(android_path('bionic/libc/include/stdatomic.h'), headers_dst)

    # arm_neon.h gets produced as part of external/clang/lib/Basic/Android.mk.
    # We must bundle the resulting file as part of the official Clang headers.
    arm_neon_h = os.path.join(
        build_dir, 'host', host, 'obj/STATIC_LIBRARIES/'
        'libclangBasic_intermediates/include/clang/Basic/arm_neon.h')
    install_file(arm_neon_h, headers_dst)

    os.symlink(short_version(),
               os.path.join(install_dir, 'lib64/clang', long_version()))


def install_profile_rt(build_dir, install_dir, host):
    lib_dir = os.path.join(
        install_dir, 'lib64/clang', short_version(), 'lib/linux')
    os.makedirs(lib_dir)

    install_target_profile_rt(build_dir, lib_dir)

    # We only support profiling libs for Linux and Android.
    if host == 'linux-x86':
        install_host_profile_rt(build_dir, host, lib_dir)


def install_target_profile_rt(build_dir, lib_dir):
    product_to_arch = {
        'generic': 'arm',
        'generic_arm64': 'aarch64',
        'generic_mips': 'mipsel',
        'generic_mips64': 'mips64el',
        'generic_x86': 'i686',
        'generic_x86_64': 'x86_64',
    }

    for product, arch in product_to_arch.items():
        product_dir = os.path.join(build_dir, 'target/product', product)
        static_libs = os.path.join(product_dir, 'obj/STATIC_LIBRARIES')
        built_lib = os.path.join(
            static_libs, 'libprofile_rt_intermediates/libprofile_rt.a')
        lib_name = 'libclang_rt.profile-{}-android.a'.format(arch)
        install_file(built_lib, os.path.join(lib_dir, lib_name))


def install_host_profile_rt(build_dir, host, lib_dir):
    arch_to_obj_dir = {
        'i686': 'obj32',
        'x86_64': 'obj',
    }

    for arch, obj_dir in arch_to_obj_dir.items():
        static_libs = os.path.join(
            build_dir, 'host', host, obj_dir, 'STATIC_LIBRARIES')
        built_lib = os.path.join(
            static_libs, 'libprofile_rt_intermediates/libprofile_rt.a')
        lib_name = 'libclang_rt.profile-{}.a'.format(arch)
        install_file(built_lib, os.path.join(lib_dir, lib_name))


def install_sanitizers(build_dir, install_dir, host):
    headers_src = android_path('external/compiler-rt/include/sanitizer')
    clang_lib = os.path.join(install_dir, 'lib64/clang', short_version())
    headers_dst = os.path.join(clang_lib, 'include/sanitizer')
    lib_dst = os.path.join(clang_lib, 'lib/linux')
    install_directory(headers_src, headers_dst)

    if host == 'linux-x86':
        install_host_sanitizers(build_dir, host, lib_dst)

    # Tuples of (product, arch, libdir)
    product_to_arch = (
        ('generic', 'arm', 'lib'),
        ('generic_arm64', 'aarch64', 'lib64'),
        ('generic_x86', 'i686', 'lib'),
    )

    for product, arch, libdir in product_to_arch:
        product_dir = os.path.join(build_dir, 'target/product', product)
        system_dir = os.path.join(product_dir, 'system')
        system_lib_dir = os.path.join(system_dir, libdir)
        lib_name = 'libclang_rt.asan-{}-android.so'.format(arch)
        built_lib = os.path.join(system_lib_dir, lib_name)
        install_file(built_lib, lib_dst)


def install_host_sanitizers(build_dir, host, lib_dst):
    # Tuples of (name, multilib).
    libs = (
        ('asan', True),
        ('asan_cxx', True),
        ('ubsan_standalone', True),
        ('ubsan_standalone_cxx', True),
        ('tsan', False),
        ('tsan_cxx', False),
    )

    obj32 = os.path.join(build_dir, 'host', host, 'obj32/STATIC_LIBRARIES')
    obj64 = os.path.join(build_dir, 'host', host, 'obj/STATIC_LIBRARIES')
    for lib, is_multilib in libs:
        built_lib_name = 'lib{}.a'.format(lib)

        obj64_dir = os.path.join(obj64, 'lib{}_intermediates'.format(lib))
        lib64_name = 'libclang_rt.{}-x86_64.a'.format(lib)
        built_lib64 = os.path.join(obj64_dir, built_lib_name)
        install_file(built_lib64, os.path.join(lib_dst, lib64_name))
        if is_multilib:
            obj32_dir = os.path.join(obj32, 'lib{}_intermediates'.format(lib))
            lib32_name = 'libclang_rt.{}-i686.a'.format(lib)
            built_lib32 = os.path.join(obj32_dir, built_lib_name)
            install_file(built_lib32, os.path.join(lib_dst, lib32_name))


def install_license_files(install_dir):
    projects = (
        'clang',
        'compiler-rt',
        'libcxx',
        'libcxxabi',
        'libunwind_llvm',
        'llvm',
    )

    notices = []
    for project in projects:
        project_path = android_path('external', project)
        license_pattern = os.path.join(project_path, 'MODULE_LICENSE_*')
        for license_file in glob.glob(license_pattern):
            install_file(license_file, install_dir)
        with open(os.path.join(project_path, 'NOTICE')) as notice_file:
            notices.append(notice_file.read())
    with open(os.path.join(install_dir, 'NOTICE'), 'w') as notice_file:
        notice_file.write('\n'.join(notices))


def install_repo_prop(install_dir):
    file_name = 'repo.prop'

    dist_dir = os.environ.get('DIST_DIR')
    if dist_dir is not None:
        dist_repo_prop = os.path.join(dist_dir, file_name)
        shutil.copy(dist_repo_prop, install_dir)
    else:
        out_file = os.path.join(install_dir, file_name)
        with open(out_file, 'w') as prop_file:
            cmd = [
                'repo', 'forall', '-c',
                'echo $REPO_PROJECT $(git rev-parse HEAD)',
            ]
            subprocess.check_call(cmd, stdout=prop_file)


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '--build-name', default='dev', help='Release name for the package.')

    multi_stage_group = parser.add_mutually_exclusive_group()
    multi_stage_group.add_argument(
        '--multi-stage', action='store_true', default=True,
        help='Perform multi-stage build (enabled by default).')
    multi_stage_group.add_argument(
        '--no-multi-stage', action='store_false', dest='multi_stage',
        help='Do not perform multi-stage build.')

    parser.add_argument(
        '--build-all-llvm-tools', action='store_true', default=True,
        help='Build all the LLVM tools/utilities.')

    parser.add_argument(
        '--no-build-all-llvm-tools', action='store_false',
        dest='build_all_llvm_tools',
        help='Build all the LLVM tools/utilities.')

    return parser.parse_args()


def main():
    args = parse_args()

    if sys.platform.startswith('linux'):
        hosts = ['linux-x86', 'windows-x86']
    elif sys.platform == 'darwin':
        hosts = ['darwin-x86']
    else:
        raise RuntimeError('Unsupported host: {}'.format(sys.platform))

    stage_1_out_dir = build_path('stage1')
    build(out_dir=stage_1_out_dir)
    final_out_dir = stage_1_out_dir
    if args.multi_stage:
        stage_1_install_dir = build_path('stage1-install')
        for host in hosts:
            package_name = 'clang-' + args.build_name
            install_host_dir = os.path.join(stage_1_install_dir, host)
            install_dir = os.path.join(install_host_dir, package_name)

            # Remove any previously installed toolchain so it doesn't pollute
            # the build.
            if os.path.exists(install_host_dir):
                shutil.rmtree(install_host_dir)

            install_toolchain(stage_1_out_dir, install_dir, host)

        stage_2_out_dir = build_path('stage2')
        build(out_dir=stage_2_out_dir, prebuilts_path=stage_1_install_dir,
              prebuilts_version=package_name,
              build_all_llvm_tools=args.build_all_llvm_tools)
        final_out_dir = stage_2_out_dir

    dist_dir = ORIG_ENV.get('DIST_DIR', final_out_dir)
    for host in hosts:
        package_toolchain(final_out_dir, args.build_name, host, dist_dir)


if __name__ == '__main__':
    main()

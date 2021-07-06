import argparse
import os
import platform
import subprocess
import shlex
import sys
from setuptools import Extension, setup

MIN_MESA_VERSION ='6.3'
PLATFORMS = {'windows', 'linux', 'darwin'}

target = platform.system().lower()

for known in PLATFORMS:
    if target.startswith(known):
        target = known

if target not in PLATFORMS:
    target = 'linux'

if target == 'darwin':
    import os
    from distutils.sysconfig import get_config_var
    from distutils.version import LooseVersion
    if 'MACOSX_DEPLOYMENT_TARGET' not in os.environ:
        current_system = LooseVersion(platform.mac_ver()[0])
        python_target = LooseVersion(get_config_var('MACOSX_DEPLOYMENT_TARGET'))
        if python_target < '10.9' and current_system >= '10.9':
            os.environ['MACOSX_DEPLOYMENT_TARGET'] = '10.9'

def find_osmesa():
    pkg_config = os.getenv('PKGCONFIG','pkg-config')
    _r = subprocess.run([pkg_config,'--version'], capture_output=True)
    if _r.returncode != 0:
        return False

    # check whether osmesa exists in pkg-config
    _r = subprocess.run([pkg_config,f'--atleast-version={MIN_MESA_VERSION}','osmesa'])
    if _r.returncode != 0:
        return False

    # create a argparser for parsing the returns from
    # pkgconfig
    _parser = argparse.ArgumentParser(add_help=False)
    _parser.add_argument("-I", dest="include_dirs", action="append")
    _parser.add_argument("-L", dest="library_dirs", action="append")
    _parser.add_argument("-l", dest="libraries", action="append")

    _r = subprocess.run([pkg_config,'--libs','--cflags','osmesa'],capture_output=True)
    _r.check_returncode()
    args, _ = _parser.parse_known_args(shlex.split(_r.stdout.decode()))
    return args.__dict__

wgl = Extension(
    name='glcontext.wgl',
    sources=['glcontext/wgl.cpp'],
    extra_compile_args=['-fpermissive'] if 'GCC' in sys.version else [],
    libraries=['user32', 'gdi32'],
)

x11 = Extension(
    name='glcontext.x11',
    sources=['glcontext/x11.cpp'],
    extra_compile_args=['-fpermissive'],
    libraries=['dl'],
)

egl = Extension(
    name='glcontext.egl',
    sources=['glcontext/egl.cpp'],
    extra_compile_args=['-fpermissive'],
    libraries=['dl'],
)

darwin = Extension(
    name='glcontext.darwin',
    sources=['glcontext/darwin.cpp'],
    extra_compile_args=['-fpermissive', '-Wno-deprecated-declarations'],
    extra_link_args=['-framework', 'OpenGL', '-Wno-deprecated'],
)

ext_modules = {
    'windows': [wgl],
    'linux': [x11, egl],
    'darwin': [darwin],
}

build_mesa = find_osmesa()
if build_mesa:
    osmesa = Extension(
        name='glcontext.osmesa',
        sources=['glcontext/osmesa.cpp'],
        extra_compile_args=['-fpermissive'] if 'GCC' in sys.version else [],
        **build_mesa
    )
    ext_modules[target] += [osmesa]

setup(
    name='glcontext',
    version='2.3.2',
    description='Portable OpenGL Context',
    long_description=open('README.md').read(),
    long_description_content_type='text/markdown',
    url='https://github.com/moderngl/glcontext',
    author='Szabolcs Dombi',
    author_email='cprogrammer1994@gmail.com',
    license='MIT',
    platforms=['any'],
    packages=['glcontext'],
    ext_modules=ext_modules[target],
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
        'Topic :: Games/Entertainment',
        'Topic :: Multimedia :: Graphics',
        'Topic :: Multimedia :: Graphics :: 3D Rendering',
        'Topic :: Scientific/Engineering :: Visualization',
        'Programming Language :: Python :: 3 :: Only',
    ],
)

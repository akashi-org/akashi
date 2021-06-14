# type: ignore
from setuptools import setup
from setuptools.command.install import install
from setuptools.command.sdist import sdist

from on_publish import pre_sdist, post_sdist
from on_install import post_install


class SdistCommand(sdist):

    def run(self):
        pre_sdist(self)
        super().run()
        post_sdist(self)


class InstallCommand(install):

    def run(self):
        # pre_install(self)
        super().run()
        post_install(self)


def readme():
    with open('README.md') as f:
        return f.read()


setup(
    name='akashi-engine',
    version='0.1.4',
    description='A next-generation video editor',
    long_description=readme(),
    long_description_content_type='text/markdown',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: C++',
        'Programming Language :: Python',
        'Operating System :: POSIX :: Linux',
        'Topic :: Multimedia',
        'Topic :: Multimedia :: Graphics',
        'Topic :: Multimedia :: Graphics :: Editors',
        'Topic :: Multimedia :: Graphics :: Graphics Conversion',
        'Topic :: Multimedia :: Graphics :: Presentation',
        'Topic :: Multimedia :: Graphics :: Viewers',
        'Topic :: Multimedia :: Sound/Audio',
        'Topic :: Multimedia :: Sound/Audio :: Conversion',
        'Topic :: Multimedia :: Sound/Audio :: Editors',
        'Topic :: Multimedia :: Sound/Audio :: Mixers',
        'Topic :: Multimedia :: Sound/Audio :: Players',
        'Topic :: Multimedia :: Video',
        'Topic :: Multimedia :: Video :: Capture',
        'Topic :: Multimedia :: Video :: Conversion',
        'Topic :: Multimedia :: Video :: Display',
        'Topic :: Multimedia :: Video :: Non-Linear Editor'
    ],
    keywords='video nle non-linear video-editor editor editing audio sound media graphics multimedia',
    url='https://github.com/akashi-org/akashi',
    author='crux14',
    license='Apache-2.0',
    packages=['akashi_cli', 'akashi_core'],
    package_data={
        "akashi_cli": ["py.typed"],
        "akashi_core": ["py.typed"],
    },
    cmdclass={
        'sdist': SdistCommand,
        'install': InstallCommand
    },
    install_requires=[
        'requests'
    ],
    python_requires='>=3.9,<3.10',
    entry_points={
        'console_scripts': [
            'akashi = akashi_cli:akashi_cli'
        ],
    },
)

from setuptools import setup

setup(
    name="RTrobot_FS3000",
    version="0.1",
    description="RTrobot FS3000 library",
    author="RTrobot",
    author_email="admin@rtrobot.org",
    url='https://rtrobot.org',
    packages=["RTrobot_FS3000"],
    install_requires=[
    "RPi.GPIO","numpy",
    ],
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: GPL License',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.8',
)


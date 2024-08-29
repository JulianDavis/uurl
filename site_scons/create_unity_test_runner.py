from os import environ
from pathlib import Path

from SCons.Action import Action
from SCons.Builder import Builder

def generate_runner(env, test_src, target=None, other_src=[], libs=[]):
    # HACK: Save the source so I can use this same path later when building the runner
    source_c = test_src

    runner_c = env.Command(
        target = str(Path(test_src).with_suffix('.runner.c')),
        source = test_src,
        action = Action(
            '${UNITY_GENERATE_TEST_RUNNER} ${SOURCE} ${TARGET}',
            '${UNITYCOMSTR}'
        )
    )

    unity_o = env.Object(
        source = '${UNITY_SRC}/unity.c',
        target = 'build/test/unity.o',
    )

    env.Append(
        CPPPATH=[
            '${UNITY_SRC}',
        ]
    )
    runner = env.Program(
        target = str(Path(test_src).with_suffix('.runner')),
        source=[
            source_c,
            runner_c,
            unity_o,
            other_src,
        ],
        LIBS=libs,
    )
    return runner

def generate(env) -> None:
    env.SetDefault(
        UNITY_ROOT = environ['THROW_THE_SWITCH_UNITY_FILEPATH'],
        UNITY_GENERATE_TEST_RUNNER = '${UNITY_ROOT}/auto/generate_test_runner.rb',
        UNITY_SRC = '${UNITY_ROOT}/src',
    )

    if not env.get('VERBOSE', False):
        env.SetDefault(
            UNITYCOMSTR = '  (Unity) $TARGET'
        )

    env.AddMethod(generate_runner, 'CreateUnityTestRunner')

def exists(env) -> bool:
    return True

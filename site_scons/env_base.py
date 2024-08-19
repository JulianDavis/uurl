from SCons.Script import ARGUMENTS
from SCons.Variables import Variables, BoolVariable, ListVariable

def is_env_base_already_present(env):
    tools = [tool for tool in env['TOOLS']]
    return tools.count('env_base') > 1

def add_tool_dependencies(env):
    dependencies = ['gcc', 'gnulink', 'gas', 'ar']
    for dependency in dependencies:
        if dependency not in env['TOOLS']:
            env.Tool(dependency)

def generate(env) -> None:
    if is_env_base_already_present(env):
        return

    add_tool_dependencies(env)

    env.SetDefault(
        ARCH = '',
        SUBARCH = '',
        OS = '',
        MODE = '',
        OPTCFLAGS = '',
        OPTLINKFLAGS = '',

        BUILD_ROOT = '#build',
        BUILD_DIR = '${BUILD_ROOT}/${ARCH}_${SUBARCH}-${OS}/${MODE}',
        STAGING_ROOT = '${BUILD_ROOT}/bin',
        STAGING_DIR = '${STAGING_ROOT}/${ARCH}_${SUBARCH}-${OS}/${MODE}',
    )

    env.Append(
        CFLAGS=[
            '-Wall',
            '-Wextra',
            '-Werror',
            '-Wmissing-prototypes',
            '-Wstrict-prototypes',
            '-Wpointer-arith',
            '$OPTCFLAGS',
        ],
        LINKFLAGS=[
            '$OPTLINKFLAGS',
        ],
        LIBPATH=[
            '${STAGING_DIR}',
        ],
    )

    variables = Variables(None, ARGUMENTS)
    variables.AddVariables(
        BoolVariable(
            'VERBOSE',
            help='',
            default=False
        ),
        ListVariable(
            'BUILD_MODE',
            help='',
            default='all',
            names=('debug', 'release')
        )
    )
    variables.Update(env)

    if not env['VERBOSE']:
        env.SetDefault(
            CCCOMSTR     = '  (CC) $TARGET',
            ARCOMSTR     = '  (AR) $TARGET',
            RANLIBCOMSTR = '  (AR) $TARGET',
            ASCOMSTR     = '  (AS) $TARGET',
            LINKCOMSTR   = '  (LD) $TARGET',
            INSTALLSTR   = 'Install $TARGET',
        )

def exists(env) -> bool:
    return True

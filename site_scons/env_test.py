def generate(env) -> None:
    env.Tool('env_base')

    env.Replace(
        MODE = 'test'
    )

    env.SetDefault(
        OPTCFLAGS=[
            '-g'
        ],
    )

    env.AppendUnique(CFLAGS=['-Wno-missing-prototypes'])

    env.Append(
        CPPDEFINES={
            'TEST': None,
        },
    )

def exists(env) -> bool:
    return True

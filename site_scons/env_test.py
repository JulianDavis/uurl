def generate(env) -> None:
    env.Tool('env_base')

    env.Replace(
        MODE = 'test'
    )

    env.AppendUnique(
        CFLAGS=['-Wno-missing-prototypes']
    )

    env.Append(
        CFLAGS=[
            '-g'
        ],
        CPPDEFINES={
            'TEST': None,
        },
    )

def exists(env) -> bool:
    return True

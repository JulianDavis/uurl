def generate(env) -> None:
    env.Replace(
        MODE = 'debug'
    )

    env.SetDefault(
        OPTCFLAGS=[
            '-g3'
        ],
    )

    env.Append(
        CPPDEFINES={
            'DEBUG': None,
        },
    )

def exists(env) -> bool:
    return True

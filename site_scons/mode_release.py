def generate(env) -> None:
    env.Replace(
        MODE = 'release'
    )

    env.SetDefault(
        OPTCFLAGS=[
            '-OS'
        ],
        OPTLINKFLAGS=[
            '-Wl,--strip-all',
        ],
    )

    env.Append(
        CPPDEFINES={
            'NDEBUG': None,
        },
    )

def exists(env) -> bool:
    return True

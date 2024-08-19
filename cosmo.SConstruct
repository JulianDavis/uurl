base_env = Environment(
    toolpath=['#site_scons'],
    tools=['env_base'],
)

host_env = base_env.Clone()
host_env.Append(
    ARCH = 'x86',
    SUBARCH = '64',
    OS = 'linux',
)

# Setup environment
cosmo_env = host_env.Clone()
cosmo_env.Append(
    CPPDEFINES={
        '_COSMO_SOURCE': None # This is needed for serialize.h
    },
    CPPPATH=[
        '#cosmopolitan',
    ],
    CCFLAGS=[
        '-Wno-sign-compare', # This is needed for parsehttpmessage.c
    ]
)

# Setup build environments
build_mode = base_env['BUILD_MODE']
build_envs = [cosmo_env.Clone(tools=[f'mode_{mode}']) for mode in build_mode]

# Build library
for build_env in build_envs:
    libcosmohttparsemsg = build_env.SConscript(
        'cosmopolitan/SConscript',
        variant_dir='${BUILD_DIR}',
        duplicate=False,
        exports={'env': build_env},
    )
    build_env.Install('${STAGING_DIR}', libcosmohttparsemsg)

# Setup test environment
cosmo_test_env = host_env.Clone(
    tools=['env_test', 'create_unity_test_runner'],
)
cosmo_test_env.Append(
    CPPDEFINES={
        'TEST_COSMO_PARSE': None,
    },
    CPPPATH=[
        '#cosmopolitan',
        '#cosmopolitan/net/http',
    ],
    LIBS=[
        libcosmohttparsemsg,
    ],
)

# Build test runner
cosmo_parse_runner = cosmo_test_env.SConscript(
    'test/SConscript',
    variant_dir='${BUILD_DIR}',
    duplicate=False,
    exports={'env': cosmo_test_env},
)
cosmo_test_env.Install('${STAGING_DIR}/cosmo', cosmo_parse_runner)

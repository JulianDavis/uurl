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
uurl_env = host_env.Clone()
uurl_env.Append(
    CPPPATH=[
        '#uurl',
    ],
)

# Setup build environments
build_mode = base_env['BUILD_MODE']
build_envs = [uurl_env.Clone(tools=[f'mode_{mode}']) for mode in build_mode]

# Build library
for build_env in build_envs:
    libuurl = build_env.SConscript(
        'uurl/SConscript',
        variant_dir='${BUILD_DIR}',
        duplicate=False,
        exports={'env': build_env},
    )
    build_env.Install('${STAGING_DIR}', libuurl)

# Setup test environment
uurl_test_env = host_env.Clone(
    tools=['env_test', 'create_unity_test_runner'],
)
uurl_test_env.Append(
    CPPPATH=[
        '#uurl',
        '#test',
    ],
    LIBS=[
        'uurl',
    ],
    LIBPATH=[
        '${STAGING_ROOT}/x86_64-linux/debug/'
    ]
)

# Build test runner
uurl_parse_response_runner, uurl_parse_request_runner = uurl_test_env.SConscript(
    'test/SConscript',
    variant_dir='${BUILD_DIR}',
    duplicate=False,
    exports={'env': uurl_test_env},
)
uurl_test_env.Install('${STAGING_DIR}/uurl', [uurl_parse_response_runner, uurl_parse_request_runner])

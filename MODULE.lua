return {
    name = 'yabt',
    version = 1,
    deps = {
        yabt_cc_rules = {
            url = 'https://tangled.org/javier.alvarez.engineer/yabt_cc_rules',
            version = 'origin/main',
            hash = '026a79b1ded53bd3d28879c97d1eb4347fed917f',
            type = 'git',
        },
        googletest = {
            url = 'https://github.com/google/googletest.git',
            version = 'v1.17.0',
            hash = '52eb8108c5bdec04579160ae17225d66034bd723',
            external = true,
        },
    }
}

return {
    name = 'yabt',
    version = 1,
    deps = {
        yabt_cc_rules = {
            url = 'https://tangled.org/javier.alvarez.engineer/yabt_cc_rules',
            version = 'origin/main',
            hash = '0871dcf2a9489b29a3c8da59aad480562067d11d',
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

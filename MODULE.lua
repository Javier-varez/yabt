return {
    name = 'yabt',
    version = 1,
    deps = {
        yabt_cc_rules = {
            url = 'https://github.com/Javier-varez/yabt_cc_rules.git',
            version = 'origin/main',
            hash = '2526f0261fa5da88bd054bc6e23b609f1f324bab',
        },
        googletest = {
            url = 'https://github.com/google/googletest.git',
            version = 'v1.17.0',
            hash = '52eb8108c5bdec04579160ae17225d66034bd723',
            external = true,
        },
    }
}

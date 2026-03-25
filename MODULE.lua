return {
    name = 'yabt',
    version = 1,
    deps = {
        yabt_cc_rules = {
            url = 'https://github.com/Javier-varez/yabt_cc_rules.git',
            version = 'origin/main',
            hash = '032905ab07bba6c0bc194f0447a7ca514f4e48b9',
        },
        googletest = {
            url = 'https://github.com/google/googletest.git',
            version = 'v1.17.0',
            hash = '52eb8108c5bdec04579160ae17225d66034bd723',
            external = true,
        },
    }
}

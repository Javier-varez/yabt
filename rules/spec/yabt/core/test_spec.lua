local path = require('yabt/core/path')

describe('Path module', function()
    setup(function()
        _G.SOURCE_DIR = '/source_dir/'
        _G.OUTPUT_DIR = '/output_dir/'
    end)

    test('Input path methods', function()
        local p = path.InPath:new_relative('p2.pdf')
        assert.are.same(p:absolute(), '/source_dir/p2.pdf')
        assert.are.same(p:relative(), 'p2.pdf')

        p = p:withExt('mp4.pdf')
        assert.are.same(p:absolute(), '/output_dir/p2.mp4.pdf')

        p = p:withExt('mp4')
        assert.are.same(p:absolute(), '/output_dir/p2.mp4.mp4')
        p = p:withPrefix('pre')
        assert.are.same(p:absolute(), '/output_dir/prep2.mp4.mp4')
        p = p:withSuffix('suff')
        assert.are.same(p:absolute(), '/output_dir/prep2.mp4.mp4suff')
    end)

    test('output path methods', function()
        local p = path.OutPath:new_relative('p2.pdf')
        assert.are.same(p:absolute(), '/output_dir/p2.pdf')

        p = p:withExt('mp4.pdf')
        assert.are.same(p:absolute(), '/output_dir/p2.mp4.pdf')

        p = p:withExt('mp4')
        assert.are.same(p:absolute(), '/output_dir/p2.mp4.mp4')
        p = p:withPrefix('pre')
        assert.are.same(p:absolute(), '/output_dir/prep2.mp4.mp4')
        p = p:withSuffix('suff')
        assert.are.same(p:absolute(), '/output_dir/prep2.mp4.mp4suff')
    end)
end)

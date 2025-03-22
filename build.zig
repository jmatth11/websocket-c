const std = @import("std");

fn createModule(b: *std.Build, optimize: std.builtin.OptimizeMode, target: std.Build.ResolvedTarget) *std.Build.Module {
    const files: []const []const u8 = &.{
        "src/websocket.c",
        "src/http.c",
    };
    const flags: []const []const u8 = &.{
        "-Wall",
        "-O2",
        "-std=c11",
    };
    const module = b.addModule("ws", .{
        .pic = true,
        .target = target,
        .optimize = optimize,
    });
    module.addCSourceFiles(.{
        .language = .c,
        .files = files,
        .flags = flags,
    });
    module.addIncludePath(b.path("."));
    module.addIncludePath(b.path("./deps/cstd/headers/"));
    module.addIncludePath(b.path("./deps/cstd/deps/utf8-zig/headers/"));
    module.addIncludePath(.{ .cwd_relative = "/usr/include/x86_64-linux-gnu" });
    module.addIncludePath(.{ .cwd_relative = "/usr/include" });
    return module;
}

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});
    const webTarget = b.resolveTargetQuery(.{ .cpu_arch = .wasm32, .os_tag = .freestanding });
    const webLib = b.addLibrary(.{
        .name = "webws",
        .linkage = .static,
        .root_module = createModule(b, optimize, webTarget),
        .use_llvm = true,
    });
    b.installArtifact(webLib);
    const linkage = b.option(std.builtin.LinkMode, "linkage", "Link mode for ws library") orelse .static;
    const nativeTarget = b.standardTargetOptions(.{});
    const nativeLib = b.addLibrary(.{
        .name = "ws",
        .linkage = linkage,
        .root_module = createModule(b, optimize, nativeTarget),
    });
    b.installArtifact(nativeLib);
}

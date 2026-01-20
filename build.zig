const std = @import("std");

fn createModule(
    b: *std.Build,
    optimize: std.builtin.OptimizeMode,
    target: std.Build.ResolvedTarget,
    use_ssl: bool,
) *std.Build.Module {
    const files: []const []const u8 = &.{
        "src/websocket.c",
        "src/http.c",
        "src/net.c",
        "src/reader.c",
        "src/protocol.c",
    };
    const ssl_flag: []const u8 = if (use_ssl) "-DWEBC_USE_SSL=1" else "";
    const flags: []const []const u8 = &.{
        "-Wall",
        "-Wextra",
        "-O2",
        "-std=gnu11",
        ssl_flag,
    };
    const module = b.addModule("ws", .{
        .pic = true,
        .target = target,
        .optimize = optimize,
    });
    if (use_ssl) {
        module.linkSystemLibrary("libssl", .{ .needed = use_ssl });
    }
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
    const use_ssl = b.option(bool, "use_ssl", "Use OpenSSL for WebSocket Secure support.") orelse false;
    const webTarget = b.resolveTargetQuery(.{ .cpu_arch = .wasm32, .os_tag = .freestanding });
    const webLib = b.addLibrary(.{
        .name = "webws",
        .linkage = .static,
        .root_module = createModule(b, optimize, webTarget, use_ssl),
        .use_llvm = true,
    });
    b.installArtifact(webLib);
    const linkage = b.option(std.builtin.LinkMode, "linkage", "Link mode for ws library") orelse .static;
    const nativeTarget = b.standardTargetOptions(.{});
    const nativeLib = b.addLibrary(.{
        .name = "ws",
        .linkage = linkage,
        .root_module = createModule(b, optimize, nativeTarget, use_ssl),
    });
    b.installArtifact(nativeLib);
}

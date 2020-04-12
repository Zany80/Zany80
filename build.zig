const std = @import("std");
const Builder = std.build.Builder;

pub fn build(b: *Builder) void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard release options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall.
    const mode = b.standardReleaseOptions();
    const exe = b.addExecutable("SIMPLE", "src/main.zig");
    exe.c_std = .C99;
    exe.version = .{
        .major = 0,
        .minor = 1,
        .patch = 7,
    };
    var version_buffer: [32]u8 = undefined;
    const version_macro = std.fmt.bufPrint(&version_buffer, "PROJECT_VERSION={}.{}.{}", .{ exe.version.major, exe.version.minor, exe.version.patch }) catch unreachable;
    exe.defineCMacro(version_macro);
    if (exe.target.isWindows()) {
        exe.addObjectFile("src/icon.rc");
        exe.defineCMacro("SIMPLE_WINDOWS");
    } else if (exe.target.isLinux()) {
        exe.defineCMacro("SIMPLE_LINUX");
    }
    exe.addIncludeDir("headers");
    exe.addIncludeDir("headers/SIMPLE/third_party/rapidxml");
    for (&[_][]const u8{
        "src/data.c",        "src/graphics.c",
        //"src/graphics.cc", "src/main.cc",
        "src/plugins.c",     "src/repositories.c",
        "src/ring_buffer.c", "src/XML.cc",
    }) |v| {
        exe.addCSourceFile(v, &[_][]const u8{});
    }
    // Text editor
    exe.addCSourceFile("src/third_party/imguitexteditor/TextEditor.cpp", &[_][]const u8{});
    exe.linkSystemLibrary("c");
    exe.linkSystemLibrary("stdc++");
    exe.linkSystemLibrary("git2");
    exe.setTarget(target);
    exe.setBuildMode(mode);
    exe.install();
}

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
    const exe = b.addExecutable("Zany80", null);
    exe.c_std = .C99;
    exe.is_dynamic = true;
    exe.version = .{
        .major = 0,
        .minor = 2,
        .patch = 0,
    };
    var version_buffer: [32]u8 = undefined;
    const version_macro = std.fmt.bufPrint(&version_buffer, "PROJECT_VERSION={}.{}.{}", .{ exe.version.major, exe.version.minor, exe.version.patch }) catch unreachable;
    exe.defineCMacro(version_macro);
    exe.linkLibC();
    if (mode == .Debug) {
        exe.defineCMacro("_DEBUG");
    }
    for (&[_][]const u8{
        "src/main.c",
        "lib/sokol/sokol.c",
    }) |v| {
        exe.addCSourceFile(v, &[_][]const u8{});
    }
    for (&[_][]const u8{"lib/sokol/"}) |v| {
        exe.addIncludeDir(v);
    }
    if (exe.target.isWindows()) {
        exe.addObjectFile("src/icon.rc");
        exe.defineCMacro("ZANY_WINDOWS");
    } else if (exe.target.isLinux()) {
        exe.defineCMacro("ZANY_LINUX");
        exe.defineCMacro("SOKOL_GLCORE33");
        exe.linkSystemLibraryName("GL");
        exe.linkSystemLibraryName("X11");
        exe.addIncludeDir("/usr/include");
    }
    exe.setTarget(target);
    exe.setBuildMode(mode);

    exe.setVerboseLink(true);
    exe.install();
}

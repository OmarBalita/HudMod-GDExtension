# Made by Malek
# NOTE FROM MALEK: Omar need to review this code

from SCons.Script import * # type: ignore
import os
import shutil
import glob

release = ARGUMENTS.get("release", "no") == "yes"
ARGUMENTS["target"] = "template_release" if release else ARGUMENTS.get("target", "template_debug")

env = SConscript("godot-cpp/SConstruct")

target_platform = str(env.get("platform", ""))
target_arch     = str(env.get("arch", ""))
is_mingw        = env.get("use_mingw", False)

if target_platform == "windows" and target_arch == "arm64" and is_mingw:
    env['ENV']['PATH'] = os.environ['PATH']
    env['CC']     = 'aarch64-w64-mingw32-gcc'
    env['CXX']    = 'aarch64-w64-mingw32-g++'
    env['LINK']   = 'aarch64-w64-mingw32-g++'
    env['AR']     = 'aarch64-w64-mingw32-ar'
    env['RANLIB'] = 'aarch64-w64-mingw32-ranlib'
    env['AS']     = 'aarch64-w64-mingw32-as'

if target_platform in ["linux", "x11"] and target_arch == "arm64":
    env['CC']     = 'aarch64-linux-gnu-gcc'
    env['CXX']    = 'aarch64-linux-gnu-g++'
    env['LINK']   = 'aarch64-linux-gnu-g++'
    env['AR']     = 'aarch64-linux-gnu-ar'
    env['RANLIB'] = 'aarch64-linux-gnu-ranlib'
    env.Append(CCFLAGS=['-march=armv8-a'])

platform_map = {"windows": "windows", "linux": "linux", "x11": "linux"}
arch_map      = {"x86_64": "x8664", "x86_32": "x8632", "arm64": "arm64"}

p_prefix    = platform_map.get(target_platform, target_platform)
a_suffix    = arch_map.get(target_arch, target_arch)
folder_name = "{}_{}".format(p_prefix, a_suffix)

ffmpeg_base         = os.path.abspath("thirdparty/ffmpeg")
ffmpeg_lib_path     = os.path.join(ffmpeg_base, "lib", folder_name)
ffmpeg_include_path = os.path.join(ffmpeg_base, "include")

if not os.path.isdir(ffmpeg_lib_path):
    print("ERROR: FFmpeg lib folder not found: {}\n Available folders: {}".format(
        ffmpeg_lib_path,
        os.listdir(os.path.join(ffmpeg_base, "lib")) if os.path.isdir(os.path.join(ffmpeg_base, "lib")) else "lib/ directory missing",
    ))
    Exit(1)

env.Append(CPPPATH=["src/", ffmpeg_include_path])
env.Append(LIBPATH=[ffmpeg_lib_path])

ffmpeg_libs = ["avformat", "avcodec", "avutil", "swscale", "swresample"]

if target_platform == "windows" and not is_mingw:
    env.Append(LIBS=[lib + ".lib" for lib in ffmpeg_libs])
else:
    env.Append(LIBS=ffmpeg_libs)

if target_platform in ["linux", "x11"]:
    env.Append(LIBS=["m", "z", "dl", "pthread"])

env.VariantDir("build", "src", duplicate=0)

sources = (
    env.Glob("build/audio/*.cpp")    +
    env.Glob("build/core/*.cpp")     +
    env.Glob("build/core/math/*.cpp")     +
    env.Glob("build/core/functionality/*.cpp")     +
    env.Glob("build/register/*.cpp") +
    env.Glob("build/video/*.cpp")
)

out_dir_prefix = "win" if target_platform == "windows" else "linux"
out_dir_suffix = "arm64" if target_arch == "arm64" else ("64" if "64" in target_arch else "32")

out_dir     = "demo/addons/hudmod-gdextension/{}{}/".format(out_dir_prefix, out_dir_suffix)
output_path = "{}libhudmod{}{}".format(out_dir, env["suffix"], env["SHLIBSUFFIX"])

library = env.SharedLibrary(output_path, source=sources)

# Copy the ffmpeg dynamic libraries
ffmpeg_shared_libs = (
    glob.glob(os.path.join(ffmpeg_lib_path, "*.so*")) +
    glob.glob(os.path.join(ffmpeg_lib_path, "*.dll"))
)

os.makedirs(out_dir, exist_ok=True)
for lib in ffmpeg_shared_libs:
    shutil.copy2(lib, out_dir)
    print("Copied: {} -> {}".format(os.path.basename(lib), out_dir))

# Copy .gdextension file
shutil.copy2("./gdextension/hudmod.gdextension", os.path.join(out_dir, ".."))

print("\n--- Build Configuration ---")
print("Target Arch:   {}".format(target_arch))
print("Using MinGW:   {}".format(is_mingw))
print("Release:       {}".format(release))
print("Compiler CXX:  {}".format(env['CXX']))
print("FFmpeg base:   {}".format(ffmpeg_base))
print("Output Path:   {}".format(output_path))
print("----------------------------------\n")

Default(library) # type: ignore

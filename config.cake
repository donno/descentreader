#===------------------------------------------------------------------------===#
#
#                     The Descent map loader
#
# NAME         : config.py
# PURPOSE      : Configures the build system, cake.
# COPYRIGHT    : (c) 2014 Sean Donnellan. All Rights Reserved.
# AUTHORS      : Sean Donnellan (darkdonno@gmail.com)
# DESCRIPTION  : This configures the cake build system to use MSVC and Mingw
#                GCC under Windows.
#
#===------------------------------------------------------------------------===#
from cake.engine import Variant
from cake.async import waitForAsyncResult
from cake.library.compilers import CompilerNotFoundError
from cake.library.env import EnvironmentTool
from cake.library.filesys import FileSystemTool
from cake.library.logging import LoggingTool
from cake.library.project import ProjectTool
from cake.library.script import ScriptTool
from cake.library.shell import ShellTool
from cake.library.variant import VariantTool
from cake.library.zipping import ZipTool
from cake.script import Script

import cake.path
import cake.system

platform = cake.system.platform().lower()
hostArchitecture = cake.system.architecture().lower()
configuration = Script.getCurrent().configuration
engine = Script.getCurrent().engine

# Create the project tool, only enabled during project generation.
projectTool = ProjectTool(configuration=configuration)
projectTool.product = ProjectTool.VS2010
projectTool.enabled = hasattr(engine.options, "createProjects") and \
  engine.options.createProjects

# Add a build success callback that will do the actual project generation.
engine.addBuildSuccessCallback(projectTool.build)

def createVariants(platform, architecture, compiler):
  for target in ["debug", "release"]:
    variant = Variant(
      platform=platform,
      architecture=architecture,
      compiler=compiler.name,
      release=target,
      )
    variant.tools["env"] = env = EnvironmentTool(configuration=configuration)
    variant.tools["script"] = ScriptTool(configuration=configuration)
    variant.tools["logging"] = LoggingTool(configuration=configuration)
    variant.tools["variant"] = VariantTool(configuration=configuration)
    variant.tools["shell"] = ShellTool(configuration=configuration)
    variant.tools["filesys"] = FileSystemTool(configuration=configuration)
    variant.tools["zipping"] = ZipTool(configuration=configuration)
    variant.tools["compiler"] = compilerClone = compiler.clone()
    variant.tools["project"] = projectClone = projectTool.clone()

    # Set a build directory specific to this variant.
    env["VARIANT"] = "-".join([platform, compiler.name, architecture, target])

    # Turn on debug symbols for the debug target.
    compilerClone.debugSymbols = target == "debug"

    # Set the project config and platform names for this variant. Note that if
    # these are not set a default will be used that is based on the variants
    # keywords.
    projectClone.projectConfigName = '%s %s (%s) %s' % (
      platform.capitalize(),
      compiler.name.capitalize(),
      architecture,
      target.capitalize(),
      )
    projectClone.solutionConfigName = target.capitalize()
    projectClone.solutionPlatformName = '%s %s (%s)' % (
      platform.capitalize(),
      compiler.name.capitalize(),
      architecture,
      )

    # Disable all other tools if the project tool is enabled.
    if projectTool.enabled:
      for tool in variant.tools.itervalues():
        if not isinstance(tool, ProjectTool):
          tool.enabled = False

    configuration.addVariant(variant)

# Create GCC Compiler.
try:
  from cake.library.compilers.gcc import findGccCompiler
  compiler = findGccCompiler(configuration=configuration)
  compiler.addLibrary("stdc++")
  createVariants(platform, hostArchitecture, compiler)
except CompilerNotFoundError:
  pass

if cake.system.isWindows():
  # Create MinGW Compiler.
  try:
    from cake.library.compilers.gcc import findMinGWCompiler
    compiler = findMinGWCompiler(configuration=configuration)
    createVariants(platform, hostArchitecture, compiler)
  except CompilerNotFoundError:
    pass

  # Create MSVC Compilers.
  from cake.library.compilers.msvc import (findMsvcCompiler,
                                           getVisualStudio2017Compiler)
  for architecture in ["x86", "x64"]:
    try:
      compiler = getVisualStudio2017Compiler(configuration=configuration,
                                             targetArchitecture=architecture)
    except CompilerNotFoundError:
      try:
        compiler = findMsvcCompiler(configuration=configuration,
                                    architecture=architecture)
      except CompilerNotFoundError:
        pass

    compiler.addDefine("WIN32")
    if architecture in ["x64"]:
      compiler.addDefine("WIN64")
    createVariants(platform, architecture, compiler)

  if sum(1 for _ in configuration.findAllVariants()) == 0:
    engine.raiseError(
      "Error: No variants were registered - no suitable compilers found.\n")

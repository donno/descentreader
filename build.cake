#===------------------------------------------------------------------------===#
#
#                     The Descent map loader
#
# NAME         : build.cake
# PURPOSE      : Describes how the C++ code should be built into an executable.
# COPYRIGHT    : (c) 2014 Sean Donnellan. All Rights Reserved.
# AUTHORS      : Sean Donnellan (darkdonno@gmail.com)
# DESCRIPTION  : This instructs the cake build system how to compile the source
#                files.
#
#===------------------------------------------------------------------------===#
from cake.tools import compiler, env, script, shell, project, variant
from cake.library import waitForAsyncResult, getPath

genProjects = False

env['BUILD'] = script.cwd(
  'build',
  variant.release + '_' + variant.architecture + '_' + variant.compiler)

sources = script.cwd([
  'hog.cpp',
  'hogiterator.cpp',
  'rdl.cpp',
  ])

compiler.addDefine('_CRT_SECURE_NO_WARNINGS')

if variant.compiler == 'mingw':
  compiler.addLibrary('stdc++')

compiler.enableExceptions = True

objs = compiler.objects(
  targetDir=env.expand('$BUILD/objs'),
  sources=sources,
  )

prog = compiler.program(
  target=script.cwd(env.expand('$BUILD'), 'hog'),
  sources=objs,
  )

proj = project.project(
  target=script.cwd('build', 'project', 'descent'),
  intermediateDir=env.expand('$BUILD/objs'),
  items={
    'Source': sources,
    'Include': [],
    '': [script.path],
    },
    )

sln = project.solution(
  target=script.cwd('build', 'project', '_descent'),
  projects=[proj],
  )

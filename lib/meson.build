libpsutils_sources = files(
	'psutils.c',
	'sysdeps/astral.c',
)

libpsutils = shared_library(
	'psutils',
	libpsutils_sources,
	include_directories: psutils_include,
	version: meson.project_version(),
	install: true,
)

libpsutils_dep = declare_dependency(
	include_directories: psutils_include,
	link_with: libpsutils,
)

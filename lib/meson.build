libpsutils_sources = files('psutils.c')

if host_machine.system() == 'astral'
	libpsutils_sources += files('sysdeps/astral.c')
else
	error('No psutils sysdep implemented for ' + host_machine.system())
endif

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
